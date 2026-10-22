#include "newbasic/frontend.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
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
    "CALL", "CHDIR", "COM", "ENVIRON", "FILES", "INP",   "IOCTL", "KILL", "MKDIR",  "MOTOR",
    "NAME", "OUT",   "PEN", "POKE",    "RMDIR", "STICK", "STRIG", "USR",  "VARPTR", "WAIT",
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
    {"STR$", Func::Str},
    {"STRING$", Func::String},
    {"TAN", Func::Tan},
    {"TIME$", Func::Time},
    {"TIMER", Func::Timer},
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
    const int index = static_cast<unsigned char>(name[0]) - 'A';
    const char suffix = (index >= 0 && index < 26) ? default_type_[index] : '!';
    return name + suffix;
  }

  bool CompileStore(const LineStore& store) {
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
  void AddLineFixup(int instr_index, int line) {
    Fixup fixup;
    fixup.instr_index = instr_index;
    fixup.line = line;
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
    if (keyword == "GOTO") {
      Advance();
      return CompileLineTarget(Op::Jump);
    }
    if (keyword == "GOSUB") {
      Advance();
      return CompileLineTarget(Op::Gosub);
    }
    if (keyword == "RETURN") {
      Advance();
      Emit(Op::Return);
      return true;
    }
    if (keyword == "ON") {
      Advance();
      return CompileOn();
    }
    if (keyword == "END") {
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
    if (keyword == "KEY") {
      Advance();
      if (TakeKeyword("ON")) {
        Emit(Op::KeyOn);
        return true;
      }
      if (TakeKeyword("OFF")) {
        Emit(Op::KeyOff);
        return true;
      }
      if (TakeKeyword("LIST")) {
        Emit(Op::KeyList);
        return true;
      }
      if (IsOp("(")) {
        // KEY(n) ON/OFF/STOP: arm or disarm the trapping of function key n.
        Advance();
        if (Cur().kind != Tok::Number)
          return Syntax("Expected a key number after KEY(");
        const int key = static_cast<int>(std::lround(Cur().number.AsNumber()));
        Advance();
        if (!ExpectOp(")"))
          return false;
        int action = 1;
        if (TakeKeyword("OFF")) {
          action = 0;
        } else if (TakeKeyword("STOP")) {
          action = 2;
        } else if (!TakeKeyword("ON")) {
          return Syntax("Expected ON, OFF or STOP after KEY(n)");
        }
        Instr in;
        in.op = Op::KeyControl;
        in.a = key;
        in.b = action;
        out_->instrs_.push_back(in);
        return true;
      }
      // KEY n, string
      if (!ParseExpression())
        return false;
      if (!TakeOp(","))
        return Syntax("Expected ',' after the key number");
      if (!ParseExpression())
        return false;
      Emit(Op::KeySet);
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
        const double first = Cur().number.AsNumber();
        Advance();
        in.a = static_cast<int>(first);
      }
      if (TakeOp("-")) {
        in.b = 1; // a range was given; the end defaults to the last line
        if (Cur().kind == Tok::Number) {
          const double last = Cur().number.AsNumber();
          Advance();
          in.b = static_cast<int>(last);
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
      const double first = Cur().number.AsNumber();
      Advance();
      int last = static_cast<int>(first);
      if (TakeOp("-")) {
        if (Cur().kind == Tok::Number) {
          last = static_cast<int>(Cur().number.AsNumber());
          Advance();
        } else {
          last = 32767; // DELETE first-  removes to the end
        }
      }
      Instr in;
      in.op = Op::Delete;
      in.a = static_cast<int>(first);
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
      // PALETTE [attribute, colour]: no arguments restores the default palette.
      Advance();
      Instr in;
      in.op = Op::Palette;
      in.a = 0;
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
      const int target = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      const int index = EmitPlaceholder(Op::Resume);
      out_->instrs_[static_cast<std::size_t>(index)].b = 2;
      AddLineFixup(index, target);
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
      in.a = static_cast<int>(std::lround(Cur().number.AsNumber()));
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
      if (!names.empty())
        names += ',';
      names += TypeName(Cur().text);
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

  bool CompileLet() {
    std::string name;
    bool array = false;
    int subscripts = 0;
    if (!ParseVariableRef(&name, &array, &subscripts))
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
      if (!CompileLineTarget(plain_gosub ? Op::Gosub : Op::Jump))
        return false;
      PatchTarget(false_jump, Pc());
      return true;
    }
    if (!TakeKeyword("THEN"))
      return Syntax("Expected THEN");
    const int false_jump = EmitPlaceholder(Op::JumpIfFalse);
    if (Cur().kind == Tok::Number) {
      // `IF expr THEN line` is an implicit GOTO.
      if (direct_)
        return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
      const int line = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      AddLineFixup(EmitPlaceholder(Op::Jump), line);
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
    const std::string name = Cur().text;
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

  // ---------------------------------------------------------------- jumps
  bool CompileLineTarget(Op op) {
    if (Cur().kind != Tok::Number)
      return Syntax("Expected a line number");
    const int target_line = static_cast<int>(std::lround(Cur().number.AsNumber()));
    Advance();
    if (direct_)
      return Error(kErrIllegalDirect, ErrorMessage(kErrIllegalDirect));
    AddLineFixup(EmitPlaceholder(op), target_line);
    return true;
  }

  bool CompileOn() {
    if (IsKeyword("KEY")) {
      // ON KEY(n) GOSUB line
      Advance();
      if (!ExpectOp("("))
        return false;
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a key number after ON KEY(");
      const int key = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      if (!ExpectOp(")"))
        return false;
      if (!TakeKeyword("GOSUB"))
        return Syntax("Expected GOSUB after ON KEY(n)");
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number");
      const int target = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      // The line operand is filled in by the fixup; the key number travels in b.
      const int index = EmitPlaceholder(Op::OnKey);
      out_->instrs_[static_cast<std::size_t>(index)].b = key;
      AddLineFixup(index, target);
      return true;
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
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number");
      const int target = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      AddLineFixup(EmitPlaceholder(Op::OnTimer), target);
      return true;
    }
    if (IsKeyword("ERROR")) {
      Advance();
      if (!TakeKeyword("GOTO"))
        return Syntax("Expected GOTO after ON ERROR");
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number");
      const int target = static_cast<int>(std::lround(Cur().number.AsNumber()));
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
      AddLineFixup(EmitPlaceholder(Op::OnError), target);
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
      if (Cur().kind != Tok::Number)
        return Syntax("Expected a line number");
      const int target_line = static_cast<int>(std::lround(Cur().number.AsNumber()));
      Advance();
      out_->jump_tables_[static_cast<std::size_t>(table_index)].push_back(-1);
      Fixup fixup;
      fixup.instr_index = -1;
      fixup.table = table_index;
      fixup.slot = slot;
      fixup.line = target_line;
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
    Emit(Op::End); // implicit END when execution falls off the program
    for (const Fixup& fixup : fixups_) {
      int target = -1;
      const auto it = line_starts_.find(fixup.line);
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
  std::vector<Fixup> fixups_;
  std::vector<LineSpan> line_spans_;
  std::map<int, int> line_starts_;
};

} // namespace

bool IsIllegalDirect(const std::string& keyword) {
  static const char* const kIllegalDirect[] = {
      "DATA", "DEF",  "DIM",    "DELETE", "ERASE", "FOR",   "GOSUB",
      "GOTO", "NEXT", "RESUME", "RETURN", "WEND",  "WHILE",
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
  out->statement_pcs_.clear();
  Compiler compiler(out, error, true);
  if (!compiler.CompileDirectSource(text))
    return false;
  out->line_table_ = compiler.LineSpans();
  return true;
}

} // namespace newbasic
