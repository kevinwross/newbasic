#include "newbasic/frontend.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/lexer.h"

namespace newbasic {
namespace {

// Statement keywords that are recognized but not implemented yet. They parse
// (with their operands skipped) and raise "Advanced Feature" at run time, so
// programs using them fail predictably instead of misparsing.
//
// Keep this list honest: a statement that has an explicit branch above must not
// appear here, and tests/test_inventory.cpp checks both directions. Statements that
// exist only as direct-mode commands (FILES, KILL, MKDIR, CHDIR, RMDIR) stay,
// because they are not available inside a program.
const char* const kUnimplementedStatements[] = {
    "CHDIR", "COM", "ENVIRON", "FILES", "INP",   "IOCTL", "KEY", "KILL",   "MKDIR", "MOTOR",
    "NAME",  "OUT", "PEN",     "POKE",  "RMDIR", "STICK", "STRIG", "USR", "VARPTR", "WAIT",
};

// Tab/SPC are only meaningful inside PRINT, so they are handled there.
struct FuncEntry {
  const char* name;
  Func func;
};

const FuncEntry kFuncTable[] = {
    {"ABS", Func::Abs},
    {"ASC", Func::Asc},
    {"ATN", Func::Atn},
    {"CDBL", Func::Cdbl},
    {"CHR$", Func::Chr},
    {"CINT", Func::Cint},
    {"COS", Func::Cos},
    {"CSNG", Func::Csng},
    {"CSRLIN", Func::Csrlin},
    {"CVD", Func::Cvd},
    {"CVI", Func::Cvi},
    {"CVS", Func::Cvs},
    {"DATE$", Func::Date},
    {"ENVIRON$", Func::Environ},
    {"EXP", Func::Exp},
    {"FIX", Func::Fix},
    {"HEX$", Func::Hex},
    {"INKEY$", Func::Inkey},
    {"INSTR", Func::Instr},
    {"INT", Func::Int},
    {"LEFT$", Func::Left},
    {"LEN", Func::Len},
    {"LOG", Func::Log},
    {"MID$", Func::Mid},
    {"MKD$", Func::Mkd},
    {"MKI$", Func::Mki},
    {"MKS$", Func::Mks},
    {"LOF", Func::Lof},
    {"LOC", Func::Loc},
    {"OCT$", Func::Oct},
    {"POS", Func::Pos},
    {"RIGHT$", Func::Right},
    {"RND", Func::Rnd},
    {"SGN", Func::Sgn},
    {"SIN", Func::Sin},
    {"SPACE$", Func::Space},
    {"SQR", Func::Sqr},
    {"STACKAT", Func::StackAt},
    {"STACKDEPTH", Func::StackDepth},
    {"STR$", Func::Str},
    {"STRING$", Func::String},
    {"TAN", Func::Tan},
    {"TIME$", Func::Time},
    {"TIMER", Func::Timer},
    {"TEXTWIDTH", Func::TextWidth},
    {"TEXTHEIGHT", Func::TextHeight},
    {"MOUSEPOSX", Func::MousePosX},
    {"MOUSEPOSY", Func::MousePosY},
    {"MOUSEBUTTONS", Func::MouseButtons},
    {"MOUSEWHEEL", Func::MouseWheel},
    {"SCREENW", Func::ScreenWidth},
    {"SCREENH", Func::ScreenHeight},
    {"VIEWX1", Func::ViewX1},
    {"VIEWY1", Func::ViewY1},
    {"VIEWX2", Func::ViewX2},
    {"VIEWY2", Func::ViewY2},
    {"VAL", Func::Val},
    {"EOF", Func::Eof},
    // Machine-level functions (M13): recognised so they report a clear refusal
    // rather than a syntax error.
    {"FRE", Func::Fre},
    {"INP", Func::Inp},
    {"PEEK", Func::Peek},
    {"PMAP", Func::Pmap},
    {"POINT", Func::Point},
    {"MOUSE", Func::Mouse},
    {"MOUSEB", Func::MouseB},
    {"MOUSEX", Func::MouseX},
    {"MOUSEY", Func::MouseY},
    {"STICK", Func::Stick},
    {"STRIG", Func::Strig},
    {"USR", Func::Usr},
    {"VARPTR", Func::Varptr},
};

bool LookupFunc(const std::string& name, Func* out) {
  for (const FuncEntry& entry : kFuncTable) {
    if (name == entry.name) {
      *out = entry.func;
      return true;
    }
  }
  return false;
}

bool InList(const char* const* list, std::size_t count, const std::string& value) {
  for (std::size_t i = 0; i < count; ++i) {
    if (value == list[i])
      return true;
  }
  return false;
}

Op PushOpForKind(Value::Kind kind) {
  switch (kind) {
  case Value::Kind::Int:
    return Op::PushInt;
  case Value::Kind::Long:
    return Op::PushLong;
  case Value::Kind::Sgl:
    return Op::PushSgl;
  case Value::Kind::Str:
    return Op::PushStr;
  default:
    return Op::PushDbl;
  }
}

class Compiler {
public:
  Compiler(Program* out, CompileError* error, bool direct)
      : out_(out), error_(error), direct_(direct) {
    for (char& c : default_type_)
      c = '!'; // Single until DEFINT/DEFSNG/DEFDBL/DEFSTR says otherwise
  }

  // Resolves the storage type of a variable or array name: an explicit type
  // suffix wins, otherwise the DEF* default for the initial letter.
  std::string TypeName(const std::string& name) {
    if (name.empty())
      return name;
    const char last = name.back();
    if (last == '%' || last == '&' || last == '!' || last == '#' || last == '$')
      return name;
    const auto alias = const_alias_.find(name);
    if (alias != const_alias_.end())
      return alias->second;
    const int index = static_cast<unsigned char>(name[0]) - 'A';
    const char suffix = (index >= 0 && index < 26) ? default_type_[index] : '!';
    return name + suffix;
  }

  static std::string NormalizeLabel(const std::string& name) {
    std::string out = name;
    for (char& c : out) {
      if (c >= 'a' && c <= 'z')
        c = static_cast<char>(c - 'a' + 'A');
    }
    return out;
  }

  static bool IsLabelName(const std::string& name) {
    if (name.empty())
      return false;
    const char last = name.back();
    return last != '%' && last != '&' && last != '!' && last != '#' && last != '$';
  }

  static bool HasTypeSuffix(const std::string& name) {
    if (name.empty())
      return false;
    const char last = name.back();
    return last == '%' || last == '&' || last == '!' || last == '#' || last == '$';
  }

  static std::string StripTypeSuffix(std::string name) {
    if (HasTypeSuffix(name))
      name.pop_back();
    return name;
  }

  // A procedure name and every variable/array name with the same base collide.
  // This catches `SUB Foo` used as `Foo%`, and `FUNCTION Foo%` used as `Foo`.
  bool ProcedureBaseExists(const std::string& name) const {
    return procedure_bases_.count(NormalizeLabel(StripTypeSuffix(name))) != 0;
  }

  bool IsCurrentFunctionResult(const std::string& name) const {
    if (current_proc_ < 0 ||
        static_cast<std::size_t>(current_proc_) >= out_->procedures_.size())
      return false;
    const Procedure& proc = out_->procedures_[static_cast<std::size_t>(current_proc_)];
    return proc.kind == ProcKind::Function && proc.result_name == name;
  }

  // CONST name = literal. The value is converted to the name's type when the
  // program is compiled; the VM installs it before the first instruction.
  bool ParseConstDecl(const std::vector<Token>& tokens, std::size_t index, int line,
                      bool prescan, std::size_t* next) {
    if (index + 3 >= tokens.size() || tokens[index].kind != Tok::Keyword ||
        tokens[index].text != "CONST") {
      error_->line = line;
      error_->code = kErrSyntax;
      error_->message = "Expected CONST name = value";
      return false;
    }
    const Token& name_token = tokens[index + 1];
    if (name_token.kind != Tok::Ident || name_token.text.empty()) {
      error_->line = line;
      error_->column = name_token.pos;
      error_->code = kErrSyntax;
      error_->message = "Expected a constant name after CONST";
      return false;
    }
    const Token& eq = tokens[index + 2];
    if (eq.kind != Tok::Op || eq.text != "=") {
      error_->line = line;
      error_->column = eq.pos;
      error_->code = kErrSyntax;
      error_->message = "Expected '=' after the constant name";
      return false;
    }

    std::size_t value_index = index + 3;
    bool negative = false;
    bool had_sign = false;
    if (tokens[value_index].kind == Tok::Op &&
        (tokens[value_index].text == "-" || tokens[value_index].text == "+")) {
      negative = tokens[value_index].text == "-";
      had_sign = true;
      ++value_index;
      if (value_index >= tokens.size()) {
        error_->line = line;
        error_->code = kErrSyntax;
        error_->message = "Expected a constant value";
        return false;
      }
    }
    const Token& value_token = tokens[value_index];
    if (value_token.kind == Tok::String && had_sign) {
      error_->line = line;
      error_->column = value_token.pos;
      error_->code = kErrSyntax;
      error_->message = "A string constant cannot have a sign";
      return false;
    }
    if (value_token.kind != Tok::Number && value_token.kind != Tok::String) {
      error_->line = line;
      error_->column = value_token.pos;
      error_->code = kErrSyntax;
      error_->message = "CONST requires a literal number or string";
      return false;
    }
    const std::size_t after = value_index + 1;
    if (after < tokens.size()) {
      const Token& tail = tokens[after];
      const bool end = tail.kind == Tok::EndOfLine ||
                       (tail.kind == Tok::Op &&
                        (tail.text == ":" || tail.text == "'"));
      if (!end) {
        error_->line = line;
        error_->column = tail.pos;
        error_->code = kErrSyntax;
        error_->message = "CONST requires a literal number or string";
        return false;
      }
    }

    const std::string raw = name_token.text;
    char suffix = raw.back();
    std::string base = raw;
    if (suffix == '%' || suffix == '&' || suffix == '!' || suffix == '#' || suffix == '$') {
      base.pop_back();
    } else {
      suffix = '\0';
    }
    if (base.empty()) {
      error_->line = line;
      error_->column = name_token.pos;
      error_->code = kErrSyntax;
      error_->message = "Expected a constant name";
      return false;
    }

    Value value;
    if (suffix == '\0') {
      if (value_token.kind == Tok::String)
        suffix = '$';
      else {
        switch (value_token.number.kind()) {
        case Value::Kind::Int:
          suffix = '%';
          break;
        case Value::Kind::Long:
          suffix = '&';
          break;
        case Value::Kind::Dbl:
          suffix = '#';
          break;
        default:
          suffix = '!';
          break;
        }
      }
    }

    if (suffix == '$') {
      if (value_token.kind != Tok::String) {
        error_->line = line;
        error_->column = value_token.pos;
        error_->code = kErrTypeMismatch;
        error_->message = "Type mismatch";
        return false;
      }
      value = Value::Str(value_token.str);
    } else {
      if (value_token.kind == Tok::String) {
        error_->line = line;
        error_->column = value_token.pos;
        error_->code = kErrTypeMismatch;
        error_->message = "Type mismatch";
        return false;
      }
      double number = value_token.number.AsNumber();
      if (negative)
        number = -number;
      if (suffix == '%' || suffix == '&') {
        if (number < -2147483648.0 || number > 2147483647.0) {
          error_->line = line;
          error_->column = value_token.pos;
          error_->code = kErrOverflow;
          error_->message = "Overflow";
          return false;
        }
        const long long rounded = std::llround(number);
        if (rounded < -2147483648LL || rounded > 2147483647LL) {
          error_->line = line;
          error_->column = value_token.pos;
          error_->code = kErrOverflow;
          error_->message = "Overflow";
          return false;
        }
        if (suffix == '%')
          value = Value::Int(static_cast<std::int32_t>(rounded));
        else
          value = Value::Long(static_cast<std::int32_t>(rounded));
      } else if (suffix == '#') {
        value = Value::Dbl(number);
      } else {
        value = Value::Sgl(number);
      }
    }

    const std::string canonical = NormalizeLabel(base) + suffix;
    const auto alias = const_alias_.find(base);
    if (alias != const_alias_.end()) {
      if (prescan || alias->second != canonical) {
        error_->line = line;
        error_->column = name_token.pos;
        error_->code = kErrSyntax;
        error_->message = "Duplicate constant " + base;
        return false;
      }
    } else {
      const_alias_[base] = canonical;
      out_->constants_.push_back(ConstDef{canonical, value, line});
    }
    const_names_.insert(canonical);
    if (next != nullptr)
      *next = after;
    return true;
  }

  bool BuildConsts(const LineStore& store) {
    for (const SourceLine& line : store.Lines()) {
      std::vector<Token> tokens;
      std::string error;
      int pos = 0;
      if (!Lexer::Tokenize(line.source, &tokens, &error, &pos))
        continue; // CompileSource reports the lexical error with position
      if (tokens.empty() || tokens[0].kind != Tok::Keyword || tokens[0].text != "CONST")
        continue;
      if (!ParseConstDecl(tokens, 0, line.line_number, true, nullptr))
        return false;
    }
    return true;
  }

  // Labels are collected before compilation so forward references resolve.
  // Only a LABEL that is the first statement on a line is pre-scanned; that is
  // the documented form and keeps the scan independent of the statement parser.
  bool BuildLabels(const LineStore& store) {
    for (const SourceLine& line : store.Lines()) {
      std::vector<Token> tokens;
      std::string error;
      int pos = 0;
      if (!Lexer::Tokenize(line.source, &tokens, &error, &pos))
        continue; // CompileSource reports the lexical error with position
      if (tokens.size() < 2 || tokens[0].kind != Tok::Keyword || tokens[0].text != "LABEL")
        continue;
      if (tokens[1].kind != Tok::Ident || !IsLabelName(tokens[1].text)) {
        error_->line = line.line_number;
        error_->column = tokens[1].pos;
        error_->code = kErrSyntax;
        error_->message = "Expected a label name after LABEL";
        return false;
      }
      const std::string name = NormalizeLabel(tokens[1].text);
      const auto existing = labels_.find(name);
      if (existing != labels_.end()) {
        error_->line = line.line_number;
        error_->column = tokens[1].pos;
        error_->code = kErrSyntax;
        error_->message = "Duplicate label " + name;
        return false;
      }
      labels_[name] = line.line_number;
    }
    return true;
  }

