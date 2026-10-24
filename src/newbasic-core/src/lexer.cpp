#include "newbasic/lexer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <string>

#include "newbasic/errors.h"

namespace newbasic {
namespace {

// Reserved words (statements, functions, operators), GW-BASIC 3.23.
// Must be sorted for the binary search in IsKeyword().
const char* const kKeywordList[] = {
    "ABS",     "AND",          "ASC",       "ATN",       "AUTO",       "BASE",     "BEEP",
    "BLOAD",   "BSAVE",        "CALL",      "CASE",      "CDBL",       "CHAIN",    "CHDIR",
    "CHR$",    "CINT",         "CIRCLE",    "CLEAR",     "CLOSE",      "CLS",      "COLOR",
    "COM",     "COMMON",       "CONST",     "CONT",      "COS",        "CSNG",     "CSRLIN",
    "CVD",
    "CVI",     "CVS",          "DATA",      "DATE$",     "DEF",        "DEFDBL",   "DEFINT",
    "DEFSNG",  "DEFSTR",       "DELETE",    "DIM",       "DRAW",       "DRAWTEXT", "EDIT",
    "ELSE",    "END",          "ENVIRON",   "ENVIRON$",  "EOF",        "EQV",      "ERASE",
    "ERL",     "ERR",          "ERROR",     "EXP",       "FIELD",      "FILES",    "FIX",
    "FN",      "FOR",          "FRE",       "GET",       "GOSUB",      "GOTO",     "HEX$",
    "IF",      "IMP",          "INKEY$",    "INP",       "INPUT",      "INPUT$",   "INSTR",
    "INT",     "IOCTL",        "KEY",       "KILL",      "LABEL",      "LEFT$",    "LEN",
    "LET",
    "LINE",    "LIST",         "LLIST",     "LOAD",      "LOC",        "LOCATE",   "LOF",
    "LOG",     "LPOS",         "LPRINT",    "LSET",      "MEM",        "MERGE",    "MID$",
    "MKD$",    "MKDIR",        "MKI$",      "MKS$",      "MOD",        "MOTOR",    "MOUSE",
    "MOUSEB",  "MOUSEBUTTONS", "MOUSEPOSX", "MOUSEPOSY", "MOUSEWHEEL", "MOUSEX",   "MOUSEY",
    "NAME",    "NEW",          "NEXT",      "NOT",       "OCT$",       "OFF",      "ON",
    "OPEN",    "OPTION",       "OR",        "OUT",       "PAINT",      "PALETTE",  "PCOPY",
    "PEEK",    "PEN",          "PLAY",      "PMAP",      "POINT",      "POKE",     "POP",
    "POS",     "PRESET",       "PRINT",     "PSET",      "PUSH",       "PUT",      "RANDOMIZE",
    "READ",    "REM",          "RENUM",     "RESET",     "RESTORE",    "RESUME",   "RETURN",
    "RIGHT$",  "RMDIR",        "RND",       "RSET",      "RUN",        "SAVE",     "SCREEN",
    "SCREENH", "SCREENW",      "SELECT",    "SGN",       "SHELL",      "SIN",      "SLEEP",
    "SOUND",   "SPACE$",       "SPC",       "SQR",       "STACK",      "STEP",     "STICK",
    "STOP",    "STR$",         "STRIG",     "STRING$",   "SWAP",       "SYSTEM",   "TAB",
    "TAN",     "TEXTHEIGHT",   "TEXTWIDTH", "THEN",      "TIME$",      "TIMER",    "TO",
    "TROFF",   "TRON",         "USING",     "USR",       "VAL",        "VARPTR",   "VIEW",
    "VIEWX1",  "VIEWX2",       "VIEWY1",    "VIEWY2",    "WAIT",       "WEND",     "WHILE",
    "WIDTH",   "WINDOW",       "WRITE",     "XOR",
};

bool IsNameStart(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsNameChar(char c) {
  return IsNameStart(c) || (c >= '0' && c <= '9') || c == '.';
}

bool IsTypeSuffix(char c) {
  return c == '%' || c == '!' || c == '#' || c == '&' || c == '$';
}

bool IsDigit(char c) {
  return c >= '0' && c <= '9';
}

char UpperChar(char c) {
  return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

} // namespace

std::string Lexer::Upper(const std::string& text) {
  std::string out = text;
  for (char& c : out)
    c = UpperChar(c);
  return out;
}

std::size_t Lexer::KeywordCount() {
  return sizeof(kKeywordList) / sizeof(kKeywordList[0]);
}

const char* Lexer::KeywordAt(std::size_t index) {
  return index < KeywordCount() ? kKeywordList[index] : nullptr;
}

// A keyword is only a keyword when the whole name is reserved: `TOTAL` is a
// variable, while `TO` is the keyword. This deliberately drops GW-BASIC's
// keyword-first tokenization of run-together forms such as `GOTO100`.
bool Lexer::IsKeyword(const std::string& upper) {
  const auto* begin = std::begin(kKeywordList);
  const auto* end = std::end(kKeywordList);
  const auto it = std::lower_bound(
      begin, end, upper, [](const char* a, const std::string& b) { return std::string(a) < b; });
  return it != end && upper == *it;
}

bool Lexer::Tokenize(const std::string& line, std::vector<Token>* out, std::string* error,
                     int* error_pos) {
  out->clear();
  std::size_t i = 0;
  bool space = false;
  auto fail = [&](const std::string& message, std::size_t pos) {
    *error = message;
    *error_pos = static_cast<int>(pos);
    return false;
  };

  while (i < line.size()) {
    const char c = line[i];
    if (c == ' ' || c == '\t') {
      space = true;
      ++i;
      continue;
    }

    // Comment to end of line.
    if (c == '\'') {
      Token token;
      token.kind = Tok::Op;
      token.text = "'";
      token.pos = static_cast<int>(i);
      token.space_before = space;
      out->push_back(std::move(token));
      break; // the rest of the line is the comment
    }

    // String literal (doubled quotes are an embedded quote).
    if (c == '"') {
      Token token;
      token.kind = Tok::String;
      token.pos = static_cast<int>(i);
      token.space_before = space;
      ++i;
      bool closed = false;
      while (i < line.size()) {
        if (line[i] == '"') {
          if (i + 1 < line.size() && line[i + 1] == '"') {
            token.str += '"';
            i += 2;
            continue;
          }
          ++i;
          closed = true;
          break;
        }
        token.str += line[i];
        ++i;
      }
      if (!closed)
        return fail("Unterminated string", token.pos);
      out->push_back(std::move(token));
      space = false;
      continue;
    }

    // Numeric literal: &H/&O or decimal/fixed/exponential, optional suffix.
    if (IsDigit(c) || (c == '.' && i + 1 < line.size() && IsDigit(line[i + 1])) ||
        (c == '&' && i + 1 < line.size() &&
         (UpperChar(line[i + 1]) == 'H' || UpperChar(line[i + 1]) == 'O'))) {
      Token token;
      token.kind = Tok::Number;
      token.pos = static_cast<int>(i);
      token.space_before = space;
      space = false;

      const std::size_t start = i;
      if (c == '&') {
        const bool hex = UpperChar(line[i + 1]) == 'H';
        const int base = hex ? 16 : 8;
        i += 2;
        const std::size_t digits_begin = i;
        long long v = 0;
        while (i < line.size()) {
          const char d = line[i];
          int dv = -1;
          if (IsDigit(d))
            dv = d - '0';
          else if (hex && d >= 'a' && d <= 'f')
            dv = d - 'a' + 10;
          else if (hex && d >= 'A' && d <= 'F')
            dv = d - 'A' + 10;
          if (dv < 0 || dv >= base)
            break;
          v = v * base + dv;
          ++i;
        }
        if (i == digits_begin) {
          return fail("Expected digits after &H/&O", start);
        }
        if (v > std::numeric_limits<std::int32_t>::max()) {
          return fail("Overflow", start);
        }
        if (v <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
          token.number = Value::Int(static_cast<std::int32_t>(v));
        } else {
          token.number = Value::Long(static_cast<std::int32_t>(v));
        }
        out->push_back(std::move(token));
        continue;
      }

      bool has_digits = false;
      bool has_dot = false;
      while (i < line.size() && IsDigit(line[i])) {
        ++i;
        has_digits = true;
      }
      if (i < line.size() && line[i] == '.') {
        has_dot = true;
        ++i;
        while (i < line.size() && IsDigit(line[i])) {
          ++i;
          has_digits = true;
        }
      }
      if (!has_digits)
        return fail("Expected numeric literal", start);
      int exponent_kind = 0; // 0 none, 1 = E (Single), 2 = D (Double)
      if (i < line.size() && (UpperChar(line[i]) == 'E' || UpperChar(line[i]) == 'D')) {
        // `E` is an exponent marker only when followed by digits/sign+digits.
        std::size_t look = i + 1;
        if (look < line.size() && (line[look] == '+' || line[look] == '-'))
          ++look;
        if (look < line.size() && IsDigit(line[look])) {
          exponent_kind = UpperChar(line[i]) == 'E' ? 1 : 2;
          i = look;
          while (i < line.size() && IsDigit(line[i]))
            ++i;
        }
      }
      char suffix = '\0';
      if (i < line.size() && IsTypeSuffix(line[i]) && line[i] != '$') {
        suffix = line[i];
        ++i;
      }
      if (suffix == '$')
        return fail("Type mismatch", start);

      const std::string literal = line.substr(start + (c == '&' ? 2 : 0),
                                              i - start - (suffix ? 1 : 0) - (c == '&' ? 2 : 0));
      double d = 0.0;
      try {
        d = std::stod(literal);
      } catch (...) {
        return fail("Overflow", start);
      }
      int digit_count = 0;
      for (char ch : literal) {
        if (IsDigit(ch))
          ++digit_count;
      }

      if (suffix == '%') {
        if (has_dot || exponent_kind != 0)
          return fail("Invalid numeric constant", start);
        if (d > 2147483647.0 || d < -2147483648.0)
          return fail("Overflow", start);
        token.number = Value::Int(static_cast<std::int32_t>(d));
      } else if (suffix == '&') {
        if (has_dot || exponent_kind != 0)
          return fail("Invalid numeric constant", start);
        if (d > 2147483647.0 || d < -2147483648.0)
          return fail("Overflow", start);
        token.number = Value::Long(static_cast<std::int32_t>(d));
      } else if (suffix == '!') {
        token.number = Value::Sgl(d);
      } else if (suffix == '#' || suffix == '^') {
        token.number = Value::Dbl(d);
      } else if (exponent_kind == 1) {
        token.number = Value::Sgl(d);
      } else if (exponent_kind == 2) {
        token.number = Value::Dbl(d);
      } else if (!has_dot && digit_count <= 7) {
        // Integer literals use the 32-bit Integer kind when in range.
        if (d <= 2147483647.0)
          token.number = Value::Int(static_cast<std::int32_t>(d));
        else
          token.number = Value::Long(static_cast<std::int32_t>(d));
      } else if (digit_count <= 7) {
        token.number = Value::Sgl(d);
      } else {
        token.number = Value::Dbl(d);
      }
      out->push_back(std::move(token));
      continue;
    }

    // Whole-word keyword or identifier. Keywords are recognised only when the
    // entire name is reserved; `TOTAL`, `FORT`, `PRINTX` and `FNA` are names.
    if (IsNameStart(c)) {
      Token token;
      token.pos = static_cast<int>(i);
      token.space_before = space;
      space = false;
      const std::size_t name_begin = i;
      while (i < line.size() && IsNameChar(line[i]))
        ++i;
      std::string name = Upper(line.substr(name_begin, i - name_begin));
      if (i < line.size() && IsTypeSuffix(line[i])) {
        name += line[i];
        ++i;
      }
      token.kind = Lexer::IsKeyword(name) ? Tok::Keyword : Tok::Ident;
      token.text = std::move(name);
      out->push_back(std::move(token));
      continue;
    }

    // Operators and punctuation.
    {
      Token token;
      token.kind = Tok::Op;
      token.pos = static_cast<int>(i);
      token.space_before = space;
      space = false;
      if (i + 1 < line.size()) {
        const std::string two = line.substr(i, 2);
        if (two == "<=" || two == ">=" || two == "<>") {
          token.text = two;
          i += 2;
          out->push_back(std::move(token));
          continue;
        }
      }
      token.text = std::string(1, UpperChar(c));
      ++i;
      out->push_back(std::move(token));
    }
  }

  Token end;
  end.kind = Tok::EndOfLine;
  end.pos = static_cast<int>(line.size());
  end.space_before = space;
  out->push_back(std::move(end));
  return true;
}

} // namespace newbasic