  // Collect SUB/FUNCTION headers and their matching END lines before any body
  // is compiled. Forward CALLs and function calls need the procedure table.
  bool BuildProcedures(const LineStore& store) {
    int open = -1;
    for (const SourceLine& line : store.Lines()) {
      std::vector<Token> tokens;
      std::string error;
      int pos = 0;
      if (!Lexer::Tokenize(line.source, &tokens, &error, &pos))
        continue; // CompileSource reports the lexical error with position
      if (tokens.empty() || tokens[0].kind == Tok::EndOfLine)
        continue;
      if (tokens[0].kind == Tok::Keyword &&
          (tokens[0].text == "SUB" || tokens[0].text == "FUNCTION")) {
        if (open >= 0) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "SUB/FUNCTION cannot be nested";
          return false;
        }
        const bool function = tokens[0].text == "FUNCTION";
        if (tokens.size() < 2 || tokens[1].kind != Tok::Ident) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "Expected a procedure name";
          return false;
        }
        const std::string name = NormalizeLabel(tokens[1].text);
        if (function && !HasTypeSuffix(name)) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "FUNCTION name requires a type suffix";
          return false;
        }
        if (!function && HasTypeSuffix(name)) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "SUB name cannot have a type suffix";
          return false;
        }
        const std::string base = StripTypeSuffix(name);
        if (procedure_bases_.count(base) != 0) {
          error_->line = line.line_number;
          error_->code = kErrDuplicateDefinition;
          error_->message = "Duplicate procedure " + base;
          return false;
        }
        for (const std::string& constant : const_names_) {
          if (StripTypeSuffix(constant) == base) {
            error_->line = line.line_number;
            error_->code = kErrDuplicateDefinition;
            error_->message = "Procedure name collides with CONST " + constant;
            return false;
          }
        }
        Procedure proc;
        proc.name = name;
        proc.kind = function ? ProcKind::Function : ProcKind::Sub;
        proc.header_line = line.line_number;
        proc.result_name = function ? name : std::string();
        std::size_t index = 2;
        if (index < tokens.size() && tokens[index].kind == Tok::Op &&
            tokens[index].text == "(") {
          ++index;
          if (index < tokens.size() && tokens[index].kind == Tok::Op &&
              tokens[index].text == ")") {
            ++index;
          } else {
            for (;;) {
              if (index >= tokens.size() || tokens[index].kind != Tok::Ident) {
                error_->line = line.line_number;
                error_->code = kErrSyntax;
                error_->message = "Expected a parameter name";
                return false;
              }
              ProcParam param;
              param.name = NormalizeLabel(tokens[index].text);
              param.kind = Value::Kind::Sgl;
              proc.params.push_back(std::move(param));
              ++index;
              if (index < tokens.size() && tokens[index].kind == Tok::Op &&
                  tokens[index].text == ",") {
                ++index;
                continue;
              }
              if (index >= tokens.size() || tokens[index].kind != Tok::Op ||
                  tokens[index].text != ")") {
                error_->line = line.line_number;
                error_->code = kErrSyntax;
                error_->message = "Expected ')' after procedure parameters";
                return false;
              }
              ++index;
              break;
            }
          }
        }
        if (index < tokens.size() && tokens[index].kind != Tok::EndOfLine &&
            !(tokens[index].kind == Tok::Op && tokens[index].text == "'")) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "Procedure header must be alone on its line";
          return false;
        }
        const int proc_index = static_cast<int>(out_->procedures_.size());
        out_->procedures_.push_back(std::move(proc));
        out_->procedure_index_[name] = proc_index;
        procedure_bases_[base] = proc_index;
        open = proc_index;
        continue;
      }
      if (tokens[0].kind == Tok::Keyword && tokens[0].text == "END" &&
          tokens.size() >= 2 && tokens[1].kind == Tok::Keyword &&
          (tokens[1].text == "SUB" || tokens[1].text == "FUNCTION")) {
        if (open < 0) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "END " + tokens[1].text + " without an open procedure";
          return false;
        }
        const bool function = tokens[1].text == "FUNCTION";
        if ((function ? ProcKind::Function : ProcKind::Sub) !=
            out_->procedures_[static_cast<std::size_t>(open)].kind) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "Procedure END does not match its header";
          return false;
        }
        if (tokens.size() > 2 && tokens[2].kind != Tok::EndOfLine &&
            !(tokens[2].kind == Tok::Op && tokens[2].text == "'")) {
          error_->line = line.line_number;
          error_->code = kErrSyntax;
          error_->message = "END procedure must be alone on its line";
          return false;
        }
        out_->procedures_[static_cast<std::size_t>(open)].end_line = line.line_number;
        open = -1;
      }
    }
    if (open >= 0) {
      error_->line =
          out_->procedures_[static_cast<std::size_t>(open)].header_line;
      error_->code = kErrSyntax;
      error_->message = "SUB/FUNCTION without END";
      return false;
    }
    return true;
  }

  bool RejectConstWrite(const std::string& name) {
    if (const_names_.count(name) == 0)
      return false;
    Error(kErrIllegalFunctionCall, "Cannot assign to constant " + name);
    return true; // the caller must stop compiling
  }

  bool CompileStore(const LineStore& store) {
    if (!BuildConsts(store))
      return false;
    if (!BuildLabels(store))
      return false;
    if (!BuildProcedures(store))
      return false;
    for (const SourceLine& line : store.Lines()) {
      const int begin = Pc();
      line_starts_[line.line_number] = begin;
      if (!CompileSource(line.line_number, line.source))
        return false;
      const int end = Pc() - 1;
      if (end >= begin)
        line_spans_.push_back(LineSpan{line.line_number, begin, end});
    }
    return Finish();
  }

  bool CompileDirectSource(const std::string& source) {
    const int begin = Pc();
    if (!CompileSource(0, source))
      return false;
    const int end = Pc() - 1;
    if (end >= begin)
      line_spans_.push_back(LineSpan{0, begin, end});
    return Finish();
  }

  const std::vector<LineSpan>& LineSpans() const { return line_spans_; }

private:
  // SELECT CASE compilation state. Declared early because the inline compilation
  // methods below instantiate it.
  struct SelectEntry {
    int line = 0;
    int push_index = -1;         // SelectPush instruction, patched with the end line
    int next_case_jump = -1;     // no-match jump to the next CASE/END SELECT
    std::vector<int> exit_jumps; // successful case bodies jump to END SELECT
    bool body_open = false;
    bool has_else = false;
  };

  // ---------------------------------------------------------------- tokens
  const Token& Cur() const { return tokens_[pos_]; }

  void Advance() {
    if (pos_ + 1 < tokens_.size())
      ++pos_;
  }

  bool IsOp(const char* text) const { return Cur().kind == Tok::Op && Cur().text == text; }
  bool IsKeyword(const char* text) const {
    return Cur().kind == Tok::Keyword && Cur().text == text;
  }
  bool TakeOp(const char* text) {
    if (IsOp(text)) {
      Advance();
      return true;
    }
    return false;
  }
  bool TakeKeyword(const char* text) {
    if (IsKeyword(text)) {
      Advance();
      return true;
    }
    return false;
  }
  // Some OPEN words (OUTPUT/APPEND/AS) are not lexer keywords, so accept an
  // identifier with the same text too.
  bool TakeWord(const char* text) {
    if (IsKeyword(text) || (Cur().kind == Tok::Ident && Cur().text == text)) {
      Advance();
      return true;
    }
    return false;
  }

  // GET/PUT name an array without subscripts, e.g. GET (0,0)-(10,10), A%.
  bool ParseArrayName(std::string* out) {
    if (Cur().kind != Tok::Ident)
      return Syntax("Expected an array name");
    *out = Cur().text;
    Advance();
    return true;
  }

  // PUT's optional drawing action: PSET (the default), PRESET, AND, OR or XOR.
  int ParsePutAction() {
    if (TakeWord("PSET"))
      return 0;
    if (TakeWord("PRESET"))
      return 1;
    if (TakeWord("AND"))
      return 2;
    if (TakeWord("OR"))
      return 3;
    if (TakeWord("XOR"))
      return 4;
    Syntax("Expected PSET, PRESET, AND, OR or XOR");
    return 0;
  }

  bool ExpectOp(const char* text) {
    if (TakeOp(text))
      return true;
    return Syntax(std::string("Expected '") + text + "'");
  }
  // End of the current statement (end of line or a comment).
  bool AtStatementEnd() const {
    return Cur().kind == Tok::EndOfLine || (Cur().kind == Tok::Op && Cur().text == "'");
  }
  // End of a PRINT/INPUT item list: also stops at ':' (statement separator)
  // and at ELSE so `IF x THEN PRINT 1 ELSE PRINT 2` parses.
  bool AtListEnd() const {
    if (AtStatementEnd())
      return true;
    if (Cur().kind == Tok::Op && Cur().text == ":")
      return true;
    return Cur().kind == Tok::Keyword && Cur().text == "ELSE";
  }
  void SkipToStatementEnd() {
    int depth = 0;
    while (Cur().kind != Tok::EndOfLine) {
      if (Cur().kind == Tok::Op) {
        if (Cur().text == "(")
          ++depth;
        else if (Cur().text == ")")
          --depth;
        else if (Cur().text == ":" && depth <= 0)
          return;
        else if (Cur().text == "'")
          return;
      }
      Advance();
    }
  }
  void SkipToLineEnd() {
    while (Cur().kind != Tok::EndOfLine)
      Advance();
  }

  // ---------------------------------------------------------------- output
  int Pc() const { return static_cast<int>(out_->instrs_.size()); }

  void Emit(Op op) {
    Instr in;
    in.op = op;
    out_->instrs_.push_back(in);
  }
  int EmitPlaceholder(Op op) {
    const int index = Pc();
    Emit(op);
    return index;
  }
  void EmitPush(const Value& value) {
    Instr in;
    in.op = PushOpForKind(value.kind());
    in.value = value;
    out_->instrs_.push_back(in);
  }
  void EmitPushNumber(double value) { EmitPush(Value::Sgl(value)); }
  void PatchTarget(int index, int target) {
    out_->instrs_[static_cast<std::size_t>(index)].a = target;
  }
  void AddLineFixup(int instr_index, int line, int owner_proc = -2,
                    bool boundary_exempt = false, bool main_only = false) {
    Fixup fixup;
    fixup.instr_index = instr_index;
    fixup.line = line;
    fixup.owner_proc = owner_proc == -2 ? current_proc_ : owner_proc;
    fixup.boundary_exempt = boundary_exempt;
    fixup.main_only = main_only;
    fixup.source_line = line_;
    fixups_.push_back(fixup);
  }

  void AddLabelFixup(int instr_index, std::string label, int owner_proc = -2,
                     bool boundary_exempt = false, bool main_only = false) {
    Fixup fixup;
    fixup.instr_index = instr_index;
    fixup.label = std::move(label);
    fixup.owner_proc = owner_proc == -2 ? current_proc_ : owner_proc;
    fixup.boundary_exempt = boundary_exempt;
    fixup.main_only = main_only;
    fixup.source_line = line_;
    fixups_.push_back(fixup);
  }

  // ---------------------------------------------------------------- errors
  bool Syntax(const std::string& message) {
    error_->line = line_;
    error_->column = Cur().pos;
    error_->code = kErrSyntax;
    error_->message = message;
    return false;
  }
  bool Error(int code, const std::string& message) {
    error_->line = line_;
    error_->column = Cur().pos;
    error_->code = code;
    error_->message = message;
    return false;
  }
  bool UnsupportedStatement(const std::string& name) {
    Instr in;
    in.op = Op::Unsupported;
    in.name = name;
    out_->instrs_.push_back(in);
    SkipToStatementEnd();
    return true;
  }

  // ---------------------------------------------------------------- lines
  bool CompileSource(int line, const std::string& source) {
    line_ = line;
    std::string lex_error;
    int lex_pos = 0;
    if (!Lexer::Tokenize(source, &tokens_, &lex_error, &lex_pos)) {
      error_->line = line;
      error_->column = lex_pos;
      error_->code = kErrSyntax;
      error_->message = lex_error;
      return false;
    }
    pos_ = 0;
    source_ = &source;
    return CompileStatementList(false);
  }

  bool CompileStatementList(bool stop_at_else) {
    for (;;) {
      if (Cur().kind == Tok::EndOfLine)
        return true;
      if (stop_at_else && IsKeyword("ELSE"))
        return true;
      if (IsOp("'"))
        return true; // comment ends the line
      out_->statement_pcs_.push_back(Pc());
      if (!CompileStatement())
        return false;
      if (TakeOp(":"))
        continue;
      if (Cur().kind == Tok::EndOfLine)
        return true;
      if (stop_at_else && IsKeyword("ELSE"))
        return true;
      if (IsOp("'"))
        return true;
      return Syntax("Expected ':' or end of statement");
    }
  }

  bool CompileStatement() {
    if (IsOp("'"))
      return true;
    if (current_proc_ >= 0 && Cur().kind == Tok::Keyword) {
      const std::string& kw = Cur().text;
      if (kw != "LOCAL" && kw != "REM" && kw != "LABEL" && kw != "CONST" &&
          kw != "DATA" && kw != "SUB" && kw != "FUNCTION")
        proc_has_executable_ = true;
    } else if (current_proc_ >= 0) {
      proc_has_executable_ = true;
    }
    if (Cur().kind == Tok::Ident)
      return CompileLet();
    if (IsOp("?")) {
      Advance();
      return CompilePrint();
    }
    if (Cur().kind != Tok::Keyword)
      return Syntax("Expected a statement");

    const std::string keyword = Cur().text;
    if (direct_ && IsIllegalDirect(keyword)) {
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    }
    if (keyword == "LET") {
      Advance();
      return CompileLet();
    }
    if (keyword == "PRINT") {
      Advance();
      return CompilePrint();
    }
    if (keyword == "INPUT") {
      Advance();
      return CompileInput(false);
    }
    if (keyword == "LINE") {
      // LINE [[STEP](x1,y1)]-[STEP](x2,y2)[,[colour][,B[F]]]. This branch runs
      // before the general graphics statements because LINE also starts LINE INPUT.
      // An omitted first coordinate means "from the last referenced point", which the
      // VM fills in from its own tracking.
      Advance();
      if (TakeKeyword("INPUT"))
        return CompileInput(true);
      int flags = 0; // 1 first given, 2 first relative, 4 second relative, 8 B, 16 BF,
                     // 32 a style mask was given
      int style_mask = 0;
      if (!IsOp("-")) {
        if (TakeKeyword("STEP"))
          flags |= 2;
        if (!ExpectOp("("))
          return false;
        if (!ParseExpression()) // x1
          return false;
        if (!TakeOp(","))
          return Syntax("Expected ',' between LINE coordinates");
        if (!ParseExpression()) // y1
          return false;
        if (!ExpectOp(")"))
          return false;
        flags |= 1;
      } else {
        // The omitted coordinate still occupies its two stack slots.
        EmitPush(Value::Int(0));
        EmitPush(Value::Int(0));
      }
      if (!TakeOp("-"))
        return Syntax("Expected '-' in LINE");
      if (TakeKeyword("STEP"))
        flags |= 4;
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x2
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between LINE coordinates");
      if (!ParseExpression()) // y2
        return false;
      if (!ExpectOp(")"))
        return false;
      // Optional colour, then B or BF for a box. An empty slot (,,B) is skipped.
      bool colour_given = false;
      while (TakeOp(",")) {
        if (AtListEnd() || IsOp(",") || IsOp(":"))
          continue;
        // B and BF are words, not lexer keywords, so they need TakeWord: otherwise
        // they lex as variables and the box silently becomes a line.
        if (TakeWord("BF")) {
          flags |= 16;
          continue;
        }
        if (TakeWord("B")) {
          flags |= 8;
          continue;
        }
        if (!colour_given && (flags & (8 | 16)) == 0) {
          if (!ParseExpression()) // colour
            return false;
          colour_given = true;
          continue;
        }
        // A style mask: the reference's 16-bit pattern, which needs a constant at
        // compile time because the VM carries it in the instruction.
        if (Cur().kind == Tok::Number && (flags & 32) == 0) {
          const double style = Cur().number.AsNumber();
          Advance();
          flags |= 32;
          style_mask = static_cast<int>(std::lround(style)) & 0xFFFF;
          continue;
        }
        return Syntax("LINE style must be a constant mask");
      }
      if (!colour_given)
        EmitPush(Value::Int(1));
      Instr line_in;
      line_in.op = Op::Line;
      line_in.a = flags;
      line_in.b = style_mask;
      out_->instrs_.push_back(line_in);
      return true;
    }
    if (keyword == "IF") {
      Advance();
      return CompileIf();
    }
    if (keyword == "FOR") {
      Advance();
      return CompileFor();
    }
    if (keyword == "NEXT") {
      Advance();
      return CompileNext();
    }
    if (keyword == "WHILE") {
      Advance();
      return CompileWhile();
    }
    if (keyword == "WEND") {
      Advance();
      return CompileWend();
    }
    if (keyword == "CONST") {
      return CompileConst();
    }
    if (keyword == "LABEL") {
      Advance();
      return CompileLabel();
    }
    if (keyword == "SUB" || keyword == "FUNCTION") {
      return CompileProcedureHeader(keyword == "FUNCTION");
    }
    if (keyword == "LOCAL") {
      Advance();
      return CompileLocal();
    }
    if (keyword == "CALL") {
      Advance();
      return CompileCall();
    }
    if (keyword == "EXIT") {
      Advance();
      return CompileExit();
    }
    if (keyword == "GOTO") {
      Advance();
      return CompileGotoTarget();
    }
    if (keyword == "GOSUB") {
      Advance();
      return CompileGosubTarget();
    }
    if (keyword == "RETURN") {
      Advance();
      Emit(Op::Return);
      return true;
    }
    if (keyword == "PUSH") {
      Advance();
      if (!TakeKeyword("VIEW"))
        return Syntax("Expected VIEW after PUSH");
      Emit(Op::PushView);
      return true;
    }
    if (keyword == "POP") {
      Advance();
      if (!TakeKeyword("VIEW"))
        return Syntax("Expected VIEW after POP");
      Emit(Op::PopView);
      return true;
    }
    if (keyword == "STACK") {
      // STACK PUSH expr[, expr]... / STACK POP var[, var]... /
      // STACK TOP var[, var]... / STACK PEEK var[, var]... /
      // STACK PEEK index, var / STACK DROP [count]
      Advance();
      if (TakeWord("PUSH")) {
        const std::int32_t start = static_cast<std::int32_t>(out_->stack_sources_.size());
        int count = 0;
        do {
          if (AtWholeArrayRef()) {
            StackSource source;
            source.name = TypeName(Cur().text);
            source.whole_array = true;
            Advance(); // name
            Advance(); // (
            Advance(); // )
            out_->stack_sources_.push_back(std::move(source));
          } else {
            if (!ParseExpression())
              return false;
            out_->stack_sources_.push_back(StackSource{});
          }
          ++count;
        } while (TakeOp(","));
        Instr in;
        in.op = Op::StackPush;
        in.a = start;
        in.b = count;
        out_->instrs_.push_back(in);
        return true;
      }
      if (TakeWord("DROP")) {
        if (AtListEnd()) {
          EmitPush(Value::Int(1));
        } else if (!ParseExpression()) {
          return false;
        }
        Emit(Op::StackDrop);
        return true;
      }
      bool pop = false;
      bool peek = false;
      if (TakeWord("POP")) {
        pop = true;
      } else if (TakeWord("PEEK")) {
        peek = true;
      } else if (!TakeWord("TOP")) {
        return Syntax("Expected PUSH, POP, TOP, PEEK, or DROP after STACK");
      }
      // A numeric or parenthesized first expression is an indexed PEEK. A
      // variable first argument remains the existing top-down target list, so
      // STACK PEEK B, A keeps its original meaning.
      const bool indexed_peek =
          peek &&
          (Cur().kind == Tok::Number || IsOp("(") || IsOp("+") || IsOp("-") ||
           (Cur().kind == Tok::Keyword &&
            (Cur().text == "STACKAT" || Cur().text == "STACKDEPTH")));
      if (indexed_peek) {
        if (!ParseExpression())
          return false;
        if (!TakeOp(","))
          return Syntax("Expected ',' after the STACK PEEK index");
        const std::int32_t start = static_cast<std::int32_t>(out_->stack_targets_.size());
        if (!ParseStackTarget(true))
          return false;
        if (TakeOp(","))
          return Syntax("Indexed STACK PEEK takes one target");
        Instr in;
        in.op = Op::StackPeekAt;
        in.a = start;
        in.b = 1;
        out_->instrs_.push_back(in);
        return true;
      }
      const std::int32_t start = static_cast<std::int32_t>(out_->stack_targets_.size());
      int count = 0;
      do {
        if (!ParseStackTarget(true)) // STACK POP and TOP both assign
          return false;
        ++count;
      } while (TakeOp(","));
      Instr in;
      in.op = pop ? Op::StackPop : Op::StackTop;
      in.a = start;
      in.b = count;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "SLEEP") {
      Advance();
      if (!ParseExpression())
        return false;
      Emit(Op::Sleep);
      return true;
    }
    if (keyword == "SELECT") {
      Advance();
      return CompileSelectCase();
    }
    if (keyword == "CASE") {
      Advance();
      return CompileCase();
    }
    if (keyword == "ON") {
      Advance();
      return CompileOn();
    }
    if (keyword == "END") {
      if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].kind == Tok::Keyword &&
          tokens_[pos_ + 1].text == "SELECT") {
        Advance(); // END
        Advance(); // SELECT
        return CompileEndSelect();
      }
      if (pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].kind == Tok::Keyword &&
          (tokens_[pos_ + 1].text == "SUB" || tokens_[pos_ + 1].text == "FUNCTION")) {
        const bool function_end = tokens_[pos_ + 1].text == "FUNCTION";
        Advance(); // END
        Advance(); // SUB/FUNCTION
        return CompileProcedureEnd(function_end);
      }
      Advance();
      Emit(Op::End);
      return true;
    }
    if (keyword == "STOP") {
      Advance();
      Emit(Op::Stop);
      return true;
    }
    if (keyword == "REM") {
      Advance();
      SkipToLineEnd();
      return true;
    }
    if (keyword == "FIELD") {
      // FIELD #n, width AS var$ [, width AS var$]...
      Advance();
      Instr in;
      in.op = Op::Field;
      if (!TakeOp("#"))
        return Syntax("Expected '#' after FIELD");
      if (!ParseExpression())
        return false; // file number
      if (!TakeOp(","))
        return Syntax("Expected ',' after the file number in FIELD");
      std::string list;
      for (;;) {
        if (Cur().kind != Tok::Number)
          return UnsupportedStatement("FIELD with a computed width");
        const long width = static_cast<long>(Cur().number.AsNumber());
        Advance();
        if (!TakeWord("AS"))
          return Syntax("Expected AS in FIELD");
        std::string name;
        bool array = false;
        int subscripts = 0;
        if (!ParseVariableRef(&name, &array, &subscripts))
          return false;
        if (RejectConstWrite(name))
          return false;
        if (array)
          return UnsupportedStatement("FIELD of an array element");
        if (!list.empty())
          list += ',';
        list += name + ':' + std::to_string(width);
        if (!TakeOp(","))
          break;
      }
      in.name = list;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "LSET" || keyword == "RSET") {
      const bool right_justify = keyword == "RSET";
      Advance();
      std::string name;
      bool array = false;
      int subscripts = 0;
      if (!ParseVariableRef(&name, &array, &subscripts))
        return false;
      if (RejectConstWrite(name))
        return false;
      if (array)
        return UnsupportedStatement("LSET/RSET of an array element");
      if (!TakeOp("="))
        return Syntax("Expected '=' in LSET/RSET");
      if (!ParseExpression())
        return false;
      Instr in;
      in.op = Op::LSet;
      in.a = right_justify ? 1 : 0;
      in.name = name;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "GET" || keyword == "PUT") {
      const bool put = keyword == "PUT";
      Advance();
      if (IsOp("(") || IsKeyword("STEP")) {
        // Graphics form (M12): GET (x1,y1)-(x2,y2), array
        //                     PUT (x,y), array [, action]
        if (!put) {
          // GET does not take STEP (its coordinates are a rectangle); accept the token
          // only so a stray STEP reports a clear error rather than a confusing one.
          if (TakeWord("STEP"))
            return Syntax("GET takes absolute coordinates");
          if (!ExpectOp("("))
            return false;
          if (!ParseExpression()) // x1
            return false;
          if (!TakeOp(","))
            return Syntax("Expected ',' between GET coordinates");
          if (!ParseExpression()) // y1
            return false;
          if (!ExpectOp(")"))
            return false;
          if (!TakeOp("-"))
            return Syntax("Expected '-' in GET");
          if (!ExpectOp("("))
            return false;
          if (!ParseExpression()) // x2
            return false;
          if (!TakeOp(","))
            return Syntax("Expected ',' between GET coordinates");
          if (!ParseExpression()) // y2
            return false;
          if (!ExpectOp(")"))
            return false;
          if (!TakeOp(","))
            return Syntax("Expected ',' before the GET array");
          std::string array_name;
          if (!ParseArrayName(&array_name))
            return false;
          if (RejectConstWrite(array_name))
            return false;
          Instr in;
          in.op = Op::GetImage;
          in.name = array_name;
          out_->instrs_.push_back(in);
          return true;
        }
        if (TakeWord("STEP"))
          return Syntax("PUT takes absolute coordinates");
        if (!ExpectOp("("))
          return false;
        if (!ParseExpression()) // x
          return false;
        if (!TakeOp(","))
          return Syntax("Expected ',' between PUT coordinates");
        if (!ParseExpression()) // y
          return false;
        if (!ExpectOp(")"))
          return false;
        if (!TakeOp(","))
          return Syntax("Expected ',' before the PUT array");
        std::string array_name;
        if (!ParseArrayName(&array_name))
          return false;
        Instr in;
        in.op = Op::PutImage;
        in.name = array_name;
        in.a = 0;
        if (TakeOp(","))
          in.a = ParsePutAction(); // PSET by default
        out_->instrs_.push_back(in);
        return true;
      }
      if (!TakeOp("#"))
        return Syntax("Expected '#' or '(' after GET/PUT");
      if (!ParseExpression())
        return false; // file number
      Instr in;
      in.op = put ? Op::PutRecord : Op::GetRecord;
      in.a = 0;
      if (TakeOp(",")) {
        if (!ParseExpression())
          return false;
        in.a = 1;
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "LIST") {
      // LIST [[first][-[last]]]
      Advance();
      Instr in;
      in.op = Op::List;
      in.a = 0;
      in.b = 0;
      if (Cur().kind == Tok::Number) {
        int first = 0;
        if (!LineNumberFromNumber(Cur().number.AsNumber(), &first))
          return Error(kErrIllegalFunctionCall, "Line number out of range");
        Advance();
        in.a = first;
      }
      if (TakeOp("-")) {
        in.b = 1; // a range was given; the end defaults to the last line
        if (Cur().kind == Tok::Number) {
          int last = 0;
          if (!LineNumberFromNumber(Cur().number.AsNumber(), &last))
            return Error(kErrIllegalFunctionCall, "Line number out of range");
          Advance();
          in.b = last;
        }
      } else if (in.a != 0) {
        in.b = in.a; // a single line number means just that line
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "DELETE") {
      // DELETE first[-last] or DELETE line
      Advance();
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number after DELETE");
      int first = 0;
      if (!LineNumberFromNumber(Cur().number.AsNumber(), &first))
        return Error(kErrIllegalFunctionCall, "Line number out of range");
      Advance();
      int last = first;
      if (TakeOp("-")) {
        if (Cur().kind == Tok::Number) {
          if (!LineNumberFromNumber(Cur().number.AsNumber(), &last))
            return Error(kErrIllegalFunctionCall, "Line number out of range");
          Advance();
        } else {
          last = kMaxLineNumber; // DELETE first-  removes to the end
        }
      }
      Instr in;
      in.op = Op::Delete;
      in.a = first;
      in.b = last;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "NEW") {
      Advance();
      Instr in;
      in.op = Op::New;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "SAVE" || keyword == "LOAD") {
      // In-program SAVE/LOAD. GW-BASIC restricts these to direct mode; newbasic
      // accepts them in programs too, which is a superset (recorded in plan.md).
      const bool save = keyword == "SAVE";
      Advance();
      if (!ParseExpression())
        return false; // the file name
      Emit(save ? Op::SaveProgram : Op::LoadProgram);
      return true;
    }
    if (keyword == "CHAIN") {
      // CHAIN "file"[, line]
      Advance();
      if (TakeKeyword("MERGE"))
        return UnsupportedStatement("CHAIN MERGE");
      if (!ParseExpression())
        return false; // the file name
      Instr in;
      in.op = Op::Chain;
      in.a = 0;
      if (TakeOp(",")) {
        if (!ParseExpression())
          return false;
        in.a = 1;
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "COMMON") {
      // COMMON var[, var]: variables that survive a CHAIN.
      Advance();
      std::string list;
      for (;;) {
        std::string name;
        bool array = false;
        int subscripts = 0;
        if (!ParseVariableRef(&name, &array, &subscripts))
          return false;
        if (array)
          return UnsupportedStatement("COMMON of arrays");
        if (!list.empty())
          list += ',';
        list += name;
        if (!TakeOp(","))
          break;
      }
      Instr in;
      in.op = Op::Common;
      in.name = list;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "MERGE") {
      // The reference restricts MERGE to direct mode. Consume the operand so the
      // rest of the line still parses.
      Advance();
      if (!AtListEnd() && !IsOp(":")) {
        if (!ParseExpression())
          return false;
      }
      Instr in;
      in.op = Op::Fail;
      in.a = kErrIllegalDirect;
      in.name = "MERGE is only valid in direct mode";
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "RUN") {
      Advance();
      Instr in;
      in.op = Op::Run;
      in.a = 0;
      if (!AtListEnd() && !IsOp(":")) {
        if (!ParseExpression())
          return false;
        in.a = 1;
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "TRON" || keyword == "TROFF") {
      const bool on = keyword == "TRON";
      Advance();
      Instr in;
      in.op = on ? Op::Tron : Op::Troff;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "AUTO" || keyword == "EDIT") {
      // The reference restricts these to direct mode. EDIT takes an optional line
      // number, so parse it before reporting the restriction.
      const bool edit = keyword == "EDIT";
      Advance();
      if (edit && !AtListEnd() && !IsOp(":")) {
        if (!ParseExpression())
          return false;
      }
      Instr in;
      in.op = Op::Fail;
      in.a = kErrIllegalDirect;
      in.name = keyword + " is only valid in direct mode";
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "CONT") {
      Advance();
      Instr in;
      in.op = Op::Fail;
      in.a = kErrCantContinue;
      in.name = "CONT is only valid after a stop";
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "LLIST") {
      // LLIST: the stored program, sent to the printer (Host::WriteToPrinter).
      Advance();
      while (!AtListEnd() && !IsOp(":"))
        Advance();
      Emit(Op::LList);
      return true;
    }
    if (keyword == "LPRINT") {
      // LPRINT items: PRINT's item list, bound to the printer.
      Advance();
      printer_statement_ = true;
      const bool ok = CompilePrint();
      printer_statement_ = false;
      return ok;
    }
    if (keyword == "RENUM") {
      // The reference makes RENUM a direct-mode command: it renumbers the stored
      // program, which a running program cannot sensibly do to itself.
      Advance();
      while (!AtListEnd() && !IsOp(":"))
        Advance();
      Instr in;
      in.op = Op::Fail;
      in.a = kErrIllegalDirect;
      in.name = "RENUM is only valid in direct mode";
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "CLEAR") {
      // CLEAR: variables, arrays, stacks and files, leaving the program in place.
      // The argument form (string space / stack size) is not modelled.
      if (current_proc_ >= 0)
        return Syntax("CLEAR is not allowed inside a procedure");
      Advance();
      if (!AtListEnd() && !IsOp(":"))
        return UnsupportedStatement("CLEAR with arguments");
      Emit(Op::Clear);
      return true;
    }
    if (keyword == "SCREEN") {
      // SCREEN mode[,[colorswitch][,[apage][,[vpage]]]]: the mode is honoured, the
      // page arguments are accepted and ignored (no page flipping, see
      // docs/m12-graphics-design.md).
      Advance();
      if (!ParseExpression())
        return false;
      while (TakeOp(",")) {
        if (!AtListEnd() && !IsOp(",") && !IsOp(":")) {
          if (!ParseExpression())
            return false;
        }
      }
      Emit(Op::ScreenMode);
      return true;
    }
    if (keyword == "PSET" || keyword == "PRESET") {
      // [STEP](x,y)[,colour]: STEP makes the coordinates relative to the last point.
      const bool reset = keyword == "PRESET";
      Advance();
      const bool relative = TakeKeyword("STEP");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between PSET coordinates");
      if (!ParseExpression()) // y
        return false;
      if (!ExpectOp(")"))
        return false;
      if (TakeOp(",")) {
        if (!ParseExpression()) // colour
          return false;
      } else {
        EmitPush(Value::Int(reset ? 0 : 1));
      }
      Instr in;
      in.op = Op::PSet;
      in.a = relative ? 1 : 0;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "CIRCLE") {
      // CIRCLE [STEP](x,y), radius [, [colour] [, [start] [, end] [, aspect]]]
      Advance();
      const bool circle_relative = TakeKeyword("STEP");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between CIRCLE coordinates");
      if (!ParseExpression()) // y
        return false;
      if (!ExpectOp(")"))
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' before the CIRCLE radius");
      if (!ParseExpression()) // radius
        return false;
      // Optional arguments default to: colour 1, start 0, end 2pi, aspect from the
      // screen mode (the VM works that out).
      double defaults[4] = {1.0, 0.0, 6.283185307179586, 0.0};
      for (int slot = 0; slot < 4; ++slot) {
        if (!TakeOp(",")) {
          for (int rest = slot; rest < 4; ++rest)
            EmitPush(Value::Sgl(defaults[rest]));
          break;
        }
        if (AtListEnd() || IsOp(",") || IsOp(":")) {
          EmitPush(Value::Sgl(defaults[slot]));
          continue;
        }
        if (!ParseExpression())
          return false;
      }
      Instr circle_in;
      circle_in.op = Op::Circle;
      circle_in.a = circle_relative ? 1 : 0;
      out_->instrs_.push_back(circle_in);
      return true;
    }
    if (keyword == "PAINT") {
      // PAINT [STEP](x,y)[, [fill] [, [border]]]. The stack always carries fill then
      // border, so the VM can pop both unconditionally.
      Advance();
      const bool paint_relative = TakeKeyword("STEP");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between PAINT coordinates");
      if (!ParseExpression()) // y
        return false;
      if (!ExpectOp(")"))
        return false;
      const double fill_default = 1.0;
      const double border_default = -1.0; // -1 = stop at a different colour
      if (TakeOp(",") && !AtListEnd() && !IsOp(",") && !IsOp(":")) {
        if (!ParseExpression()) // fill
          return false;
        if (TakeOp(",") && !AtListEnd() && !IsOp(":")) {
          if (!ParseExpression()) // border
            return false;
        } else {
          EmitPush(Value::Sgl(border_default));
        }
      } else {
        EmitPush(Value::Sgl(fill_default));
        EmitPush(Value::Sgl(border_default));
      }
      Instr paint_in;
      paint_in.op = Op::Paint;
      paint_in.a = paint_relative ? 1 : 0;
      out_->instrs_.push_back(paint_in);
      return true;
    }
    if (keyword == "DRAW") {
      // DRAW macro$: the graphics macro language (draw.h).
      Advance();
      if (!ParseExpression())
        return false;
      Emit(Op::Draw);
      return true;
    }
    if (keyword == "DRAWTEXT") {
      // DRAWTEXT [STEP](x,y), text$[, colour][, size]. The default colour is 1
      // (the PSET default) and the default scale is 1.
      Advance();
      const bool relative = TakeKeyword("STEP");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between DRAWTEXT coordinates");
      if (!ParseExpression()) // y
        return false;
      if (!ExpectOp(")"))
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' before the DRAWTEXT string");
      if (!ParseExpression()) // text
        return false;
      // Empty slots (`, ,`) mean "use the default" for colour and size.
      const auto omitted = [&] {
        return Cur().kind == Tok::EndOfLine || IsOp(",") || IsOp(":");
      };
      if (TakeOp(",")) {
        if (omitted())
          EmitPush(Value::Int(1));
        else if (!ParseExpression()) // colour
          return false;
        if (TakeOp(",")) {
          if (omitted())
            EmitPush(Value::Int(1));
          else if (!ParseExpression()) // size
            return false;
        } else {
          EmitPush(Value::Int(1));
        }
      } else {
        EmitPush(Value::Int(1));
        EmitPush(Value::Int(1));
      }
      Instr in;
      in.op = Op::DrawText;
      in.a = relative ? 1 : 0;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "PCOPY") {
      // PCOPY source, destination: page 0 is the displayed canvas (see screen.h).
      Advance();
      if (!ParseExpression()) // source
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between PCOPY pages");
      if (!ParseExpression()) // destination
        return false;
      Emit(Op::Pcopy);
      return true;
    }
    if (keyword == "PALETTE") {
      // PALETTE [attribute, colour] or PALETTE USING integer-array[(index)].
      // No arguments restores the default palette.
      Advance();
      Instr in;
      in.op = Op::Palette;
      in.a = 0;
      if (TakeKeyword("USING")) {
        if (Cur().kind != Tok::Ident)
          return Syntax("Expected an integer array after PALETTE USING");
        in.a = 2;
        in.name = TypeName(Cur().text);
        Advance();
        if (TakeOp("(")) {
          if (!ParseExpression()) // first element to use
            return false;
          if (!ExpectOp(")"))
            return false;
          in.b = 1;
        }
        out_->instrs_.push_back(in);
        return true;
      }
      if (!AtListEnd() && !IsOp(":")) {
        if (!ParseExpression()) // attribute
          return false;
        if (!TakeOp(","))
          return Syntax("Expected ',' between the PALETTE arguments");
        if (!ParseExpression()) // colour
          return false;
        in.a = 1;
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "VIEW") {
      // VIEW PRINT is the text window, handled by the branch further down; only the
      // graphics VIEW form is parsed here.
      Advance();
      if (IsKeyword("PRINT"))
        return CompileViewPrintStatement();
      Instr in;
      in.op = Op::View;
      in.a = 1;
      if (AtListEnd() || IsOp(":")) {
        in.a = 0;
        out_->instrs_.push_back(in);
        return true;
      }
      TakeWord("SCREEN"); // the SCREEN form only changes the y direction of WINDOW
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x1
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between VIEW coordinates");
      if (!ParseExpression()) // y1
        return false;
      if (!ExpectOp(")"))
        return false;
      if (!TakeOp("-"))
        return Syntax("Expected '-' in VIEW");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x2
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between VIEW coordinates");
      if (!ParseExpression()) // y2
        return false;
      if (!ExpectOp(")"))
        return false;
      // Fill and border are optional; -1 means "leave unchanged".
      if (TakeOp(",") && !AtListEnd() && !IsOp(",") && !IsOp(":")) {
        if (!ParseExpression()) // fill
          return false;
        if (TakeOp(",") && !AtListEnd() && !IsOp(":")) {
          if (!ParseExpression()) // border
            return false;
        } else {
          EmitPush(Value::Int(-1));
        }
      } else {
        EmitPush(Value::Int(-1));
        EmitPush(Value::Int(-1));
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "WINDOW") {
      // WINDOW [[SCREEN](x1,y1)-(x2,y2)]; no arguments restores screen coordinates.
      Advance();
      Instr in;
      in.op = Op::Window;
      in.a = 1;
      if (AtListEnd() || IsOp(":")) {
        in.a = 0; // WINDOW with no arguments
        out_->instrs_.push_back(in);
        return true;
      }
      if (TakeWord("SCREEN"))
        in.a = 2; // y grows downward
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x1
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between WINDOW coordinates");
      if (!ParseExpression()) // y1
        return false;
      if (!ExpectOp(")"))
        return false;
      if (!TakeOp("-"))
        return Syntax("Expected '-' in WINDOW");
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // x2
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' between WINDOW coordinates");
      if (!ParseExpression()) // y2
        return false;
      if (!ExpectOp(")"))
        return false;
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "CLS") {
      Advance();
      // CLS [n]: 0 clears the screen, 1 the graphics viewport (there is no
      // graphics viewport, so it clears nothing), 2 only the text window.
      if (AtListEnd()) {
        EmitPush(Value::Int(0));
      } else if (!ParseExpression()) {
        return false;
      }
      Emit(Op::Cls);
      return true;
    }
    if (keyword == "WIDTH") {
      Advance();
      if (Cur().kind == Tok::String)
        return UnsupportedStatement("WIDTH for a device name");
      // WIDTH columns: the text grid narrows and printing wraps at that column.
      if (!ParseExpression())
        return false;
      Emit(Op::Width);
      return true;
    }
    if (keyword == "LOCATE") {
      Advance();
      return CompileLocate();
    }
    if (keyword == "COLOR") {
      Advance();
      return CompileColor();
    }
    if (keyword == "RANDOMIZE") {
      Advance();
      return CompileRandomize();
    }
    if (keyword == "SWAP") {
      Advance();
      return CompileSwap();
    }
    if (keyword == "BEEP") {
      Advance();
      Emit(Op::Beep);
      return true;
    }
    if (keyword == "SOUND") {
      // SOUND frequency, duration - duration is in clock ticks (18.2/second).
      Advance();
      if (!ParseExpression())
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' after the SOUND frequency");
      if (!ParseExpression())
        return false;
      Instr sound_in;
      sound_in.op = Op::Sound;
      out_->instrs_.push_back(sound_in);
      return true;
    }
    if (keyword == "PLAY") {
      // PLAY ON/OFF/STOP arm note-queue trapping; there is no queue (notes are
      // played synchronously), so report that rather than pretend.
      if (IsKeyword("ON") || IsKeyword("OFF") || IsKeyword("STOP")) {
        Advance();
        Instr in;
        in.op = Op::Fail;
        in.a = kErrAdvancedFeature;
        in.name = "PLAY ON/OFF/STOP needs a note queue, which newbasic does not keep";
        out_->instrs_.push_back(in);
        return true;
      }
      // PLAY string-expression: the musical macro language (see sound.h).
      Advance();
      if (!ParseExpression())
        return false;
      Instr play_in;
      play_in.op = Op::Play;
      out_->instrs_.push_back(play_in);
      return true;
    }
    if (keyword == "SYSTEM") {
      Advance();
      Emit(Op::End); // SYSTEM ends the program; the REPL exits the session
      return true;
    }
    if (keyword == "RESUME") {
      Advance();
      return CompileResume();
    }
    if (keyword == "OPEN") {
      Advance();
      return CompileOpen();
    }
    if (keyword == "CLOSE") {
      Advance();
      return CompileClose();
    }
    if (keyword == "WRITE") {
      Advance();
      write_statement_ = true;
      const bool ok = CompilePrint();
      write_statement_ = false;
      return ok;
    }
    if (keyword == "TIMER") {
      // TIMER ON / OFF / STOP (the TIMER function is parsed in expressions).
      Advance();
      int action = 1;
      if (TakeKeyword("OFF")) {
        action = 0;
      } else if (TakeKeyword("STOP")) {
        action = 2;
      } else if (!TakeKeyword("ON")) {
        return Syntax("Expected ON, OFF or STOP after TIMER");
      }
      Instr timer_in;
      timer_in.op = Op::TimerControl;
      timer_in.a = action;
      out_->instrs_.push_back(timer_in);
      return true;
    }
    if (keyword == "SHELL") {
      Advance();
      Instr in;
      in.op = Op::Shell;
      in.b = 0;
      if (!AtListEnd() && !IsOp(":")) {
        if (!ParseExpression())
          return false;
        in.b = 1;
      }
      out_->instrs_.push_back(in);
      return true;
    }
    if (keyword == "ERROR") {
      Advance();
      if (!ParseExpression())
        return false;
      Emit(Op::ErrorStmt);
      return true;
    }
    if (keyword == "DIM") {
      Advance();
      return CompileDim();
    }
    if (keyword == "ERASE") {
      Advance();
      return CompileErase();
    }
    if (keyword == "OPTION") {
      Advance();
      return CompileOption();
    }
    if (keyword == "DEF") {
      Advance();
      return CompileDef();
    }
    if (keyword == "DEFINT" || keyword == "DEFSNG" || keyword == "DEFDBL" || keyword == "DEFSTR") {
      const char suffix = keyword == "DEFINT"
                              ? '%'
                              : (keyword == "DEFSNG" ? '!' : (keyword == "DEFDBL" ? '#' : '$'));
      Advance();
      return CompileDefType(suffix);
    }
    if (keyword == "DATA") {
      Advance();
      return CompileData();
    }
    if (keyword == "READ") {
      Advance();
      return CompileRead();
    }
    if (keyword == "RESTORE") {
      Advance();
      return CompileRestore();
    }
    if (InList(kUnimplementedStatements,
               sizeof(kUnimplementedStatements) / sizeof(kUnimplementedStatements[0]), keyword)) {
      Advance();
      return UnsupportedStatement(keyword);
    }
    return Syntax("Syntax error");
  }

  // ---------------------------------------------------------------- DATA
  // DATA items are stored raw (text plus a parsed value when numeric) in the
  // program, in textual order; READ consumes them, RESTORE rewinds.
  bool CompileData() {
    if (source_ == nullptr)
      return Syntax("Internal error");
    const std::size_t start = static_cast<std::size_t>(Cur().pos);
    const std::string raw = start <= source_->size() ? source_->substr(start) : std::string();
    if (out_->line_data_index_.find(line_) == out_->line_data_index_.end()) {
      out_->line_data_index_[line_] = static_cast<int>(out_->data_items_.size());
    }
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;
    for (char c : raw) {
      if (c == '"') {
        in_quotes = !in_quotes;
        current += c;
        continue;
      }
      if (c == ',' && !in_quotes) {
        fields.push_back(current);
        current.clear();
        continue;
      }
      current += c;
    }
    fields.push_back(current);
    for (std::string& field : fields) {
      // trim
      std::size_t begin = 0;
      std::size_t end = field.size();
      while (begin < end && (field[begin] == ' ' || field[begin] == '\t'))
        ++begin;
      while (end > begin && (field[end - 1] == ' ' || field[end - 1] == '\t'))
        --end;
      field = field.substr(begin, end - begin);

      DataItem item;
      if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
        const std::string inner = field.substr(1, field.size() - 2);
        for (std::size_t i = 0; i < inner.size(); ++i) {
          if (inner[i] == '"' && i + 1 < inner.size() && inner[i + 1] == '"') {
            item.text += '"';
            ++i;
          } else {
            item.text += inner[i];
          }
        }
      } else {
        item.text = field;
      }
      if (item.text.empty()) {
        item.numeric = true;
        item.number = Value::Int(0); // omitted items read as zero
      } else {
        const char* text_begin = item.text.c_str();
        char* text_end = nullptr;
        const double value = std::strtod(text_begin, &text_end);
        if (text_end != text_begin) {
          while (*text_end == ' ' || *text_end == '\t')
            ++text_end;
          if (*text_end == '\0') {
            item.numeric = true;
            item.number = Value::Sgl(value);
          }
        }
      }
      out_->data_items_.push_back(std::move(item));
    }
    SkipToLineEnd();
    Emit(Op::Nop);
    return true;
  }

  bool CompileRead() {
    // One instruction per target keeps DATA order and lets a target be an array element:
    // ParseVariableRef has already pushed the subscript expressions, so the machine pops them
    // and takes the value from the DATA stream in turn.
    do {
      std::string name;
      bool array = false;
      int subscripts = 0;
      if (!ParseVariableRef(&name, &array, &subscripts))
        return false;
      if (RejectConstWrite(name))
        return false;
      Instr in;
      in.op = array ? Op::ReadArray : Op::Read;
      in.name = name;
      in.a = array ? subscripts : 0;
      out_->instrs_.push_back(in);
    } while (TakeOp(","));
    return true;
  }

  // RESUME / RESUME NEXT / RESUME <line>: leave the error handler.
  bool CompileResume() {
    Instr in;
    in.op = Op::Resume;
    if (TakeKeyword("NEXT")) {
      in.b = 1;
      out_->instrs_.push_back(in);
      return true;
    }
    if (Cur().kind == Tok::Number) {
      int target = 0;
      if (!LineNumberFromNumber(Cur().number.AsNumber(), &target))
        return Error(kErrIllegalFunctionCall, "Line number out of range");
      Advance();
      const int index = EmitPlaceholder(Op::Resume);
      out_->instrs_[static_cast<std::size_t>(index)].b = 2;
      AddLineFixup(index, target, current_proc_, true, false);
      return true;
    }
    in.b = 0;
    out_->instrs_.push_back(in);
    return true;
  }

  // OPEN "name" FOR INPUT|OUTPUT|APPEND AS #n and the short form
  // OPEN "I"|"O"|"A", #n, "name".
  bool CompileOpen() {
    Instr in;
    in.op = Op::Open;
    const bool short_form = Cur().kind == Tok::String && pos_ + 1 < tokens_.size() &&
                            tokens_[pos_ + 1].kind == Tok::Op && tokens_[pos_ + 1].text == ",";
    if (short_form) {
      if (!ParseExpression()) // mode string
        return false;
      if (!ExpectOp(","))
        return false;
      if (!ExpectOp("#"))
        return false;
      if (!ParseExpression()) // file number
        return false;
      if (!ExpectOp(","))
        return false;
      if (!ParseExpression()) // file name
        return false;
      in.b = 3;
      out_->instrs_.push_back(in);
      return true;
    }
    if (!ParseExpression()) // file name
      return false;
    if (!TakeKeyword("FOR"))
      return Syntax("Expected FOR");
    if (TakeWord("INPUT")) {
      in.b = 0;
    } else if (TakeWord("OUTPUT") || (TakeKeyword("OUT") && TakeWord("PUT"))) {
      // "OUTPUT" lexes as OUT + PUT (the keyword table has OUT).
      in.b = 1;
    } else if (TakeWord("APPEND")) {
      in.b = 2;
    } else if (TakeWord("RANDOM")) {
      in.b = 4;
    } else {
      return Syntax("Expected INPUT, OUTPUT, APPEND or RANDOM");
    }
    if (!TakeWord("AS"))
      return Syntax("Expected AS");
    if (!ExpectOp("#"))
      return false;
    if (!ParseExpression()) // file number
      return false;
    if (in.b == 4 && TakeWord("LEN")) {
      // OPEN "f" FOR RANDOM AS #1 LEN=32: the record length follows the number.
      if (!TakeOp("="))
        return Syntax("Expected '=' after LEN");
      if (!ParseExpression())
        return false;
      in.a = 1;
    }
    out_->instrs_.push_back(in);
    return true;
  }

  bool CompileClose() {
    Instr in;
    in.op = Op::Close;
    in.b = 0;
    if (TakeOp("#")) {
      if (!ParseExpression())
        return false;
      in.b = 1;
    }
    out_->instrs_.push_back(in);
    return true;
  }

  bool CompileRestore() {
    Instr in;
    in.op = Op::Restore;
    in.a = 0;
    if (Cur().kind == Tok::Number) {
      int target = 0;
      if (!LineNumberFromNumber(Cur().number.AsNumber(), &target))
        return Error(kErrIllegalFunctionCall, "Line number out of range");
      in.a = target;
      Advance();
    }
    out_->instrs_.push_back(in);
    return true;
  }

  // ---------------------------------------------------------------- types
  // DEFINT/DEFSNG/DEFDBL/DEFSTR: default storage type per initial letter.
  // Applied in program order (an approximation of GW-BASIC's entry-order
  // semantics) and compiled away, since types are fixed at compile time.
  bool CompileDefType(char suffix) {
    for (;;) {
      if (Cur().kind != Tok::Ident || Cur().text.size() != 1)
        return Syntax("Expected a letter");
      char first = Cur().text[0];
      if (first >= 'a' && first <= 'z')
        first = static_cast<char>(first - 'a' + 'A');
      Advance();
      char last = first;
      if (TakeOp("-")) {
        if (Cur().kind != Tok::Ident || Cur().text.size() != 1)
          return Syntax("Expected a letter");
        last = Cur().text[0];
        if (last >= 'a' && last <= 'z')
          last = static_cast<char>(last - 'a' + 'A');
        Advance();
      }
      if (first < 'A' || first > 'Z' || last < first || last > 'Z')
        return Syntax("Expected a letter range");
      for (char c = first; c <= last; ++c)
        default_type_[c - 'A'] = suffix;
      if (!TakeOp(","))
        break;
    }
    Emit(Op::Nop);
    return true;
  }

  int FindUserFunc(const std::string& name) const {
    for (std::size_t i = out_->user_funcs_.size(); i-- > 0;) {
      if (out_->user_funcs_[i].name == name)
        return static_cast<int>(i);
    }
    return -1; // the last definition wins
  }

  // DEF FN<name>[(params)] = expression: the body is compiled inline and
  // skipped at run time; DEF FN must appear before its calls (GW-BASIC
  // raises "Undefined user function" instead, which newbasic reports at
  // compile time).
  bool CompileDef() {
    // Whole-word lexing makes `FNA` one identifier. Accept both the classic
    // `DEF FN A(...)` spelling and the compact `DEF FNA(...)` spelling.
    std::string name;
    if (IsKeyword("FN")) {
      Advance();
      if (Cur().kind != Tok::Ident)
        return Syntax("Expected a function name after DEF FN");
      name = "FN" + TypeName(Cur().text);
      Advance();
    } else if (Cur().kind == Tok::Ident && Cur().text.size() > 2 &&
               Cur().text.compare(0, 2, "FN") == 0) {
      name = TypeName(Cur().text);
      Advance();
    } else {
      return Syntax("Expected FN after DEF");
    }
    if (current_proc_ >= 0)
      return Syntax("DEF FN is not allowed inside a procedure");
    if (ProcedureBaseExists(name))
      return Syntax("DEF FN name collides with a procedure");
    std::vector<std::string> params;
    if (TakeOp("(")) {
      if (!IsOp(")")) {
        do {
          if (Cur().kind != Tok::Ident)
            return Syntax("Expected a parameter name");
          params.push_back(TypeName(Cur().text));
          Advance();
        } while (TakeOp(","));
      }
      if (!ExpectOp(")"))
        return false;
    }
    if (!ExpectOp("="))
      return false;
    const int skip = EmitPlaceholder(Op::Jump);
    UserFunc fn;
    fn.name = name;
    fn.params = params;
    fn.body_pc = Pc();
    out_->user_funcs_.push_back(fn);
    if (!ParseExpression())
      return false;
    Emit(Op::UserReturn);
    PatchTarget(skip, Pc());
    return true;
  }

  bool CompileOption() {
    if (!TakeKeyword("BASE"))
      return Syntax("Expected BASE after OPTION");
    if (Cur().kind != Tok::Number)
      return Syntax("Expected 0 or 1");
    const double value = Cur().number.AsNumber();
    Advance();
    if (value != 0.0 && value != 1.0)
      return Syntax("OPTION BASE takes 0 or 1");
    base_ = static_cast<int>(value);
    Emit(Op::Nop);
    return true;
  }

  bool CompileDim() {
    do {
      if (Cur().kind != Tok::Ident) {
        // Naming which word collided is worth the two lines: FN$ is reserved for DEF FN, and
        // a bare syntax error gives no hint that the name itself is the problem.
        if (Cur().kind == Tok::Keyword)
          return Syntax(Cur().text + " is a reserved word and cannot be used as a name");
        return Syntax("Expected an array name");
      }
      const std::string name = TypeName(Cur().text);
      if (ProcedureBaseExists(name))
        return Syntax("Procedure name cannot be used as an array");
      if (RejectConstWrite(name))
        return false;
      Advance();
      if (!ExpectOp("("))
        return false;
      int count = 0;
      if (!IsOp(")")) {
        do {
          if (!ParseExpression())
            return false;
          ++count;
        } while (TakeOp(","));
      }
      if (!ExpectOp(")"))
        return false;
      if (count == 0)
        return Syntax("Expected a subscript");
      Instr in;
      in.op = Op::ArrayDim;
      in.name = name;
      in.a = count;
      in.b = base_;
      out_->instrs_.push_back(in);
    } while (TakeOp(","));
    return true;
  }

  bool CompileErase() {
    std::string names;
    do {
      if (Cur().kind != Tok::Ident) {
        // Naming which word collided is worth the two lines: FN$ is reserved for DEF FN, and
        // a bare syntax error gives no hint that the name itself is the problem.
        if (Cur().kind == Tok::Keyword)
          return Syntax(Cur().text + " is a reserved word and cannot be used as a name");
        return Syntax("Expected an array name");
      }
      const std::string array_name = TypeName(Cur().text);
      if (ProcedureBaseExists(array_name))
        return Syntax("Procedure name cannot be used as an array");
      if (!names.empty())
        names += ',';
      names += array_name;
      Advance();
    } while (TakeOp(","));
    Instr in;
    in.op = Op::Erase;
    in.name = names;
    out_->instrs_.push_back(in);
    return true;
  }

  // ---------------------------------------------------------------- LET
  // Parses a variable or array-element reference, emitting the subscript
  // expressions in order (so a following value ends up on top of the stack).
  bool ParseVariableRef(std::string* name, bool* array, int* subscripts) {
    if (Cur().kind != Tok::Ident) {
      if (Cur().kind == Tok::Keyword)
        return Syntax(Cur().text + " is a reserved word and cannot be used as a name");
      return Syntax("Expected a variable name");
    }
    *name = TypeName(Cur().text);
    if (ProcedureBaseExists(*name) && !IsCurrentFunctionResult(*name))
      return Syntax("Procedure name cannot be used as a variable");
    *array = false;
    *subscripts = 0;
    Advance();
    if (IsOp("(")) {
      *array = true;
      Advance();
      if (!IsOp(")")) {
        do {
          if (!ParseExpression())
            return false;
          ++*subscripts;
        } while (TakeOp(","));
      }
      if (!ExpectOp(")"))
        return false;
    }
    return true;
  }

  // A() in STACK context means the whole array, not a zero-subscript element.
  bool AtWholeArrayRef() const {
    return Cur().kind == Tok::Ident && pos_ + 2 < tokens_.size() &&
           tokens_[pos_ + 1].kind == Tok::Op && tokens_[pos_ + 1].text == "(" &&
           tokens_[pos_ + 2].kind == Tok::Op && tokens_[pos_ + 2].text == ")";
  }

  // STACK POP/TOP target: a scalar, array element, or whole array. ParseVariableRef
  // has already emitted element subscript expressions in target order.
  bool ParseStackTarget(bool write) {
    if (AtWholeArrayRef()) {
      StackTarget target;
      target.name = TypeName(Cur().text);
      if (write && RejectConstWrite(target.name))
        return false;
      target.whole_array = true;
      target.base = base_;
      Advance(); // name
      Advance(); // (
      Advance(); // )
      out_->stack_targets_.push_back(std::move(target));
      return true;
    }
    std::string name;
    bool array = false;
    int subscripts = 0;
    if (!ParseVariableRef(&name, &array, &subscripts))
      return false;
    if (write && RejectConstWrite(name))
      return false;
    if (array && subscripts == 0)
      return Syntax("Expected a subscript in a STACK target");
    StackTarget target;
    target.name = std::move(name);
    target.array = array;
    target.subscripts = subscripts;
    target.base = base_;
    out_->stack_targets_.push_back(std::move(target));
    return true;
  }

  bool CompileLet() {
    std::string name;
    bool array = false;
    int subscripts = 0;
    if (!ParseVariableRef(&name, &array, &subscripts))
      return false;
    if (RejectConstWrite(name))
      return false;
    if (!ExpectOp("="))
      return false;
    if (!ParseExpression())
      return false;
    Instr in;
    if (array) {
      in.op = Op::ArrayStore;
      in.name = name;
      in.a = subscripts;
      in.b = base_;
    } else {
      in.op = Op::StoreVar;
      in.name = name;
    }
    out_->instrs_.push_back(in);
    return true;
  }

  // ---------------------------------------------------------------- PRINT
  bool CompilePrint() {
    if (printer_statement_) {
      // LPRINT: the same item list, but the line goes to the printer.
      Emit(Op::LPrint);
    }
    if (TakeOp("#")) {
      // PRINT #n, ... / WRITE #n, ...
      if (!ParseExpression())
        return false;
      Instr file_in;
      file_in.op = Op::FilePrint;
      file_in.a = write_statement_ ? 1 : 0;
      out_->instrs_.push_back(file_in);
      if (!TakeOp(","))
        return Syntax("Expected ',' after the file number");
    }
    if (TakeKeyword("USING")) {
      // PRINT [USING format$;] items: the format is a runtime string expression,
      // so it is evaluated and popped by Op::Using.
      if (!ParseExpression())
        return false;
      Emit(Op::Using);
      if (!TakeOp(";") && !TakeOp(","))
        return Syntax("Expected ';' after the PRINT USING format");
    }
    bool trailing_sep = false;
    for (;;) {
      if (AtListEnd())
        break;
      if (IsOp(",") || IsOp(";")) {
        const bool comma = IsOp(",");
        Advance();
        Emit(comma ? Op::SepComma : Op::SepSemi);
        trailing_sep = true;
        continue;
      }
      if (IsKeyword("TAB") || IsKeyword("SPC")) {
        const bool tab = IsKeyword("TAB");
        Advance();
        if (!ExpectOp("("))
          return false;
        if (!ParseExpression())
          return false;
        if (!ExpectOp(")"))
          return false;
        Emit(tab ? Op::Tab : Op::Spc);
        trailing_sep = false;
        continue;
      }
      if (!ParseExpression())
        return false;
      Emit(Op::Emit);
      trailing_sep = false;
    }
    Emit(trailing_sep ? Op::NoLineEnd : Op::LineEnd);
    return true;
  }

  // ---------------------------------------------------------------- INPUT
  bool CompileInput(bool line_input) {
    if (TakeOp("#")) {
      // INPUT #n, vars / LINE INPUT #n, A$
      if (!ParseExpression())
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' after the file number");
      std::string file_names;
      for (;;) {
        std::string name;
        bool array = false;
        int subscripts = 0;
        if (!ParseVariableRef(&name, &array, &subscripts))
          return false;
        if (RejectConstWrite(name))
          return false;
        if (array)
          return UnsupportedStatement("array INPUT #");
        if (!file_names.empty())
          file_names += ',';
        file_names += name;
        if (!TakeOp(","))
          break;
      }
      Instr file_in;
      file_in.op = Op::FileInput;
      file_in.name = file_names;
      file_in.b = line_input ? 1 : 0;
      out_->instrs_.push_back(file_in);
      return true;
    }
    bool question_mark = true;
    std::string prompt;
    if (TakeOp(";"))
      question_mark = false;
    if (Cur().kind == Tok::String) {
      // The prompt is displayed by the VM when the INPUT executes, not pushed as an
      // expression: the statement takes no expression operand.
      prompt = Cur().str;
      Advance();
      if (TakeOp(";")) {
        question_mark = true;
      } else if (TakeOp(",")) {
        question_mark = false;
      } else if (!line_input) {
        return Syntax("Expected ';' or ',' after the INPUT prompt");
      }
      if (AtListEnd() && !line_input)
        return Syntax("Expected a variable after the prompt");
    }
    std::string names;
    bool any_array = false;
    for (;;) {
      std::string name;
      bool array = false;
      int subscripts = 0;
      if (!ParseVariableRef(&name, &array, &subscripts))
        return false;
      if (RejectConstWrite(name))
        return false;
      if (array)
        any_array = true;
      if (!names.empty())
        names += ',';
      names += name;
      if (!TakeOp(","))
        break;
    }
    if (any_array)
      return UnsupportedStatement("array INPUT");
    if (line_input && names.find(',') != std::string::npos) {
      return Syntax("LINE INPUT takes a single string variable");
    }
    Instr in;
    in.op = line_input ? Op::LineInput : Op::Input;
    in.name = names;
    if (!prompt.empty())
      in.value = Value::Str(prompt);
    // LINE INPUT never prints a question mark: the reference says one is not printed unless
    // it is part of the prompt string. A plain INPUT does, unless a comma followed the prompt.
    in.b = (!line_input && question_mark) ? 1 : 0;
    out_->instrs_.push_back(in);
    return true;
  }

  // ---------------------------------------------------------------- IF
  bool CompileIf() {
    if (!ParseExpression())
      return false;
    const bool plain_goto = TakeKeyword("GOTO");
    const bool plain_gosub = !plain_goto && TakeKeyword("GOSUB");
    if (plain_goto || plain_gosub) {
      if (direct_)
        return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
      const int false_jump = EmitPlaceholder(Op::JumpIfFalse);
      if (plain_gosub) {
        if (!CompileGosubTarget())
          return false;
      } else if (!CompileGotoTarget()) {
        return false;
      }
      PatchTarget(false_jump, Pc());
      return true;
    }
    if (!TakeKeyword("THEN"))
      return Syntax("Expected THEN");
    const int false_jump = EmitPlaceholder(Op::JumpIfFalse);
    if (Cur().kind == Tok::Number || IsKnownLabelTarget()) {
      // `IF expr THEN line` / `IF expr THEN label` is an implicit GOTO.
      // Only known labels are considered here; `IF ... THEN A = 1` must stay a
      // statement list, not an implicit jump to a label named A.
      if (direct_)
        return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
      if (IsKnownLabelTarget()) {
        if (!CompileLabelTarget(Op::Goto))
          return false;
      } else {
        int line = 0;
        if (!LineNumberFromNumber(Cur().number.AsNumber(), &line))
          return Error(kErrIllegalFunctionCall, "Line number out of range");
        Advance();
        AddLineFixup(EmitPlaceholder(Op::Goto), line);
      }
    } else if (!CompileStatementList(true)) {
      return false;
    }
    int end_jump = -1;
    if (IsKeyword("ELSE"))
      end_jump = EmitPlaceholder(Op::Jump);
    PatchTarget(false_jump, Pc());
    if (IsKeyword("ELSE")) {
      Advance();
      if (!CompileStatementList(false))
        return false;
    }
    if (end_jump >= 0)
      PatchTarget(end_jump, Pc());
    return true;
  }

  // ---------------------------------------------------------------- FOR
  bool CompileFor() {
    if (Cur().kind != Tok::Ident)
      return Syntax("Expected a variable after FOR");
    const std::string name = TypeName(Cur().text);
    if (ProcedureBaseExists(name))
      return Syntax("Procedure name cannot be used as a variable");
    if (RejectConstWrite(name))
      return false;
    Advance();
    if (IsOp("("))
      return Syntax("Arrays are not supported yet");
    if (!ExpectOp("="))
      return false;
    if (!ParseExpression())
      return false;
    if (!TakeKeyword("TO"))
      return Syntax("Expected TO");
    if (!ParseExpression())
      return false;
    if (TakeKeyword("STEP")) {
      if (!ParseExpression())
        return false;
    } else {
      EmitPush(Value::Int(1));
    }
    const int index = EmitPlaceholder(Op::ForStart);
    out_->instrs_[static_cast<std::size_t>(index)].name = TypeName(name);
    ForEntry entry;
    entry.start_index = index;
    entry.name = TypeName(name);
    entry.line = line_;
    for_stack_.push_back(entry);
    return true;
  }

  bool CompileNext() {
    for (;;) {
      std::string name;
      if (Cur().kind == Tok::Ident) {
        name = Cur().text;
        Advance();
      }
      if (for_stack_.empty())
        return Error(kErrNextWithoutFor, ErrorMessage(kErrNextWithoutFor));
      std::size_t index = for_stack_.size() - 1;
      if (!name.empty()) {
        const std::string key = TypeName(name);
        bool found = false;
        for (std::size_t i = for_stack_.size(); i-- > 0;) {
          if (for_stack_[i].name == key) {
            index = i;
            found = true;
            break;
          }
        }
        if (!found)
          return Error(kErrNextWithoutFor, ErrorMessage(kErrNextWithoutFor));
      }
      Instr in;
      in.op = Op::ForNext;
      in.name = for_stack_[index].name;
      out_->instrs_.push_back(in);
      PatchTarget(for_stack_[index].start_index, Pc());
      for_stack_.resize(index);
      if (!TakeOp(","))
        break;
    }
    return true;
  }

  // ---------------------------------------------------------------- WHILE
  bool CompileWhile() {
    const int test_pc = Pc();
    if (!ParseExpression())
      return false;
    const int jump = EmitPlaceholder(Op::JumpIfFalse);
    WhileEntry entry;
    entry.jump_index = jump;
    entry.test_pc = test_pc;
    entry.line = line_;
    while_stack_.push_back(entry);
    return true;
  }

  bool CompileWend() {
    if (while_stack_.empty()) {
      return Error(kErrWendWithoutWhile, ErrorMessage(kErrWendWithoutWhile));
    }
    const WhileEntry entry = while_stack_.back();
    while_stack_.pop_back();
    Instr in;
    in.op = Op::Jump;
    in.a = entry.test_pc;
    out_->instrs_.push_back(in);
    PatchTarget(entry.jump_index, Pc());
    return true;
  }

  // ---------------------------------------------------------------- constants
  bool CompileConst() {
    std::size_t next = 0;
    if (!ParseConstDecl(tokens_, static_cast<std::size_t>(pos_), line_, false, &next))
      return false;
    pos_ = static_cast<int>(next);
    Emit(Op::Nop);
    return true;
  }

  // ---------------------------------------------------------------- procedures
  static Value::Kind KindForName(const std::string& name) {
    switch (name.empty() ? '!' : name.back()) {
    case '%':
      return Value::Kind::Int;
    case '&':
      return Value::Kind::Long;
    case '#':
      return Value::Kind::Dbl;
    case '$':
      return Value::Kind::Str;
    default:
      return Value::Kind::Sgl;
    }
  }

  bool ValidLocalName(const Procedure& proc, const std::string& canonical) {
    if (local_names_.count(canonical) != 0)
      return Syntax("Duplicate parameter/local " + canonical);
    if (StripTypeSuffix(canonical) == StripTypeSuffix(proc.name))
      return Syntax("Parameter/local name collides with procedure " + proc.name);
    if (RejectConstWrite(canonical))
      return false;
    for (const std::string& constant : const_names_) {
      if (StripTypeSuffix(constant) == StripTypeSuffix(canonical))
        return Syntax("Parameter/local name collides with CONST " + constant);
    }
    local_names_.insert(canonical);
    return true;
  }

  bool CompileProcedureHeader(bool function) {
    if (direct_)
      return Error(kErrIllegalDirect, "SUB/FUNCTION is illegal in direct mode");
    if (current_proc_ >= 0)
      return Syntax("SUB/FUNCTION cannot be nested");
    if (!IsKeyword("SUB") && !IsKeyword("FUNCTION"))
      return Syntax("Expected SUB or FUNCTION");
    Advance(); // SUB/FUNCTION
    if (Cur().kind != Tok::Ident)
      return Syntax("Expected a procedure name");
    const std::string name = NormalizeLabel(Cur().text);
    Advance();
    const int index = out_->ProcedureIndex(name);
    if (index < 0)
      return Error(kErrUndefinedProcedure, "Undefined procedure " + name);
    Procedure& proc = out_->procedures_[static_cast<std::size_t>(index)];
    if (proc.header_line != line_)
      return Syntax("Procedure header line mismatch");
    if ((function ? ProcKind::Function : ProcKind::Sub) != proc.kind)
      return Syntax("Procedure kind does not match its definition");
    local_names_.clear();
    proc.params.clear();
    proc.locals.clear();
    if (TakeOp("(")) {
      if (!IsOp(")")) {
        for (;;) {
          if (Cur().kind != Tok::Ident)
            return Syntax("Expected a parameter name");
          const std::string canonical = TypeName(Cur().text);
          Advance();
          if (IsOp("("))
            return Syntax("Array parameters are not supported yet");
          if (!ValidLocalName(proc, canonical))
            return false;
          ProcParam param;
          param.name = canonical;
          param.kind = KindForName(canonical);
          proc.params.push_back(std::move(param));
          if (!TakeOp(","))
            break;
        }
      }
      if (!ExpectOp(")"))
        return false;
    }
    if (!AtStatementEnd() && !IsOp(":"))
      return Syntax("Procedure header must be alone on its line");
    const int jump = EmitPlaceholder(Op::Jump);
    proc.body_pc = Pc();
    proc.header_line = line_;
    proc.kind = function ? ProcKind::Function : ProcKind::Sub;
    proc.result_name = function ? name : std::string();
    current_proc_ = index;
    proc_has_executable_ = false;
    proc_header_jump_ = jump;
    return true;
  }

  bool CompileLocal() {
    if (current_proc_ < 0)
      return Syntax("LOCAL is only valid inside a procedure");
    if (direct_)
      return Error(kErrIllegalDirect, "LOCAL is illegal in direct mode");
    if (proc_has_executable_)
      return Syntax("LOCAL must appear before executable statements");
    Procedure& proc = out_->procedures_[static_cast<std::size_t>(current_proc_)];
    for (;;) {
      if (Cur().kind != Tok::Ident)
        return Syntax("Expected a local variable name");
      const std::string canonical = TypeName(Cur().text);
      Advance();
      if (IsOp("("))
        return Syntax("Local arrays are not supported yet");
      if (!ValidLocalName(proc, canonical))
        return false;
      ProcParam local;
      local.name = canonical;
      local.kind = KindForName(canonical);
      proc.locals.push_back(std::move(local));
      if (!TakeOp(","))
        break;
    }
    if (!AtStatementEnd() && !IsOp(":"))
      return Syntax("Expected end of LOCAL statement");
    return true;
  }

  bool CompileCall() {
    if (direct_)
      return Error(kErrIllegalDirect, "CALL is illegal in direct mode");
    if (Cur().kind != Tok::Ident)
      return Syntax("Expected a procedure name after CALL");
    const std::string name = NormalizeLabel(Cur().text);
    Advance();
    const int index = out_->ProcedureIndex(name);
    if (index < 0)
      return Error(kErrUndefinedProcedure, "Undefined procedure " + name);
    const Procedure& proc = out_->procedures_[static_cast<std::size_t>(index)];
    if (proc.kind != ProcKind::Sub)
      return Syntax("CALL requires a SUB; use the FUNCTION in an expression");
    int argc = 0;
    if (TakeOp("(")) {
      if (!IsOp(")")) {
        do {
          if (!ParseExpression())
            return false;
          ++argc;
        } while (TakeOp(","));
      }
      if (!ExpectOp(")"))
        return false;
    } else if (!AtListEnd() && !IsOp(":")) {
      return Syntax("Expected '(' after the procedure name");
    }
    if (static_cast<std::size_t>(argc) != proc.params.size()) {
      return Syntax(name + " takes " + std::to_string(proc.params.size()) +
                    " argument(s)");
    }
    Instr in;
    in.op = Op::CallProc;
    in.a = index;
    in.b = argc;
    out_->instrs_.push_back(in);
    return true;
  }

  bool CompileExit() {
    if (current_proc_ < 0)
      return Syntax("EXIT is only valid inside a procedure");
    const Procedure& proc = out_->procedures_[static_cast<std::size_t>(current_proc_)];
    if (IsKeyword("SUB")) {
      if (proc.kind != ProcKind::Sub)
        return Syntax("EXIT SUB inside a FUNCTION");
      Advance();
      Instr in;
      in.op = Op::SubReturn;
      in.a = 0;
      out_->instrs_.push_back(in);
      return true;
    }
    if (IsKeyword("FUNCTION")) {
      if (proc.kind != ProcKind::Function)
        return Syntax("EXIT FUNCTION inside a SUB");
      Advance();
      Instr in;
      in.op = Op::SubReturn;
      in.a = 1;
      out_->instrs_.push_back(in);
      return true;
    }
    return Syntax("Expected SUB or FUNCTION after EXIT");
  }

  bool CompileProcedureEnd(bool function_end) {
    if (direct_)
      return Error(kErrIllegalDirect, "END SUB/FUNCTION is illegal in direct mode");
    if (current_proc_ < 0)
      return Syntax("END SUB/FUNCTION without an open procedure");
    Procedure& proc = out_->procedures_[static_cast<std::size_t>(current_proc_)];
    if ((function_end ? ProcKind::Function : ProcKind::Sub) != proc.kind)
      return Syntax("Procedure END does not match its header");
    Instr in;
    in.op = Op::SubReturn;
    in.a = proc.kind == ProcKind::Function ? 1 : 0;
    out_->instrs_.push_back(in);
    proc.end_pc = Pc() - 1;
    proc.end_line = line_;
    if (proc_header_jump_ < 0)
      return Syntax("Internal procedure header error");
    PatchTarget(proc_header_jump_, Pc());
    current_proc_ = -1;
    proc_has_executable_ = false;
    local_names_.clear();
    proc_header_jump_ = -1;
    return true;
  }

  // ---------------------------------------------------------------- labels
  bool CompileLabel() {
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    if (Cur().kind != Tok::Ident || !IsLabelName(Cur().text))
      return Syntax("Expected a label name after LABEL");
    const std::string name = NormalizeLabel(Cur().text);
    const auto existing = labels_.find(name);
    if (existing != labels_.end() && existing->second != line_)
      return Syntax("Duplicate label " + name);
    labels_[name] = line_;
    Emit(Op::Nop);
    Advance();
    return true;
  }

  // ---------------------------------------------------------------- SELECT CASE
  bool CompileSelectCase() {
    if (!TakeKeyword("CASE"))
      return Syntax("Expected CASE after SELECT");
    if (!ParseExpression())
      return false;
    SelectEntry entry;
    entry.line = line_;
    entry.push_index = EmitPlaceholder(Op::SelectPush);
    out_->instrs_[static_cast<std::size_t>(entry.push_index)].a = line_;
    select_stack_.push_back(entry);
    return true;
  }

  void EmitSelectLoad() {
    const int index = EmitPlaceholder(Op::SelectLoad);
    out_->instrs_[static_cast<std::size_t>(index)].a = select_stack_.back().line;
  }

  bool EmitCaseValue(std::vector<int>* success_jumps) {
    EmitSelectLoad();
    if (!ParseExpression())
      return false;
    Emit(Op::Eq);
    const int false_jump = EmitPlaceholder(Op::JumpIfFalse);
    success_jumps->push_back(EmitPlaceholder(Op::Jump));
    PatchTarget(false_jump, Pc());
    return true;
  }

  bool EmitCaseComparison(Op comparison, std::vector<int>* success_jumps) {
    EmitSelectLoad();
    if (!ParseExpression())
      return false;
    Emit(comparison);
    const int false_jump = EmitPlaceholder(Op::JumpIfFalse);
    success_jumps->push_back(EmitPlaceholder(Op::Jump));
    PatchTarget(false_jump, Pc());
    return true;
  }

  bool CompileCase() {
    if (select_stack_.empty())
      return Syntax("CASE without SELECT CASE");
    SelectEntry& entry = select_stack_.back();
    if (entry.has_else)
      return Syntax("CASE after CASE ELSE");

    // Close the previous case body and make its "no match" jump land here.
    if (entry.body_open)
      entry.exit_jumps.push_back(EmitPlaceholder(Op::Jump));
    if (entry.next_case_jump >= 0) {
      PatchTarget(entry.next_case_jump, Pc());
      entry.next_case_jump = -1;
    }

    if (TakeKeyword("ELSE")) {
      entry.has_else = true;
      entry.body_open = true;
      return true;
    }

    std::vector<int> success_jumps;
    for (;;) {
      if (TakeWord("IS")) {
        Op comparison = Op::Eq;
        if (TakeOp(">="))
          comparison = Op::Ge;
        else if (TakeOp("<="))
          comparison = Op::Le;
        else if (TakeOp("<>"))
          comparison = Op::Ne;
        else if (TakeOp(">"))
          comparison = Op::Gt;
        else if (TakeOp("<"))
          comparison = Op::Lt;
        else if (TakeOp("="))
          comparison = Op::Eq;
        else
          return Syntax("Expected a comparison after CASE IS");
        if (!EmitCaseComparison(comparison, &success_jumps))
          return false;
      } else {
        if (!EmitCaseValue(&success_jumps))
          return false;
      }
      if (!TakeOp(","))
        break;
    }

    // No case matched: fall through to the next CASE or END SELECT while the
    // selector remains on the selector stack.
    entry.next_case_jump = EmitPlaceholder(Op::Jump);
    const int body_start = Pc();
    for (int jump : success_jumps)
      PatchTarget(jump, body_start);
    entry.body_open = true;
    return true;
  }

  bool CompileEndSelect() {
    if (select_stack_.empty())
      return Syntax("END SELECT without SELECT CASE");
    SelectEntry entry = select_stack_.back();
    if (entry.next_case_jump >= 0)
      PatchTarget(entry.next_case_jump, Pc());
    out_->instrs_[static_cast<std::size_t>(entry.push_index)].b = line_;
    // Emit the pop at the very start of END SELECT so a GOTO to this line
    // closes the block instead of skipping the pop via a case-body exit jump.
    const int pop_index = EmitPlaceholder(Op::SelectPop);
    out_->instrs_[static_cast<std::size_t>(pop_index)].a = entry.line;
    // Case bodies and a no-match path jump to the SelectPop. Do not add a jump
    // after the pop here: patching it together with the case exits would run
    // SelectPop twice.
    for (int jump : entry.exit_jumps)
      PatchTarget(jump, pop_index);
    select_stack_.pop_back();
    return true;
  }

  // ---------------------------------------------------------------- jumps
  bool CompileLineTarget(Op op) {
    if (Cur().kind != Tok::Number)
      return Syntax("Expected a line number");
    int target_line = 0;
    if (!LineNumberFromNumber(Cur().number.AsNumber(), &target_line))
      return Error(kErrIllegalFunctionCall, "Line number out of range");
    Advance();
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    AddLineFixup(EmitPlaceholder(op), target_line);
    return true;
  }

  // True when the token after the current one ends the statement. GOTO/GOSUB use
  // this to keep literal targets compile-time-resolved while allowing expressions.
  bool NextIsStatementEnd() const {
    if (pos_ + 1 >= tokens_.size())
      return true;
    const Token& next = tokens_[pos_ + 1];
    return next.kind == Tok::EndOfLine ||
           (next.kind == Tok::Op && (next.text == ":" || next.text == "'")) ||
           (next.kind == Tok::Keyword && next.text == "ELSE");
  }

  bool IsLabelTarget() const {
    // A bare identifier in a line-target position is a label. Type-suffixed
    // identifiers (T%, T&, ...) remain computed line-number expressions; use
    // parentheses for an unsuffixed computed variable, e.g. GOTO (T).
    return Cur().kind == Tok::Ident && IsLabelName(Cur().text);
  }

  bool IsKnownLabelTarget() const {
    return IsLabelTarget() &&
           labels_.find(NormalizeLabel(Cur().text)) != labels_.end();
  }

  bool CompileLabelTarget(Op op, int owner_proc = -2, bool boundary_exempt = false,
                          bool main_only = false) {
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    const std::string name = NormalizeLabel(Cur().text);
    Advance();
    AddLabelFixup(EmitPlaceholder(op), name, owner_proc, boundary_exempt, main_only);
    return true;
  }

  // GOTO target: a known label, a literal line, or a numeric expression.
  bool CompileGotoTarget() {
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    if (IsLabelTarget())
      return CompileLabelTarget(Op::Goto);
    if (Cur().kind == Tok::Number && NextIsStatementEnd())
      return CompileLineTarget(Op::Goto);
    if (!ParseExpression())
      return false;
    Emit(Op::GotoIndirect);
    return true;
  }

  // GOSUB target: a known label, a literal line, or a numeric expression.
  bool CompileGosubTarget() {
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    if (IsLabelTarget())
      return CompileLabelTarget(Op::Gosub);
    if (Cur().kind == Tok::Number && NextIsStatementEnd())
      return CompileLineTarget(Op::Gosub);
    if (!ParseExpression())
      return false;
    Emit(Op::GosubIndirect);
    return true;
  }

  bool CompileOn() {
    if (IsKeyword("KEY")) {
      // ON KEY(n) belonged to the removed KEY family. Refuse it explicitly so
      // the diagnostic names the construct instead of reporting a syntax error.
      Advance();
      return UnsupportedStatement("ON KEY");
    }
    if (IsKeyword("COM") || IsKeyword("STRIG") || IsKeyword("PLAY") || IsKeyword("PEN")) {
      // There is no COM port, joystick, note queue or light pen to trap here.
      const std::string name = Cur().text;
      while (!AtListEnd() && !IsOp(":"))
        Advance();
      Instr in;
      in.op = Op::Fail;
      in.a = kErrAdvancedFeature;
      in.name = "ON " + name + " needs hardware newbasic does not have";
      out_->instrs_.push_back(in);
      return true;
    }
    if (IsKeyword("TIMER")) {
      Advance();
      if (!ExpectOp("("))
        return false;
      if (!ParseExpression()) // interval in seconds
        return false;
      if (!ExpectOp(")"))
        return false;
      if (!TakeKeyword("GOSUB"))
        return Syntax("Expected GOSUB after ON TIMER(n)");
      if (IsLabelTarget())
        return CompileLabelTarget(Op::OnTimer, current_proc_, true, true);
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number or label");
      int target = 0;
      if (!LineNumberFromNumber(Cur().number.AsNumber(), &target))
        return Error(kErrIllegalFunctionCall, "Line number out of range");
      Advance();
      AddLineFixup(EmitPlaceholder(Op::OnTimer), target, current_proc_, true, true);
      return true;
    }
    if (IsKeyword("ERROR")) {
      Advance();
      if (!TakeKeyword("GOTO"))
        return Syntax("Expected GOTO after ON ERROR");
      if (IsLabelTarget())
        return CompileLabelTarget(Op::OnError, current_proc_, true, true);
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number or label");
      int target = 0;
      if (!LineNumberFromNumber(Cur().number.AsNumber(), &target))
        return Error(kErrIllegalFunctionCall, "Line number out of range");
      Advance();
      if (target == 0) {
        Instr in;
        in.op = Op::OnError;
        in.a = -1; // ON ERROR GOTO 0 disables trapping
        out_->instrs_.push_back(in);
        return true;
      }
      if (direct_)
        return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
      AddLineFixup(EmitPlaceholder(Op::OnError), target, current_proc_, true, true);
      return true;
    }
    if (!ParseExpression())
      return false;
    int mode = 0;
    if (TakeKeyword("GOTO")) {
      mode = 0;
    } else if (TakeKeyword("GOSUB")) {
      mode = 1;
    } else {
      return Syntax("Expected GOTO or GOSUB");
    }
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    const int table_index = static_cast<int>(out_->jump_tables_.size());
    out_->jump_tables_.push_back({});
    Instr in;
    in.op = Op::OnJump;
    in.a = table_index;
    in.b = mode;
    out_->instrs_.push_back(in);
    int slot = 0;
    do {
      out_->jump_tables_[static_cast<std::size_t>(table_index)].push_back(-1);
      Fixup fixup;
      fixup.instr_index = -1;
      fixup.table = table_index;
      fixup.slot = slot;
      fixup.owner_proc = current_proc_;
      fixup.source_line = line_;
      if (IsLabelTarget()) {
        fixup.label = NormalizeLabel(Cur().text);
        Advance();
      } else {
        if (Cur().kind != Tok::Number)
          return Syntax("Expected a line number or label");
        int target_line = 0;
        if (!LineNumberFromNumber(Cur().number.AsNumber(), &target_line))
          return Error(kErrIllegalFunctionCall, "Line number out of range");
        Advance();
        fixup.line = target_line;
      }
      fixups_.push_back(fixup);
      ++slot;
    } while (TakeOp(","));
    return true;
  }

  // ---------------------------------------------------------------- screen
  // VIEW PRINT [topline TO bottomline]: the text window (M5), split out so the VIEW
  // branch can tell the two forms apart.
  bool CompileViewPrintStatement() {
    Advance(); // past PRINT
    if (AtListEnd()) {
      EmitPush(Value::Int(0));
      EmitPush(Value::Int(0));
    } else {
      if (!ParseExpression())
        return false;
      if (!TakeKeyword("TO"))
        return Syntax("Expected TO in VIEW PRINT");
      if (!ParseExpression())
        return false;
    }
    Emit(Op::ViewPrint);
    return true;
  }

  // LOCATE [row][, [col][, [cursor][, [start][, stop]]]]
  //
  // An omitted argument keeps the current value, so -1 is pushed for it: the machine reads
  // that as "unchanged". The CRT scan-line arguments are parsed and ignored, since a
  // windowed or console surface has no beam to shape.
  bool CompileLocate() {
    for (int slot = 0; slot < 5; ++slot) {
      if (slot > 0) {
        if (!TakeOp(",")) {
          for (int rest = slot; rest < 5; ++rest)
            EmitPush(Value::Int(-1));
          break;
        }
      }
      if (AtListEnd() || IsOp(":") || IsOp(","))
        EmitPush(Value::Int(-1));
      else if (!ParseExpression())
        return false;
    }
    Emit(Op::Locate);
    return true;
  }

  bool CompileColor() {
    if (AtListEnd() || IsOp(":") || IsOp(",")) {
      EmitPush(Value::Int(-1));
    } else if (!ParseExpression()) {
      return false;
    }
    if (TakeOp(",")) {
      if (AtListEnd() || IsOp(":") || IsOp(",")) {
        EmitPush(Value::Int(-1));
      } else if (!ParseExpression()) {
        return false;
      }
      if (TakeOp(",")) {
        // Border color: parsed for compatibility, ignored on text screens.
        if (!ParseExpression())
          return false;
      }
    } else {
      EmitPush(Value::Int(-1));
    }
    Emit(Op::Color);
    return true;
  }

  bool CompileRandomize() {
    Instr in;
    in.op = Op::Randomize;
    if (AtListEnd() || IsOp(":")) {
      in.b = 1;
      out_->instrs_.push_back(in);
      return true;
    }
    if (!ParseExpression())
      return false;
    in.b = 0;
    out_->instrs_.push_back(in);
    return true;
  }

  bool CompileSwap() {
    std::string first;
    std::string second;
    bool first_array = false;
    bool second_array = false;
    int first_subscripts = 0;
    int second_subscripts = 0;
    if (!ParseVariableRef(&first, &first_array, &first_subscripts))
      return false;
    if (!ExpectOp(","))
      return false;
    if (!ParseVariableRef(&second, &second_array, &second_subscripts))
      return false;
    if (RejectConstWrite(first) || RejectConstWrite(second))
      return false;
    // Array elements push their subscript expressions as they are parsed, the second side's last,
    // so the machine pops the second list before the first and the counts travel with the
    // instruction. A count of zero marks a plain scalar.
    Instr in;
    in.op = Op::Swap;
    in.name = first + "," + second;
    in.a = first_array ? first_subscripts : 0;
    in.b = second_array ? second_subscripts : 0;
    out_->instrs_.push_back(in);
    return true;
  }

  // ---------------------------------------------------------------- expressions
  bool ParseExpression() { return ParseImp(); }

  bool ParseBinary(const char* keyword, Op op, bool (Compiler::*next)()) {
    if (!(this->*next)())
      return false;
    while (TakeKeyword(keyword)) {
      if (!(this->*next)())
        return false;
      Emit(op);
    }
    return true;
  }

  bool ParseImp() { return ParseBinary("IMP", Op::Imp, &Compiler::ParseEqv); }
  bool ParseEqv() { return ParseBinary("EQV", Op::Eqv, &Compiler::ParseXor); }
  bool ParseXor() { return ParseBinary("XOR", Op::Xor, &Compiler::ParseOr); }
  bool ParseOr() { return ParseBinary("OR", Op::Or, &Compiler::ParseAnd); }
  bool ParseAnd() { return ParseBinary("AND", Op::And, &Compiler::ParseNot); }

  bool ParseNot() {
    if (TakeKeyword("NOT")) {
      if (!ParseNot())
        return false;
      Emit(Op::Not);
      return true;
    }
    return ParseRelational();
  }

  bool ParseRelational() {
    if (!ParseAdditive())
      return false;
    for (;;) {
      Op op;
      if (TakeOp("="))
        op = Op::Eq;
      else if (TakeOp("<>"))
        op = Op::Ne;
      else if (TakeOp("<="))
        op = Op::Le;
      else if (TakeOp(">="))
        op = Op::Ge;
      else if (TakeOp("<"))
        op = Op::Lt;
      else if (TakeOp(">"))
        op = Op::Gt;
      else
        break;
      if (!ParseAdditive())
        return false;
      Emit(op);
    }
    return true;
  }

  bool ParseAdditive() {
    if (!ParseMod())
      return false;
    for (;;) {
      if (TakeOp("+")) {
        if (!ParseMod())
          return false;
        Emit(Op::Add);
      } else if (TakeOp("-")) {
        if (!ParseMod())
          return false;
        Emit(Op::Sub);
      } else {
        break;
      }
    }
    return true;
  }

  bool ParseMod() { return ParseBinary("MOD", Op::Mod, &Compiler::ParseIntDiv); }

  bool ParseIntDiv() {
    if (!ParseMulDiv())
      return false;
    while (TakeOp("\\")) {
      if (!ParseMulDiv())
        return false;
      Emit(Op::IntDiv);
    }
    return true;
  }

  bool ParseMulDiv() {
    if (!ParseUnary())
      return false;
    for (;;) {
      if (TakeOp("*")) {
        if (!ParseUnary())
          return false;
        Emit(Op::Mul);
      } else if (TakeOp("/")) {
        if (!ParseUnary())
          return false;
        Emit(Op::Div);
      } else {
        break;
      }
    }
    return true;
  }

  bool ParseUnary() {
    if (TakeOp("-")) {
      if (!ParseUnary())
        return false;
      Emit(Op::Neg);
      return true;
    }
    if (TakeOp("+"))
      return ParseUnary();
    return ParsePower();
  }

  bool ParsePower() {
    if (!ParsePrimary())
      return false;
    if (TakeOp("^")) {
      if (!ParseUnary())
        return false;
      Emit(Op::Pow);
    }
    return true;
  }

  bool ParsePrimary() {
    if (Cur().kind == Tok::Number) {
      EmitPush(Cur().number);
      Advance();
      return true;
    }
    if (Cur().kind == Tok::String) {
      EmitPush(Value::Str(Cur().str));
      Advance();
      return true;
    }
    if (IsOp("(")) {
      Advance();
      if (!ParseExpression())
        return false;
      return ExpectOp(")");
    }
    if (Cur().kind == Tok::Ident) {
      const std::string name = TypeName(Cur().text);
      // A declared SUB/FUNCTION owns its base name. A FUNCTION is called with
      // parentheses; inside its own body the bare result name reads the result
      // local. A SUB name is not an expression.
      const int proc_index = out_->ProcedureIndex(name);
      if (proc_index >= 0) {
        const Procedure& proc = out_->procedures_[static_cast<std::size_t>(proc_index)];
        Advance();
        if (proc.kind == ProcKind::Function) {
          if (!IsOp("(")) {
            if (IsCurrentFunctionResult(name)) {
              Instr in;
              in.op = Op::LoadVar;
              in.name = name;
              out_->instrs_.push_back(in);
              return true;
            }
            return Syntax("Function name requires arguments");
          }
          Advance(); // (
          int argc = 0;
          if (!IsOp(")")) {
            do {
              if (!ParseExpression())
                return false;
              ++argc;
            } while (TakeOp(","));
          }
          if (!ExpectOp(")"))
            return false;
          if (static_cast<std::size_t>(argc) != proc.params.size())
            return Syntax(name + " takes " + std::to_string(proc.params.size()) +
                          " argument(s)");
          Instr in;
          in.op = Op::CallProc;
          in.a = proc_index;
          in.b = argc;
          out_->instrs_.push_back(in);
          return true;
        }
        return Syntax("SUB name cannot be used in an expression");
      }
      if (ProcedureBaseExists(name))
        return Syntax("Procedure name cannot be used as a variable");
      // `FNA` is one identifier now. A matching DEF FN is a user-function call;
      // the FN prefix keeps its function-call meaning when followed by `(`.
      // Without parentheses, FNA can still be an ordinary scalar name.
      const bool fn_prefix = name.size() > 2 && name.compare(0, 2, "FN") == 0;
      const int user_index = fn_prefix ? FindUserFunc(name) : -1;
      Advance();
      if (user_index >= 0) {
        int argc = 0;
        if (TakeOp("(")) {
          if (!IsOp(")")) {
            do {
              if (!ParseExpression())
                return false;
              ++argc;
            } while (TakeOp(","));
          }
          if (!ExpectOp(")"))
            return false;
        }
        const std::size_t want =
            out_->user_funcs_[static_cast<std::size_t>(user_index)].params.size();
        if (static_cast<std::size_t>(argc) != want) {
          return Syntax(name + " takes " + std::to_string(want) + " argument(s)");
        }
        Instr in;
        in.op = Op::CallUser;
        in.a = user_index;
        in.b = argc;
        out_->instrs_.push_back(in);
        return true;
      }
      if (fn_prefix && IsOp("("))
        return Error(kErrUndefinedUserFunction, ErrorMessage(kErrUndefinedUserFunction));
      if (IsOp("(")) {
        Advance();
        int count = 0;
        if (!IsOp(")")) {
          do {
            if (!ParseExpression())
              return false;
            ++count;
          } while (TakeOp(","));
        }
        if (!ExpectOp(")"))
          return false;
        Instr in;
        in.op = Op::ArrayLoad;
        in.name = name;
        in.a = count;
        in.b = base_;
        out_->instrs_.push_back(in);
        return true;
      }
      Instr in;
      in.op = Op::LoadVar;
      in.name = name;
      out_->instrs_.push_back(in);
      return true;
    }
    if (Cur().kind == Tok::Keyword) {
      const std::string keyword = Cur().text;
      Func func{};
      if (LookupFunc(keyword, &func)) {
        Advance();
        int argc = 0;
        if (TakeOp("(")) {
          if (!IsOp(")")) {
            do {
              if (!ParseExpression())
                return false;
              ++argc;
            } while (TakeOp(","));
          }
          if (!ExpectOp(")"))
            return false;
        }
        const int arity = FuncArity(func);
        if (arity >= 0 && argc != arity) {
          return Syntax(std::string(FuncName(func)) + " takes " + std::to_string(arity) +
                        " argument(s)");
        }
        if (arity < 0) {
          if (func == Func::Mid && (argc < 2 || argc > 3)) {
            return Syntax("MID$ takes 2 or 3 arguments");
          }
          if (func == Func::Instr && (argc < 2 || argc > 3)) {
            return Syntax("INSTR takes 2 or 3 arguments");
          }
          if (func == Func::Rnd && argc > 1)
            return Syntax("RND takes at most 1 argument");
          if (func == Func::String && argc != 2)
            return Syntax("STRING$ takes 2 arguments");
          if ((func == Func::TextWidth || func == Func::TextHeight) && (argc < 1 || argc > 2)) {
            return Syntax(std::string(FuncName(func)) + " takes 1 or 2 arguments");
          }
        }
        Instr in;
        in.op = Op::CallFunc;
        in.a = static_cast<std::int32_t>(func);
        in.b = argc;
        out_->instrs_.push_back(in);
        return true;
      }
      if (keyword == "FN") {
        Advance();
        if (Cur().kind != Tok::Ident)
          return Syntax("Expected a function name after FN");
        const std::string fname = "FN" + TypeName(Cur().text);
        Advance();
        int argc = 0;
        if (TakeOp("(")) {
          if (!IsOp(")")) {
            do {
              if (!ParseExpression())
                return false;
              ++argc;
            } while (TakeOp(","));
          }
          if (!ExpectOp(")"))
            return false;
        }
        const int index = FindUserFunc(fname);
        if (index < 0)
          return Error(kErrUndefinedUserFunction, ErrorMessage(kErrUndefinedUserFunction));
        const std::size_t want = out_->user_funcs_[static_cast<std::size_t>(index)].params.size();
        if (static_cast<std::size_t>(argc) != want) {
          return Syntax(fname + " takes " + std::to_string(want) + " argument(s)");
        }
        Instr in;
        in.op = Op::CallUser;
        in.a = index;
        in.b = argc;
        out_->instrs_.push_back(in);
        return true;
      }
      if (keyword == "ERL" || keyword == "ERR") {
        const Func error_func = keyword == "ERL" ? Func::Erl : Func::Err;
        Advance();
        Instr in;
        in.op = Op::CallFunc;
        in.a = static_cast<std::int32_t>(error_func);
        in.b = 0;
        out_->instrs_.push_back(in);
        return true;
      }
      if (keyword == "TAB" || keyword == "SPC") {
        return Syntax(keyword + " is only valid in PRINT");
      }
      return Syntax("Expected an expression");
    }
    return Syntax("Expected an expression");
  }

  // ---------------------------------------------------------------- finish
  bool Finish() {
    if (current_proc_ >= 0) {
      error_->line = line_;
      error_->code = kErrSyntax;
      error_->message = "SUB/FUNCTION without END";
      return false;
    }
    if (!for_stack_.empty()) {
      error_->line = for_stack_.back().line;
      error_->code = kErrForWithoutNext;
      error_->message = ErrorMessage(kErrForWithoutNext);
      return false;
    }
    if (!while_stack_.empty()) {
      error_->line = while_stack_.back().line;
      error_->code = kErrWhileWithoutWend;
      error_->message = ErrorMessage(kErrWhileWithoutWend);
      return false;
    }
    if (!select_stack_.empty()) {
      error_->line = select_stack_.back().line;
      error_->code = kErrSyntax;
      error_->message = "SELECT CASE without END SELECT";
      return false;
    }
    Emit(Op::End); // implicit END when execution falls off the program
    for (const Fixup& fixup : fixups_) {
      int line = fixup.line;
      if (!fixup.label.empty()) {
        const auto label = labels_.find(fixup.label);
        if (label == labels_.end()) {
          error_->line = line_;
          error_->code = kErrUndefinedLineNumber;
          error_->message = "Undefined label " + fixup.label;
          return false;
        }
        line = label->second;
      }
      if (line > 0) {
        const std::optional<int> target_proc = out_->ProcedureForLine(line);
        if (fixup.main_only) {
          if (target_proc) {
            error_->line = fixup.source_line;
            error_->code = kErrSyntax;
            error_->message = "ON ERROR/ON TIMER handler must be in main";
            return false;
          }
        } else if (!fixup.boundary_exempt) {
          if (target_proc) {
            const Procedure& proc =
                out_->procedures_[static_cast<std::size_t>(*target_proc)];
            if (line == proc.header_line || line == proc.end_line) {
              error_->line = fixup.source_line;
              error_->code = kErrProcedureBoundary;
              error_->message = "GOTO/GOSUB cannot target a procedure header or END line";
              return false;
            }
          }
          const int target = target_proc.value_or(-1);
          if (target != fixup.owner_proc) {
            error_->line = fixup.source_line;
            error_->code = kErrProcedureBoundary;
            error_->message = "GOTO/GOSUB would cross a procedure boundary";
            return false;
          }
        }
      }
      int target = -1;
      const auto it = line_starts_.find(line);
      if (it != line_starts_.end())
        target = it->second;
      if (fixup.instr_index >= 0) {
        PatchTarget(fixup.instr_index, target);
      } else {
        out_->jump_tables_[static_cast<std::size_t>(fixup.table)]
                          [static_cast<std::size_t>(fixup.slot)] = target;
      }
    }
    return true;
  }

  struct ForEntry {
    int start_index = 0;
    std::string name;
    int line = 0;
  };
  struct WhileEntry {
    int jump_index = 0;
    int test_pc = 0;
    int line = 0;
  };
  struct Fixup {
    int instr_index = 0; // -1 => jump table slot
    int table = 0;
    int slot = 0;
    int line = 0;
    std::string label; // when non-empty, resolve through labels_ at Finish
    int owner_proc = -1;
    bool boundary_exempt = false;
    bool main_only = false;
    int source_line = 0;
  };

  Program* out_;
  CompileError* error_;
  bool direct_ = false;
  bool write_statement_ = false;        // WRITE uses quoted strings and commas
  bool printer_statement_ = false;      // LPRINT binds the print list to the printer
  char default_type_[26] = {};          // DEF* default storage type per letter
  int base_ = 0;                        // OPTION BASE (0 or 1)
  const std::string* source_ = nullptr; // current physical line (for DATA)
  std::vector<Token> tokens_;
  std::size_t pos_ = 0;
  int line_ = 0;
  std::vector<ForEntry> for_stack_;
  std::vector<WhileEntry> while_stack_;
  std::vector<SelectEntry> select_stack_;
  std::vector<Fixup> fixups_;
  std::vector<LineSpan> line_spans_;
  std::map<int, int> line_starts_;
  std::map<std::string, int> labels_; // normalized label -> source line
  std::set<std::string> const_names_;             // canonical CONST names
  std::map<std::string, std::string> const_alias_; // base name -> canonical name
  std::map<std::string, int> procedure_bases_;     // base name -> procedure index
  int current_proc_ = -1;
  int proc_header_jump_ = -1;
  bool proc_has_executable_ = false;
  std::set<std::string> local_names_;
};

} // namespace

bool IsIllegalDirect(const std::string& keyword) {
  static const char* const kIllegalDirect[] = {
      "CALL", "CASE", "CONST", "DATA", "DEF", "DIM", "DELETE", "ERASE", "EXIT",
      "FOR", "FUNCTION", "GOSUB", "GOTO", "LABEL", "LOCAL", "NEXT", "RESUME",
      "RETURN", "SELECT", "SUB", "WEND", "WHILE",
  };
  return InList(kIllegalDirect, sizeof(kIllegalDirect) / sizeof(kIllegalDirect[0]), keyword);
}

bool Compile(const LineStore& store, Program* out, CompileError* error) {
  out->instrs_.clear();
  out->line_table_.clear();
  out->jump_tables_.clear();
  out->data_items_.clear();
  out->line_data_index_.clear();
  out->user_funcs_.clear();
  out->procedures_.clear();
  out->procedure_index_.clear();
  out->constants_.clear();
  out->statement_pcs_.clear();
  Compiler compiler(out, error, false);
  if (!compiler.CompileStore(store))
    return false;
  out->line_table_ = compiler.LineSpans();
  return true;
}

bool CompileDirect(const std::string& text, Program* out, CompileError* error) {
  out->instrs_.clear();
  out->line_table_.clear();
  out->jump_tables_.clear();
  out->data_items_.clear();
  out->line_data_index_.clear();
  out->user_funcs_.clear();
  out->procedures_.clear();
  out->procedure_index_.clear();
  out->constants_.clear();
  out->statement_pcs_.clear();
  Compiler compiler(out, error, true);
  if (!compiler.CompileDirectSource(text))
    return false;
  out->line_table_ = compiler.LineSpans();
  return true;
}

} // namespace newbasic
