#include "lwbasic/vm.h"

#include "lwbasic/sound.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>

#include "lwbasic/errors.h"
#include "lwbasic/field.h"
#include "lwbasic/frontend.h"
#include "lwbasic/line_store.h"
#include "lwbasic/number_format.h"
#include "lwbasic/program_files.h"
#include "lwbasic/using.h"

namespace lwbasic {
namespace {

// Default RND seed: GW-BASIC's generator is reproducible until RANDOMIZE, so
// the VM always starts from a fixed seed.
constexpr std::uint32_t kDefaultSeed = 0x5A17u;

constexpr std::size_t kMaxStringLength = 255; // GW-BASIC string limit

Value::Kind PromoteKind(Value::Kind a, Value::Kind b) {
  if (a == Value::Kind::Dbl || b == Value::Kind::Dbl)
    return Value::Kind::Dbl;
  if (a == Value::Kind::Sgl || b == Value::Kind::Sgl)
    return Value::Kind::Sgl;
  if (a == Value::Kind::Long || b == Value::Kind::Long)
    return Value::Kind::Long;
  return Value::Kind::Int;
}

bool IsIntegral(Value::Kind k) {
  return k == Value::Kind::Int || k == Value::Kind::Long;
}

Value MakeNum(Value::Kind kind, double v) {
  switch (kind) {
  case Value::Kind::Int:
    return Value::Int(static_cast<std::int16_t>(v));
  case Value::Kind::Long:
    return Value::Long(static_cast<std::int32_t>(v));
  case Value::Kind::Dbl:
    return Value::Dbl(v);
  case Value::Kind::Sgl:
  case Value::Kind::Str:
  default:
    return Value::Sgl(v);
  }
}

// Rounds to the nearest integer, ties away from zero (GW-BASIC's conversion
// rounding).
double RoundToInt(double v) {
  return v < 0.0 ? -std::floor(-v + 0.5) : std::floor(v + 0.5);
}

std::string Trim(const std::string& text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end && (text[begin] == ' ' || text[begin] == '\t'))
    ++begin;
  while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t'))
    --end;
  return text.substr(begin, end - begin);
}

// Splits INPUT data on commas outside quotes; quotes are removed.
std::vector<std::string> ParseInputFields(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool in_quotes = false;
  for (char c : line) {
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (c == ',' && !in_quotes) {
      fields.push_back(Trim(current));
      current.clear();
      continue;
    }
    current += c;
  }
  fields.push_back(Trim(current));
  return fields;
}

// Parses a numeric field; false when the text is not a number.
bool ParseFieldNumber(const std::string& field, double* out) {
  if (field.empty())
    return false;
  const char* begin = field.c_str();
  char* end = nullptr;
  const double v = std::strtod(begin, &end);
  if (end == begin)
    return false;
  while (*end == ' ' || *end == '\t')
    ++end;
  if (*end != '\0')
    return false;
  *out = v;
  return true;
}

std::vector<std::string> SplitCommas(const std::string& text) {
  std::vector<std::string> parts;
  std::string current;
  for (char c : text) {
    if (c == ',') {
      parts.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  parts.push_back(current);
  return parts;
}

} // namespace

const char* FuncName(Func f) {
  switch (f) {
  case Func::Abs:
    return "ABS";
  case Func::Asc:
    return "ASC";
  case Func::Atn:
    return "ATN";
  case Func::Cdbl:
    return "CDBL";
  case Func::Chr:
    return "CHR$";
  case Func::Cint:
    return "CINT";
  case Func::Cos:
    return "COS";
  case Func::Csng:
    return "CSNG";
  case Func::Csrlin:
    return "CSRLIN";
  case Func::Cvd:
    return "CVD";
  case Func::Cvi:
    return "CVI";
  case Func::Cvs:
    return "CVS";
  case Func::Mkd:
    return "MKD$";
  case Func::Mki:
    return "MKI$";
  case Func::Mks:
    return "MKS$";
  case Func::Lof:
    return "LOF";
  case Func::Loc:
    return "LOC";
  case Func::Date:
    return "DATE$";
  case Func::Exp:
    return "EXP";
  case Func::Fix:
    return "FIX";
  case Func::Hex:
    return "HEX$";
  case Func::Inkey:
    return "INKEY$";
  case Func::Instr:
    return "INSTR";
  case Func::Int:
    return "INT";
  case Func::Left:
    return "LEFT$";
  case Func::Len:
    return "LEN";
  case Func::Log:
    return "LOG";
  case Func::Mid:
    return "MID$";
  case Func::Oct:
    return "OCT$";
  case Func::Pos:
    return "POS";
  case Func::Right:
    return "RIGHT$";
  case Func::Rnd:
    return "RND";
  case Func::Sgn:
    return "SGN";
  case Func::Sin:
    return "SIN";
  case Func::Space:
    return "SPACE$";
  case Func::Sqr:
    return "SQR";
  case Func::Str:
    return "STR$";
  case Func::String:
    return "STRING$";
  case Func::Tan:
    return "TAN";
  case Func::Time:
    return "TIME$";
  case Func::Timer:
    return "TIMER";
  case Func::Err:
    return "ERR";
  case Func::Eof:
    return "EOF";
  case Func::Erl:
    return "ERL";
  case Func::Val:
    return "VAL";
  case Func::Fre:
    return "FRE";
  case Func::Inp:
    return "INP";
  case Func::Peek:
    return "PEEK";
  case Func::Point:
    return "POINT";
  case Func::Pmap:
    return "PMAP";
  case Func::Mouse:
    return "MOUSE";
  case Func::MouseX:
    return "MOUSEX";
  case Func::MouseY:
    return "MOUSEY";
  case Func::MouseB:
    return "MOUSEB";
  case Func::Stick:
    return "STICK";
  case Func::Strig:
    return "STRIG";
  case Func::Usr:
    return "USR";
  case Func::Varptr:
    return "VARPTR";
  default:
    return "?";
  }
}

bool FuncIsString(Func f) {
  switch (f) {
  case Func::Chr:
  case Func::Date:
  case Func::Hex:
  case Func::Inkey:
  case Func::Left:
  case Func::Mid:
  case Func::Mkd:
  case Func::Mki:
  case Func::Mks:
  case Func::Oct:
  case Func::Right:
  case Func::Space:
  case Func::Str:
  case Func::String:
  case Func::Time:
    return true;
  default:
    return false;
  }
}

int FuncArity(Func f) {
  switch (f) {
  case Func::Csrlin:
  case Func::Date:
  case Func::Err:
  case Func::Erl:
  case Func::Inkey:
  case Func::Time:
  case Func::Timer:
    return 0;
  case Func::Instr: // INSTR([start,] haystack, needle)
  case Func::Mid:   // MID$(s, start[, length])
  case Func::Rnd:   // RND[(n)]
  case Func::Point: // POINT(x,y) or POINT(n)
    return -1;
  case Func::Mouse: // MOUSE(n): 0 button, 1 x, 2 y
    return 1;
  case Func::MouseX:
  case Func::MouseY:
  case Func::MouseB:
    return 0;
  case Func::Left:
  case Func::Right:
  case Func::String:
  case Func::Pmap: // PMAP(x, which)
    return 2;
  default:
    return 1;
  }
}

std::string CanonicalVarName(const std::string& name) {
  std::string upper = name;
  for (char& c : upper) {
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  }
  const char last = upper.empty() ? '\0' : upper.back();
  if (last != '%' && last != '&' && last != '!' && last != '#' && last != '$')
    upper += '!';
  return upper;
}

char VarTypeSuffix(const std::string& name) {
  const char last = name.empty() ? '\0' : name.back();
  switch (last) {
  case '%':
  case '&':
  case '!':
  case '#':
  case '$':
    return last;
  default:
    return '!';
  }
}

std::optional<std::pair<int, int>> Program::PcRangeForLine(int line) const {
  for (const LineSpan& span : line_table_) {
    if (span.line == line)
      return std::make_pair(span.pc_begin, span.pc_end);
    if (span.line > line)
      break;
  }
  return std::nullopt;
}

std::optional<int> Program::LineForPc(int pc) const {
  for (const LineSpan& span : line_table_) {
    if (span.pc_begin <= pc && pc <= span.pc_end)
      return span.line;
    if (span.pc_begin > pc)
      break;
  }
  return std::nullopt;
}

Vm::Vm(screen::Screen& screen, Host* host, std::atomic<bool>* break_flag)
    : screen_(screen), host_(host != nullptr ? host : &default_host_), break_flag_(break_flag) {
  rng_.seed(kDefaultSeed);
}

void Vm::CloseAllFiles() {
  for (auto& entry : files_)
    entry.second.close();
  files_.clear();
  print_file_ = -1;
  write_mode_ = false;
  print_buffer_.clear();
}

// WINDOW maps a world rectangle onto the canvas. Without a WINDOW the coordinates are
// canvas pixels already, so the mapping is the identity. In the default form y grows
// upward (y1 is the bottom); with the SCREEN keyword y grows downward.
void Vm::MapGraphicsPoint(double x, double y, int* sx, int* sy) const {
  const int view_x = screen_.ViewX();
  const int view_y = screen_.ViewY();
  if (!window_active_) {
    *sx = view_x + static_cast<int>(std::lround(x));
    *sy = view_y + static_cast<int>(std::lround(y));
    return;
  }
  const double width = static_cast<double>(screen_.ViewWidth());
  const double height = static_cast<double>(screen_.ViewHeight());
  const double span_x = window_x2_ - window_x1_;
  const double span_y = window_y2_ - window_y1_;
  const double nx = span_x == 0.0 ? 0.0 : (x - window_x1_) / span_x;
  const double ny = span_y == 0.0 ? 0.0 : (y - window_y1_) / span_y;
  *sx = view_x + static_cast<int>(std::lround(nx * (width - 1.0)));
  *sy = view_y + (window_screen_ ? static_cast<int>(std::lround(ny * (height - 1.0)))
                                 : static_cast<int>(std::lround((1.0 - ny) * (height - 1.0))));
}

void Vm::Load(Program program) {
  CloseAllFiles();
  program_ = std::move(program);
  pc_ = 0;
  stack_.clear();
  for_stack_.clear();
  gosub_stack_.clear();
  func_stack_.clear();
  data_index_ = 0;
  ResetPrintState();
  last_ = RunResult{};
}

// CLEAR: variables, arrays, stacks, files and the DATA pointer. The program and
// the instruction pointer stay put, which is what CLEAR does mid-program.
void Vm::ClearState() {
  CloseAllFiles();
  vars_.clear();
  arrays_.clear();
  stack_.clear();
  for_stack_.clear();
  gosub_stack_.clear();
  func_stack_.clear();
  data_index_ = 0;
  ResetPrintState();
}

void Vm::Reset() {
  ClearState();
  pc_ = 0;
  last_ = RunResult{};
}

void Vm::ResetPrintState() {
  at_line_start_ = true;
  pending_sep_ = Sep::None;
}

void Vm::SeedRandom(std::uint32_t seed) {
  rng_.seed(seed);
}

Value Vm::Pop() {
  if (stack_.empty()) {
    // Defensive: never read past an empty stack.
    return Value::Int(0);
  }
  Value v = stack_.back();
  stack_.pop_back();
  return v;
}

void Vm::Push(const Value& v) {
  stack_.push_back(v);
}

bool Vm::Fail(int code) {
  return Fail(code, ErrorMessage(code));
}

bool Vm::Fail(int code, const std::string& message) {
  last_.status = RunResult::Status::Error;
  last_.line = LineAtPc(pc_ - 1);
  last_.code = code;
  last_.message = message;
  // ON ERROR GOTO <line>: record ERR/ERL, jump to the handler and keep
  // running. An error raised inside the handler is not trapped again.
  if (!handling_error_ && error_handler_pc_ >= 0) {
    last_error_code_ = code;
    last_error_line_ = last_.line;
    error_pc_ = StatementStart(pc_ - 1);
    error_next_pc_ = StatementNext(pc_ - 1);
    handling_error_ = true;
    trap_pending_ = true;
    pc_ = error_handler_pc_;
    stack_.clear(); // abandon the failed expression
    last_.status = RunResult::Status::Done;
    last_.code = 0;
    last_.message.clear();
  }
  return false;
}

bool Vm::Number(const Value& v, double* out) {
  if (v.IsString())
    return Fail(kErrTypeMismatch);
  *out = v.AsNumber();
  return true;
}

bool Vm::String(const Value& v, std::string* out) {
  if (!v.IsString())
    return Fail(kErrTypeMismatch);
  *out = v.AsString();
  return true;
}

std::optional<Value> Vm::GetVar(const std::string& name) const {
  const auto it = vars_.find(CanonicalVarName(name));
  if (it == vars_.end())
    return std::nullopt;
  return it->second;
}

void Vm::SetVar(const std::string& name, const Value& value) {
  const std::string key = CanonicalVarName(name);
  Value converted;
  if (ConvertForName(key, value, &converted))
    vars_[key] = converted;
}

bool Vm::ConvertForName(const std::string& name, const Value& v, Value* out) {
  const char suffix = VarTypeSuffix(name);
  if (suffix == '$') {
    if (!v.IsString())
      return Fail(kErrTypeMismatch);
    *out = v;
    return true;
  }
  if (v.IsString())
    return Fail(kErrTypeMismatch);
  const double d = v.AsNumber();
  switch (suffix) {
  case '%':
    if (d < -32768.0 || d > 32767.0)
      return Fail(kErrOverflow);
    *out = Value::Int(static_cast<std::int16_t>(RoundToInt(d)));
    return true;
  case '&':
    if (d < -2147483648.0 || d > 2147483647.0)
      return Fail(kErrOverflow);
    *out = Value::Long(static_cast<std::int32_t>(RoundToInt(d)));
    return true;
  case '#':
    *out = Value::Dbl(d);
    return true;
  case '!':
  default:
    *out = Value::Sgl(d);
    return true;
  }
}

bool Vm::StoreVariable(const std::string& name, const Value& v) {
  const std::string key = CanonicalVarName(name);
  Value converted;
  if (!ConvertForName(key, v, &converted))
    return false;
  vars_[key] = converted;
  return true;
}

Value Vm::LoadVariable(const std::string& name) {
  const std::string key = CanonicalVarName(name);
  const auto it = vars_.find(key);
  if (it != vars_.end())
    return it->second;
  const Value initial = VarTypeSuffix(key) == '$' ? Value::Str(std::string()) : Value::Sgl(0.0);
  vars_[key] = initial;
  return initial;
}

Value::Kind ArrayKindForName(const std::string& name) {
  switch (VarTypeSuffix(name)) {
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

ArrayValue* Vm::FindArray(const std::string& name) {
  const auto it = arrays_.find(CanonicalVarName(name));
  return it == arrays_.end() ? nullptr : &it->second;
}

ArrayValue* Vm::EnsureArray(const std::string& name, const std::vector<int>& upper_bounds,
                            int base) {
  const std::string key = CanonicalVarName(name);
  auto it = arrays_.find(key);
  if (it != arrays_.end())
    return &it->second;
  ArrayValue array;
  array.kind = ArrayKindForName(key);
  array.base = base;
  array.dims = upper_bounds.empty() ? std::vector<int>{10} : upper_bounds;
  // Guard against absurd arrays: a 15-dimension auto-created array would
  // otherwise try to allocate 11^15 elements and abort the process.
  constexpr std::size_t kMaxElements = 1u << 20;
  if (array.dims.size() > 60)
    return nullptr;
  std::size_t total = 1;
  for (int upper : array.dims) {
    if (upper < array.base)
      return nullptr;
    const std::size_t extent = static_cast<std::size_t>(upper - array.base + 1);
    if (extent == 0 || total > kMaxElements / extent)
      return nullptr;
    total *= extent;
  }
  const Value initial =
      array.kind == Value::Kind::Str ? Value::Str(std::string()) : Value::Sgl(0.0);
  array.data.assign(total, initial);
  it = arrays_.emplace(key, std::move(array)).first;
  return &it->second;
}

bool Vm::ArrayOffset(const ArrayValue& array, const std::vector<int>& subscripts,
                     std::size_t* out) {
  if (subscripts.size() != array.dims.size())
    return false;
  std::size_t offset = 0;
  for (std::size_t i = 0; i < subscripts.size(); ++i) {
    const int extent = array.dims[i] - array.base + 1;
    const int index = subscripts[i] - array.base;
    if (index < 0 || index >= extent)
      return false;
    offset = offset * static_cast<std::size_t>(extent) + static_cast<std::size_t>(index);
  }
  *out = offset;
  return true;
}

// WRITE format: strings are quoted (doubled quotes escaped) and numbers have
// no sign space (User's Guide: WRITE differs from PRINT in those two ways).
std::string FormatWrite(const Value& v) {
  if (v.IsString()) {
    std::string out = "\"";
    for (char c : v.AsString()) {
      if (c == '"')
        out += '"';
      out += c;
    }
    out += '"';
    return out;
  }
  std::string text = FormatNumber(v);
  if (!text.empty() && text[0] == ' ')
    text.erase(0, 1);
  return text;
}

// Draws the function-key bar on the last screen line: the key number followed by
// the first six characters of its value, ten keys at width 80 and five at width
// 40, as the reference describes. The reference does not specify the spacing, so
// each key is given eight columns.
void Vm::DrawKeyBar() {
  if (!key_bar_)
    return;
  const int row = screen_.Height() - 1;
  for (int col = 0; col < screen_.Width(); ++col)
    screen_.PutCharAt(row, col, L' ');
  const int per_key = 8;
  const int visible = std::min(10, screen_.Width() / per_key);
  for (int i = 0; i < visible; ++i) {
    const std::string& value = key_values_[static_cast<std::size_t>(i)];
    const std::string label = std::to_string(i + 1) + value.substr(0, 6);
    for (std::size_t c = 0; c < label.size() && static_cast<int>(c) < per_key; ++c) {
      screen_.PutCharAt(row, i * per_key + static_cast<int>(c),
                        static_cast<wchar_t>(static_cast<unsigned char>(label[c])));
    }
  }
}

bool Vm::EmitValue(const Value& v) {
  if (using_active_) {
    // PRINT USING: format the value against the next field of the format string,
    // which cycles when exhausted. Unsupported field constructs are reported
    // rather than emitted wrongly.
    std::string text;
    const UsingStatus status = AppendUsing(&text, using_format_, &using_pos_, v);
    if (status == UsingStatus::Unsupported) {
      return Fail(kErrAdvancedFeature, "this PRINT USING format is not implemented yet");
    }
    // A string in a numeric field (or the reverse) is a type mismatch, which is the
    // reference's error; a bare Illegal function call sends the reader looking in the
    // wrong place.
    if (status != UsingStatus::Ok)
      return Fail(kErrTypeMismatch);
    if ((print_file_ >= 0 || printer_mode_) && !write_mode_) {
      print_buffer_ += text;
    } else if (print_file_ < 0) {
      screen_.WriteAtCursor(screen::DecodeCp437(text));
    } else {
      print_buffer_ += FormatWrite(v); // WRITE has no USING form
    }
    pending_sep_ = Sep::None;
    at_line_start_ = false;
    return true;
  }
  if (print_file_ >= 0 || printer_mode_) { // PRINT#/WRITE#/LPRINT: build the record
    if (write_mode_ && print_file_ >= 0) {
      print_buffer_ += FormatWrite(v);
    } else {
      print_buffer_ += FormatForPrint(v);
      if (v.IsNumeric())
        print_buffer_ += ' '; // PRINT puts a space after numbers
    }
    pending_sep_ = Sep::None;
    at_line_start_ = false;
    return true;
  }
  if (!at_line_start_ && pending_sep_ == Sep::Comma)
    screen_.AdvanceToNextZone();
  const std::string formatted = FormatForPrint(v);
  screen_.WriteAtCursor(screen::DecodeCp437(formatted));
  if (v.IsNumeric())
    screen_.WriteAtCursor(L" ");
  pending_sep_ = Sep::None;
  at_line_start_ = false;
  return true;
}

bool Vm::DoForStart(const Instr& in) {
  const Value step_value = Pop();
  const Value end_value = Pop();
  const Value start_value = Pop();
  double start = 0.0;
  double end = 0.0;
  double step = 0.0;
  if (!Number(start_value, &start) || !Number(end_value, &end) || !Number(step_value, &step)) {
    return false;
  }
  if (!StoreVariable(in.name, start_value))
    return false;
  const bool skip = (step >= 0.0 && start > end) || (step < 0.0 && start < end);
  if (skip) {
    if (in.a < 0)
      return Fail(kErrUndefinedLineNumber);
    pc_ = in.a;
    return true;
  }
  ForFrame frame;
  frame.name = CanonicalVarName(in.name);
  frame.end = end;
  frame.step = step;
  frame.body_pc = pc_;
  for_stack_.push_back(frame);
  return true;
}

bool Vm::DoForNext(const Instr& in) {
  if (for_stack_.empty())
    return Fail(kErrNextWithoutFor);
  std::size_t index = for_stack_.size() - 1;
  if (!in.name.empty()) {
    const std::string key = CanonicalVarName(in.name);
    bool found = false;
    for (std::size_t i = for_stack_.size(); i-- > 0;) {
      if (for_stack_[i].name == key) {
        index = i;
        found = true;
        break;
      }
    }
    if (!found)
      return Fail(kErrNextWithoutFor);
  }
  const ForFrame frame = for_stack_[index];
  const Value current = LoadVariable(frame.name);
  const double next = current.AsNumber() + frame.step;
  Value stored;
  switch (current.kind()) {
  case Value::Kind::Int:
    if (next < -32768.0 || next > 32767.0)
      return Fail(kErrOverflow);
    stored = Value::Int(static_cast<std::int16_t>(next));
    break;
  case Value::Kind::Long:
    if (next < -2147483648.0 || next > 2147483647.0)
      return Fail(kErrOverflow);
    stored = Value::Long(static_cast<std::int32_t>(next));
    break;
  case Value::Kind::Dbl:
    stored = Value::Dbl(next);
    break;
  default:
    stored = Value::Sgl(next);
    break;
  }
  vars_[frame.name] = stored;
  const bool again =
      (frame.step >= 0.0 && next <= frame.end) || (frame.step < 0.0 && next >= frame.end);
  if (again) {
    pc_ = frame.body_pc;
  } else {
    for_stack_.resize(index);
  }
  return true;
}

int Vm::StatementStart(int pc) const {
  int result = 0;
  for (int start : program_.statement_pcs_) {
    if (start <= pc)
      result = start;
    else
      break;
  }
  return result;
}

int Vm::StatementNext(int pc) const {
  for (int start : program_.statement_pcs_) {
    if (start > pc)
      return start;
  }
  return static_cast<int>(program_.instrs_.size());
}

bool Vm::DoRead(const Instr& in) {
  for (const std::string& name : SplitCommas(in.name)) {
    if (data_index_ >= program_.data_items_.size())
      return Fail(kErrOutOfData);
    const DataItem& item = program_.data_items_[data_index_++];
    if (VarTypeSuffix(name) == '$') {
      if (!StoreVariable(name, Value::Str(item.text)))
        return false;
      continue;
    }
    if (!item.numeric)
      return Fail(kErrSyntax);
    if (!StoreVariable(name, item.number))
      return false;
  }
  return true;
}

bool Vm::DoInput(const Instr& in, bool line_input) {
  // The compiled statement carries the prompt string (if any) as a constant operand; it is
  // printed here, once, before the first read. GW-BASIC does not repeat it after a redo.
  if (in.value.IsString() && !in.value.AsString().empty())
    screen_.WriteAtCursor(screen::DecodeCp437(in.value.AsString()));
  if (in.b != 0)
    screen_.WriteAtCursor(L"? ");
  const std::vector<std::string> names = SplitCommas(in.name);
  for (;;) {
    screen_.PresentNow(); // INPUT blocks: present the prompt first
    const std::wstring wide = screen_.ReadLine();
    screen_.NewLine();
    const std::string line = screen::EncodeCp437(wide);
    if (line_input) {
      if (names.empty())
        return true;
      return StoreVariable(names[0], Value::Str(line));
    }
    const std::vector<std::string> fields = ParseInputFields(line);
    if (fields.size() != names.size()) {
      screen_.WriteAtCursor(L"?Redo from start");
      screen_.NewLine();
      screen_.WriteAtCursor(L"? ");
      continue;
    }
    std::vector<Value> values;
    bool ok = true;
    for (std::size_t i = 0; i < fields.size(); ++i) {
      if (VarTypeSuffix(names[i]) == '$') {
        values.push_back(Value::Str(fields[i]));
        continue;
      }
      if (fields[i].empty()) {
        values.push_back(LoadVariable(names[i])); // an empty field keeps the old value
        continue;
      }
      double number = 0.0;
      if (!ParseFieldNumber(fields[i], &number)) {
        ok = false;
        break;
      }
      values.push_back(Value::Sgl(number));
    }
    if (!ok) {
      screen_.WriteAtCursor(L"?Redo from start");
      screen_.NewLine();
      screen_.WriteAtCursor(L"? ");
      continue;
    }
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (!StoreVariable(names[i], values[i]))
        return false;
    }
    return true;
  }
}

bool Vm::CallFunction(Func f, int argc) {
  std::vector<Value> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i)
    args.push_back(Pop());
  std::reverse(args.begin(), args.end());

  auto numeric = [&](std::size_t index, double* out) {
    if (index >= args.size())
      return false;
    return Number(args[index], out);
  };
  auto text = [&](std::size_t index, std::string* out) {
    if (index >= args.size())
      return false;
    return String(args[index], out);
  };
  // Trig/transcendental results keep Double precision when given a Double.
  auto push_float = [&](double v) {
    const bool wide = argc > 0 && args[0].kind() == Value::Kind::Dbl;
    Push(wide ? Value::Dbl(v) : Value::Sgl(v));
  };

  double a = 0.0;
  std::string s;
  switch (f) {
  case Func::Abs:
    if (!numeric(0, &a))
      return false;
    Push(MakeNum(args[0].kind(), std::fabs(a)));
    return true;
  case Func::Atn:
    if (!numeric(0, &a))
      return false;
    push_float(std::atan(a));
    return true;
  case Func::Cos:
    if (!numeric(0, &a))
      return false;
    push_float(std::cos(a));
    return true;
  case Func::Sin:
    if (!numeric(0, &a))
      return false;
    push_float(std::sin(a));
    return true;
  case Func::Tan:
    if (!numeric(0, &a))
      return false;
    push_float(std::tan(a));
    return true;
  case Func::Exp:
    if (!numeric(0, &a))
      return false;
    push_float(std::exp(a));
    return true;
  case Func::Log:
    if (!numeric(0, &a))
      return false;
    if (a <= 0.0)
      return Fail(kErrIllegalFunctionCall);
    push_float(std::log(a));
    return true;
  case Func::Sqr:
    if (!numeric(0, &a))
      return false;
    if (a < 0.0)
      return Fail(kErrIllegalFunctionCall);
    push_float(std::sqrt(a));
    return true;
  case Func::Int:
    if (!numeric(0, &a))
      return false;
    Push(MakeNum(args[0].kind(), std::floor(a)));
    return true;
  case Func::Fix:
    if (!numeric(0, &a))
      return false;
    Push(MakeNum(args[0].kind(), std::trunc(a)));
    return true;
  case Func::Sgn:
    if (!numeric(0, &a))
      return false;
    Push(Value::Int(a > 0.0 ? 1 : (a < 0.0 ? -1 : 0)));
    return true;
  case Func::Cint:
    if (!numeric(0, &a))
      return false;
    if (a < -32768.0 || a > 32767.0)
      return Fail(kErrOverflow);
    Push(Value::Int(static_cast<std::int16_t>(RoundToInt(a))));
    return true;
  case Func::Csng:
    if (!numeric(0, &a))
      return false;
    Push(Value::Sgl(a));
    return true;
  case Func::Cdbl:
    if (!numeric(0, &a))
      return false;
    Push(Value::Dbl(a));
    return true;
  case Func::Rnd:
    if (argc >= 1) {
      if (!numeric(0, &a))
        return false;
      if (a < 0.0) {
        rng_.seed(static_cast<std::uint32_t>(std::llround(a)));
      } else if (a == 0.0) {
        Push(Value::Sgl(last_rnd_));
        return true;
      }
    }
    {
      std::uniform_real_distribution<double> dist(0.0, 1.0);
      last_rnd_ = dist(rng_);
      has_rnd_ = true;
      Push(Value::Sgl(last_rnd_));
    }
    return true;
  case Func::Chr: {
    if (!numeric(0, &a))
      return false;
    const long code = std::lround(a);
    if (code < 0 || code > 255)
      return Fail(kErrIllegalFunctionCall);
    Push(Value::Str(std::string(1, static_cast<char>(static_cast<unsigned char>(code)))));
    return true;
  }
  case Func::Mouse:
  case Func::MouseX:
  case Func::MouseY:
  case Func::MouseB: {
    // Pointing device, an extension. A backend with no device reports no click, so a
    // program polling it simply waits, and the button reads 0.
    int which = 0;
    if (f == Func::MouseX)
      which = 1;
    else if (f == Func::MouseY)
      which = 2;
    else if (f == Func::Mouse) {
      if (!numeric(0, &a))
        return false;
      which = static_cast<int>(std::lround(a));
      if (which < 0 || which > 2)
        return Fail(kErrIllegalFunctionCall);
    }
    int button = 0;
    screen::Screen::MouseClick click;
    if (screen_.TakeMouseClick(&click)) {
      button = click.button;
      mouse_x_ = click.x;
      mouse_y_ = click.y;
    }
    if (which == 0)
      Push(Value::Int(static_cast<std::int16_t>(button)));
    else
      Push(Value::Int(static_cast<std::int16_t>(which == 1 ? mouse_x_ : mouse_y_)));
    return true;
  }
  case Func::Point: {
    // POINT(x,y) reads a pixel's palette index and returns -1 out of range;
    // POINT(n) with one argument returns the current graphics position (0 = x).
    if (argc == 1) {
      if (!numeric(0, &a))
        return false;
      const int which = static_cast<int>(std::lround(a));
      if (which != 0 && which != 1)
        return Fail(kErrIllegalFunctionCall);
      Push(Value::Sgl(which == 0 ? last_x_ : last_y_));
      return true;
    }
    if (!numeric(0, &a))
      return false;
    double y = 0.0;
    if (!numeric(1, &y))
      return false;
    int px = 0;
    int py = 0;
    MapGraphicsPoint(a, y, &px, &py);
    const screen::Canvas& point_canvas = screen_.Graphics();
    if (px < 0 || py < 0 || px >= point_canvas.Width() || py >= point_canvas.Height()) {
      Push(Value::Int(-1));
      return true;
    }
    Push(Value::Int(static_cast<std::int16_t>(point_canvas.Pixel(px, py))));
    return true;
  }
  case Func::Pmap: {
    // PMAP(x, which): 0 and 1 map logical (world) to physical (screen), 2 and 3 map
    // physical back to logical. 0 and 2 are the x axis, 1 and 3 the y axis.
    if (!numeric(0, &a))
      return false;
    double which_arg = 0.0;
    if (!numeric(1, &which_arg))
      return false;
    const int which = static_cast<int>(std::lround(which_arg));
    if (which < 0 || which > 3)
      return Fail(kErrIllegalFunctionCall);
    if (!window_active_) { // the identity mapping
      Push(Value::Sgl(a));
      return true;
    }
    const screen::Canvas& canvas = screen_.Graphics();
    const bool horizontal = which == 0 || which == 2;
    const bool to_physical = which < 2;
    const double span = horizontal ? window_x2_ - window_x1_ : window_y2_ - window_y1_;
    const double low = horizontal ? window_x1_ : window_y1_;
    const double extent = horizontal ? static_cast<double>(canvas.Width() - 1)
                                     : static_cast<double>(canvas.Height() - 1);
    if (to_physical) {
      const double normalised = span == 0.0 ? 0.0 : (a - low) / span;
      // The y axis runs downward on screen unless WINDOW's default flipped it.
      const double physical =
          (horizontal || window_screen_) ? normalised * extent : (1.0 - normalised) * extent;
      Push(Value::Sgl(physical));
    } else {
      const double normalised =
          extent == 0.0 ? 0.0 : (horizontal || window_screen_ ? a / extent : 1.0 - a / extent);
      Push(Value::Sgl(low + normalised * span));
    }
    return true;
  }
  case Func::Fre:
  case Func::Inp:
  case Func::Peek:
  case Func::Stick:
  case Func::Strig:
  case Func::Usr:
  case Func::Varptr:
    // M13 decision: refuse machine-level access clearly rather than inventing a
    // memory or port model that does not correspond to anything on the host.
    return Fail(kErrAdvancedFeature,
                std::string(FuncName(f)) + " needs a machine model that lwbasic does not have");
  case Func::Lof:
  case Func::Loc: {
    if (!numeric(0, &a))
      return false;
    const int handle = static_cast<int>(RoundToInt(a));
    const auto stream = files_.find(handle);
    if (stream == files_.end() || !stream->second.is_open())
      return Fail(kErrBadFileNumber);
    if (f == Func::Lof) {
      stream->second.clear();
      stream->second.seekg(0, std::ios::end);
      Push(Value::Long(static_cast<std::int32_t>(stream->second.tellg())));
    } else {
      const auto state = random_files_.find(handle);
      Push(Value::Long(
          static_cast<std::int32_t>(state == random_files_.end() ? 0 : state->second.last_record)));
    }
    return true;
  }
  case Func::Cvi:
  case Func::Cvs:
  case Func::Cvd:
  case Func::Mki:
  case Func::Mks:
  case Func::Mkd: {
    // MKI$/CVI (2 bytes), MKS$/CVS (4) and MKD$/CVD (8), per the reference. A
    // string of the wrong length is an Illegal function call.
    const bool numeric_form = (f == Func::Mki || f == Func::Mks || f == Func::Mkd);
    if (numeric_form) {
      if (!numeric(0, &a))
        return false;
      if (f == Func::Mki) {
        Push(Value::Str(MkiFromInt(static_cast<std::int16_t>(std::lround(a)))));
      } else if (f == Func::Mks) {
        Push(Value::Str(MksFromSingle(a)));
      } else {
        Push(Value::Str(MkdFromDouble(a)));
      }
      return true;
    }
    if (!text(0, &s))
      return false;
    if (f == Func::Cvi) {
      std::int16_t value = 0;
      if (!CviFromString(s, &value))
        return Fail(kErrIllegalFunctionCall);
      Push(Value::Int(value));
    } else if (f == Func::Cvs) {
      double value = 0.0;
      if (!CvsFromString(s, &value))
        return Fail(kErrIllegalFunctionCall);
      Push(MakeNum(Value::Kind::Sgl, value));
    } else {
      double value = 0.0;
      if (!CvdFromString(s, &value))
        return Fail(kErrIllegalFunctionCall);
      Push(MakeNum(Value::Kind::Dbl, value));
    }
    return true;
  }
  case Func::Str:
    if (!numeric(0, &a))
      return false;
    Push(Value::Str(FormatNumber(args[0])));
    return true;
  case Func::Len:
    if (!text(0, &s))
      return false;
    Push(Value::Int(static_cast<std::int16_t>(s.size())));
    return true;
  case Func::Asc:
    if (!text(0, &s))
      return false;
    if (s.empty())
      return Fail(kErrIllegalFunctionCall);
    Push(Value::Int(static_cast<std::int16_t>(static_cast<unsigned char>(s[0]))));
    return true;
  case Func::Val: {
    if (!text(0, &s))
      return false;
    const char* begin = s.c_str();
    char* end = nullptr;
    const double v = std::strtod(begin, &end);
    Push(Value::Sgl(end == begin ? 0.0 : v));
    return true;
  }
  case Func::Left: {
    if (!text(0, &s))
      return false;
    if (!numeric(1, &a))
      return false;
    const long n = std::lround(a);
    if (n < 0)
      return Fail(kErrIllegalFunctionCall);
    Push(Value::Str(s.substr(0, std::min<std::size_t>(static_cast<std::size_t>(n), s.size()))));
    return true;
  }
  case Func::Right: {
    if (!text(0, &s))
      return false;
    if (!numeric(1, &a))
      return false;
    const long n = std::lround(a);
    if (n < 0)
      return Fail(kErrIllegalFunctionCall);
    const std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(n), s.size());
    Push(Value::Str(s.substr(s.size() - count)));
    return true;
  }
  case Func::Mid: {
    if (args.size() < 2 || args.size() > 3)
      return Fail(kErrSyntax);
    if (!text(0, &s))
      return false;
    if (!numeric(1, &a))
      return false;
    const long start = std::lround(a);
    if (start < 1L)
      return Fail(kErrIllegalFunctionCall);
    const std::size_t begin = static_cast<std::size_t>(start - 1);
    if (begin >= s.size()) {
      Push(Value::Str(std::string()));
      return true;
    }
    std::size_t count = s.size() - begin;
    if (args.size() == 3) {
      double len_value = 0.0;
      if (!numeric(2, &len_value))
        return false;
      const long len = std::lround(len_value);
      if (len < 0)
        return Fail(kErrIllegalFunctionCall);
      count = std::min<std::size_t>(count, static_cast<std::size_t>(len));
    }
    Push(Value::Str(s.substr(begin, count)));
    return true;
  }
  case Func::Space: {
    if (!numeric(0, &a))
      return false;
    const long n = std::lround(a);
    if (n < 0)
      return Fail(kErrIllegalFunctionCall);
    if (n > static_cast<long>(kMaxStringLength))
      return Fail(kErrStringTooLong);
    Push(Value::Str(std::string(static_cast<std::size_t>(n), ' ')));
    return true;
  }
  case Func::String: {
    if (args.size() != 2)
      return Fail(kErrSyntax);
    if (!numeric(0, &a))
      return false;
    const long n = std::lround(a);
    if (n < 0)
      return Fail(kErrIllegalFunctionCall);
    if (n > static_cast<long>(kMaxStringLength))
      return Fail(kErrStringTooLong);
    char fill = ' ';
    if (args[1].IsString()) {
      if (args[1].AsString().empty())
        return Fail(kErrIllegalFunctionCall);
      fill = args[1].AsString()[0];
    } else {
      fill = static_cast<char>(std::lround(args[1].AsNumber()) & 0xFF);
    }
    Push(Value::Str(std::string(static_cast<std::size_t>(n), fill)));
    return true;
  }
  case Func::Hex: {
    if (!numeric(0, &a))
      return false;
    const long long v = static_cast<long long>(RoundToInt(a));
    char buf[32];
    if (args[0].kind() == Value::Kind::Int) {
      std::snprintf(buf, sizeof(buf), "%X", static_cast<unsigned>(v & 0xFFFF));
    } else {
      std::snprintf(buf, sizeof(buf), "%X", static_cast<unsigned>(v & 0xFFFFFFFFLL));
    }
    Push(Value::Str(buf));
    return true;
  }
  case Func::Oct: {
    if (!numeric(0, &a))
      return false;
    const long long v = static_cast<long long>(RoundToInt(a));
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%o", static_cast<unsigned>(v & 0xFFFF));
    Push(Value::Str(buf));
    return true;
  }
  case Func::Instr: {
    if (args.size() < 2 || args.size() > 3)
      return Fail(kErrSyntax);
    long start = 1;
    std::size_t haystack_index = 0;
    std::size_t needle_index = 1;
    if (args.size() == 3) {
      if (!numeric(0, &a))
        return false;
      start = std::lround(a);
      if (start < 1L)
        return Fail(kErrIllegalFunctionCall);
      haystack_index = 1;
      needle_index = 2;
    }
    std::string haystack;
    std::string needle;
    if (!text(haystack_index, &haystack) || !text(needle_index, &needle))
      return false;
    if (start > static_cast<long>(haystack.size())) {
      Push(Value::Int(0));
      return true;
    }
    const std::size_t found = haystack.find(needle, static_cast<std::size_t>(start - 1));
    Push(Value::Int(found == std::string::npos ? 0 : static_cast<std::int16_t>(found + 1)));
    return true;
  }
  case Func::Timer:
    Push(Value::Sgl(host_->SecondsSinceMidnight()));
    return true;
  case Func::Eof: {
    if (!numeric(0, &a))
      return false;
    const int eof_handle = static_cast<int>(RoundToInt(a));
    const auto eof_target = files_.find(eof_handle);
    if (eof_target == files_.end() || !eof_target->second.is_open()) {
      return Fail(kErrBadFileNumber);
    }
    Push(Value::Int(eof_target->second.peek() == EOF ? -1 : 0));
    return true;
  }
  case Func::Err:
    Push(Value::Int(static_cast<std::int16_t>(last_error_code_)));
    return true;
  case Func::Erl:
    Push(Value::Int(static_cast<std::int16_t>(last_error_line_)));
    return true;
  case Func::Date:
    Push(Value::Str(host_->DateString()));
    return true;
  case Func::Time:
    Push(Value::Str(host_->TimeString()));
    return true;
  case Func::Inkey: {
    wchar_t key = 0;
    // A key consumed to fire a KEY(n) trap is handed back here first, so the handler
    // can read the character without it being lost.
    if (pending_key_ != 0) {
      key = pending_key_;
      pending_key_ = 0;
      Push(Value::Str(screen::EncodeCp437(std::wstring(1, key))));
      return true;
    }
    if (screen_.TryGetKey(&key))
      Push(Value::Str(screen::EncodeCp437(std::wstring(1, key))));
    else
      Push(Value::Str(std::string()));
    return true;
  }
  case Func::Pos:
    Push(Value::Int(static_cast<std::int16_t>(screen_.CursorCol() + 1)));
    return true;
  case Func::Csrlin:
    Push(Value::Int(static_cast<std::int16_t>(screen_.CursorRow() + 1)));
    return true;
  default:
    return Fail(kErrIllegalFunctionCall);
  }
}

bool Vm::Step() {
  if (halted_)
    return false;
  const bool more = StepImpl();
  // Only a final halt latches: a *trapped* error also returns false here, but
  // Run() resumes at the ON ERROR handler, so the VM is not halted.
  if (!more && !trap_pending_)
    halted_ = true;
  return more;
}

bool Vm::StepImpl() {
  // Only the instruction that just failed may leave a trap pending; clear it
  // here so a stale flag can never make the Run loop spin.
  trap_pending_ = false;
  if (pc_ >= static_cast<int>(program_.instrs_.size())) {
    if (last_.status != RunResult::Status::Error) {
      last_.status = RunResult::Status::Done;
      last_.line = 0;
    }
    return false;
  }
  const Instr& in = program_.instrs_[static_cast<std::size_t>(pc_)];
  ++pc_;

  switch (in.op) {
  case Op::PushInt:
  case Op::PushLong:
  case Op::PushSgl:
  case Op::PushDbl:
  case Op::PushStr:
    Push(in.value);
    break;
  case Op::LoadVar:
    Push(LoadVariable(in.name));
    break;
  case Op::StoreVar:
    return StoreVariable(in.name, Pop());
  case Op::ArrayDim: {
    std::vector<int> upper_bounds(static_cast<std::size_t>(in.a > 0 ? in.a : 1));
    for (std::size_t i = upper_bounds.size(); i-- > 0;) {
      const Value v = Pop();
      double number = 0.0;
      if (!Number(v, &number))
        return false;
      upper_bounds[i] = static_cast<int>(RoundToInt(number));
    }
    const std::string key = CanonicalVarName(in.name);
    if (arrays_.count(key) != 0)
      return Fail(kErrDuplicateDefinition);
    if (EnsureArray(key, upper_bounds, in.b) == nullptr)
      return Fail(kErrSubscriptOutOfRange);
    break;
  }
  case Op::ArrayLoad:
  case Op::ArrayStore: {
    Value stored;
    if (in.op == Op::ArrayStore)
      stored = Pop(); // the value is pushed last
    std::vector<int> subscripts(static_cast<std::size_t>(in.a > 0 ? in.a : 0));
    for (std::size_t i = subscripts.size(); i-- > 0;) {
      const Value v = Pop();
      double number = 0.0;
      if (!Number(v, &number))
        return false;
      subscripts[i] = static_cast<int>(RoundToInt(number));
    }
    ArrayValue* array = FindArray(in.name);
    if (array == nullptr) {
      array = EnsureArray(in.name, std::vector<int>(subscripts.size(), 10), in.b);
      if (array == nullptr)
        return Fail(kErrSubscriptOutOfRange);
    }
    std::size_t offset = 0;
    if (!ArrayOffset(*array, subscripts, &offset))
      return Fail(kErrSubscriptOutOfRange);
    if (in.op == Op::ArrayStore) {
      Value converted;
      if (!ConvertForName(in.name, stored, &converted))
        return false;
      array->data[offset] = converted;
    } else {
      Push(array->data[offset]);
    }
    break;
  }
  case Op::Erase:
    for (const std::string& name : SplitCommas(in.name)) {
      arrays_.erase(CanonicalVarName(name));
    }
    break;
  case Op::Add:
  case Op::Sub:
  case Op::Mul: {
    const Value b = Pop();
    const Value a = Pop();
    if (a.IsString() || b.IsString()) {
      if (in.op == Op::Add && a.IsString() && b.IsString()) {
        const std::string joined = a.AsString() + b.AsString();
        if (joined.size() > kMaxStringLength)
          return Fail(kErrStringTooLong);
        Push(Value::Str(joined));
        break;
      }
      return Fail(kErrTypeMismatch);
    }
    const Value::Kind kind = PromoteKind(a.kind(), b.kind());
    if (IsIntegral(kind)) {
      const long long x = static_cast<long long>(RoundToInt(a.AsNumber()));
      const long long y = static_cast<long long>(RoundToInt(b.AsNumber()));
      const long long r = in.op == Op::Add ? x + y : (in.op == Op::Sub ? x - y : x * y);
      if (kind == Value::Kind::Int) {
        if (r < -32768 || r > 32767)
          return Fail(kErrOverflow);
        Push(Value::Int(static_cast<std::int16_t>(r)));
      } else {
        if (r < std::numeric_limits<std::int32_t>::min() ||
            r > std::numeric_limits<std::int32_t>::max()) {
          return Fail(kErrOverflow);
        }
        Push(Value::Long(static_cast<std::int32_t>(r)));
      }
      break;
    }
    const double x = a.AsNumber();
    const double y = b.AsNumber();
    const double r = in.op == Op::Add ? x + y : (in.op == Op::Sub ? x - y : x * y);
    if (std::isinf(r))
      return Fail(kErrOverflow);
    Push(MakeNum(kind, r));
    break;
  }
  case Op::Div: {
    const Value b = Pop();
    const Value a = Pop();
    double x = 0.0;
    double y = 0.0;
    if (!Number(a, &x) || !Number(b, &y))
      return false;
    if (y == 0.0)
      return Fail(kErrDivisionByZero);
    const Value::Kind kind = (a.kind() == Value::Kind::Dbl || b.kind() == Value::Kind::Dbl)
                                 ? Value::Kind::Dbl
                                 : Value::Kind::Sgl;
    Push(MakeNum(kind, x / y));
    break;
  }
  case Op::IntDiv:
  case Op::Mod: {
    const Value b = Pop();
    const Value a = Pop();
    double x = 0.0;
    double y = 0.0;
    if (!Number(a, &x) || !Number(b, &y))
      return false;
    const long long xi = static_cast<long long>(RoundToInt(x));
    const long long yi = static_cast<long long>(RoundToInt(y));
    if (yi == 0)
      return Fail(kErrDivisionByZero);
    const bool wide = a.kind() == Value::Kind::Long || b.kind() == Value::Kind::Long;
    if (in.op == Op::IntDiv) {
      const long long r = xi / yi;
      Push(wide ? Value::Long(static_cast<std::int32_t>(r))
                : Value::Int(static_cast<std::int16_t>(r)));
    } else {
      const long long r = xi % yi;
      Push(wide ? Value::Long(static_cast<std::int32_t>(r))
                : Value::Int(static_cast<std::int16_t>(r)));
    }
    break;
  }
  case Op::Pow: {
    const Value b = Pop();
    const Value a = Pop();
    double x = 0.0;
    double y = 0.0;
    if (!Number(a, &x) || !Number(b, &y))
      return false;
    if (x < 0.0 && y != std::trunc(y))
      return Fail(kErrIllegalFunctionCall);
    if (x == 0.0 && y < 0.0)
      return Fail(kErrDivisionByZero);
    const double r = std::pow(x, y);
    if (std::isinf(r))
      return Fail(kErrOverflow);
    const Value::Kind kind = (a.kind() == Value::Kind::Dbl || b.kind() == Value::Kind::Dbl)
                                 ? Value::Kind::Dbl
                                 : Value::Kind::Sgl;
    Push(MakeNum(kind, r));
    break;
  }
  case Op::Neg: {
    const Value a = Pop();
    double x = 0.0;
    if (!Number(a, &x))
      return false;
    if (a.kind() == Value::Kind::Int) {
      if (x <= -32768.0)
        return Fail(kErrOverflow);
      Push(Value::Int(static_cast<std::int16_t>(-x)));
    } else if (a.kind() == Value::Kind::Long) {
      Push(Value::Long(static_cast<std::int32_t>(-x)));
    } else {
      Push(MakeNum(a.kind(), -x));
    }
    break;
  }
  case Op::Eq:
  case Op::Ne:
  case Op::Lt:
  case Op::Gt:
  case Op::Le:
  case Op::Ge: {
    const Value b = Pop();
    const Value a = Pop();
    bool result = false;
    if (a.IsString() || b.IsString()) {
      if (!a.IsString() || !b.IsString())
        return Fail(kErrTypeMismatch);
      const int cmp = a.AsString().compare(b.AsString());
      switch (in.op) {
      case Op::Eq:
        result = cmp == 0;
        break;
      case Op::Ne:
        result = cmp != 0;
        break;
      case Op::Lt:
        result = cmp < 0;
        break;
      case Op::Gt:
        result = cmp > 0;
        break;
      case Op::Le:
        result = cmp <= 0;
        break;
      default:
        result = cmp >= 0;
        break;
      }
    } else {
      const double x = a.AsNumber();
      const double y = b.AsNumber();
      switch (in.op) {
      case Op::Eq:
        result = x == y;
        break;
      case Op::Ne:
        result = x != y;
        break;
      case Op::Lt:
        result = x < y;
        break;
      case Op::Gt:
        result = x > y;
        break;
      case Op::Le:
        result = x <= y;
        break;
      default:
        result = x >= y;
        break;
      }
    }
    Push(Value::Int(result ? -1 : 0));
    break;
  }
  case Op::And:
  case Op::Or:
  case Op::Xor:
  case Op::Eqv:
  case Op::Imp: {
    const Value b = Pop();
    const Value a = Pop();
    double x = 0.0;
    double y = 0.0;
    if (!Number(a, &x) || !Number(b, &y))
      return false;
    const long long xi = static_cast<long long>(RoundToInt(x));
    const long long yi = static_cast<long long>(RoundToInt(y));
    long long r = 0;
    switch (in.op) {
    case Op::And:
      r = xi & yi;
      break;
    case Op::Or:
      r = xi | yi;
      break;
    case Op::Xor:
      r = xi ^ yi;
      break;
    case Op::Eqv:
      r = ~(xi ^ yi);
      break;
    default:
      r = (~xi) | yi;
      break;
    }
    const Value::Kind kind = PromoteKind(a.kind(), b.kind());
    if (kind == Value::Kind::Int)
      Push(Value::Int(static_cast<std::int16_t>(r)));
    else if (kind == Value::Kind::Long)
      Push(Value::Long(static_cast<std::int32_t>(r)));
    else
      Push(MakeNum(kind, static_cast<double>(r)));
    break;
  }
  case Op::Not: {
    const Value a = Pop();
    double x = 0.0;
    if (!Number(a, &x))
      return false;
    const long long r = ~static_cast<long long>(RoundToInt(x));
    if (a.kind() == Value::Kind::Int)
      Push(Value::Int(static_cast<std::int16_t>(r)));
    else if (a.kind() == Value::Kind::Long)
      Push(Value::Long(static_cast<std::int32_t>(r)));
    else
      Push(MakeNum(a.kind(), static_cast<double>(r)));
    break;
  }
  case Op::CallFunc:
    return CallFunction(static_cast<Func>(in.a), in.b);
  case Op::Emit:
    return EmitValue(Pop());
  case Op::Using: {
    // PRINT USING: the format is a string expression evaluated at run time.
    const Value format = Pop();
    if (!format.IsString())
      return Fail(kErrTypeMismatch);
    using_format_ = format.AsString();
    using_pos_ = 0;
    using_active_ = true;
    return true;
  }
  case Op::SepComma:
    if (using_active_) { // separators only separate items in a USING list
      pending_sep_ = Sep::None;
      break;
    }
    if (print_file_ >= 0 || printer_mode_) {
      if (write_mode_ && print_file_ >= 0) {
        print_buffer_ += ',';
      } else {
        const std::size_t next = (print_buffer_.size() / 14 + 1) * 14;
        print_buffer_.append(next - print_buffer_.size(), ' ');
      }
      break;
    }
    pending_sep_ = Sep::Comma;
    break;
  case Op::SepSemi:
    if (using_active_) {
      pending_sep_ = Sep::None;
      break;
    }
    pending_sep_ = Sep::Semi;
    break;
  case Op::LineEnd:
    using_active_ = false;
    using_pos_ = 0;
    if (printer_mode_) {
      // LPRINT: hand the finished line to the printer.
      host_->WriteToPrinter(print_buffer_ + "\n");
      print_buffer_.clear();
      printer_mode_ = false;
      at_line_start_ = true;
      pending_sep_ = Sep::None;
      break;
    }
    if (print_file_ >= 0) {
      print_buffer_ += "\r\n";
      const auto target = files_.find(print_file_);
      if (target == files_.end() || !target->second.is_open())
        return Fail(kErrBadFileNumber);
      target->second << print_buffer_;
      if (!target->second)
        return Fail(kErrDeviceIOError);
      print_buffer_.clear();
      print_file_ = -1;
      write_mode_ = false;
      break;
    }
    screen_.NewLine();
    at_line_start_ = true;
    pending_sep_ = Sep::None;
    break;
  case Op::NoLineEnd:
    using_active_ = false;
    using_pos_ = 0;
    at_line_start_ = false;
    break;
  case Op::Tab: {
    const Value n = Pop();
    double value = 0.0;
    if (!Number(n, &value))
      return false;
    if (!at_line_start_ && pending_sep_ == Sep::Comma)
      screen_.AdvanceToNextZone();
    pending_sep_ = Sep::None;
    long column = std::lround(value) - 1;
    if (column < 0)
      return Fail(kErrIllegalFunctionCall);
    column %= screen_.Width();
    if (column <= screen_.CursorCol())
      screen_.NewLine();
    screen_.SetCursor(screen_.CursorRow(), static_cast<int>(column));
    at_line_start_ = false;
    break;
  }
  case Op::Spc: {
    const Value n = Pop();
    double value = 0.0;
    if (!Number(n, &value))
      return false;
    const long count = std::lround(value);
    if (count < 0)
      return Fail(kErrIllegalFunctionCall);
    if (!at_line_start_ && pending_sep_ == Sep::Comma)
      screen_.AdvanceToNextZone();
    pending_sep_ = Sep::None;
    screen_.WriteAtCursor(std::wstring(static_cast<std::size_t>(count), L' '));
    at_line_start_ = false;
    break;
  }
  case Op::Clear:
    // CLEAR inside a program: variables, arrays, stacks and files go, and
    // execution continues at the next instruction.
    ClearState();
    return true;
  case Op::ScreenMode: {
    const Value mode_value = Pop();
    const int mode = static_cast<int>(std::lround(mode_value.AsNumber()));
    // Every SCREEN mode the reference defines is accepted. Modes 4, 5, 6, 10 and 11 used to be
    // rejected for want of a canvas size; each now maps to one that already existed.
    if (mode < screen::Screen::kTextMode || mode > 13) {
      if (mode != screen::Screen::kWideMode)
        return Fail(kErrIllegalFunctionCall);
    }
    screen_.SetGraphicsMode(mode);
    return true;
  }
  case Op::PSet: {
    const Value colour_value = Pop();
    const Value y_value = Pop();
    const Value x_value = Pop();
    const int colour = static_cast<int>(std::lround(colour_value.AsNumber()));
    if (colour < 0 || colour > screen_.MaxColour())
      return Fail(kErrIllegalFunctionCall);
    // World coordinates stay real until the WINDOW mapping: rounding first would
    // snap PSET (0.5,0.5) to (1,1) and lose the whole point of a window.
    double world_x = x_value.AsNumber();
    double world_y = y_value.AsNumber();
    if ((in.a & 1) != 0) { // STEP: relative to the last referenced point
      world_x += last_x_;
      world_y += last_y_;
    }
    int px = 0;
    int py = 0;
    MapGraphicsPoint(world_x, world_y, &px, &py);
    screen_.Plot(px, py, colour);
    last_x_ = static_cast<int>(std::lround(world_x));
    last_y_ = static_cast<int>(std::lround(world_y));
    return true;
  }
  case Op::Circle: {
    const Value aspect_value = Pop();
    const Value end_value = Pop();
    const Value start_value = Pop();
    const Value colour_value = Pop();
    const Value radius_value = Pop();
    const Value y_value = Pop();
    const Value x_value = Pop();
    const int colour = static_cast<int>(std::lround(colour_value.AsNumber()));
    if (colour < 0 || colour > screen_.MaxColour())
      return Fail(kErrIllegalFunctionCall);
    int radius = static_cast<int>(std::lround(radius_value.AsNumber()));
    if (radius < 0)
      return Fail(kErrIllegalFunctionCall);
    // aspect = y radius / x radius. A zero aspect means "the mode's default", which
    // is (4/3) * height / width: that reproduces the documented 5/6 for SCREEN 1 and
    // 5/12 for SCREEN 2, and it is what makes a visual circle, because a 4:3 display
    // of 320x200 or 640x200 has pixels taller than they are wide.
    double aspect = aspect_value.AsNumber();
    const screen::Canvas& canvas = screen_.Graphics();
    if (aspect == 0.0) {
      aspect = canvas.Width() > 0 ? (4.0 / 3.0) * static_cast<double>(canvas.Height()) /
                                        static_cast<double>(canvas.Width())
                                  : 1.0;
    }
    if (aspect <= 0.0)
      return Fail(kErrIllegalFunctionCall);
    // Below 1 the radius is measured in x pixels, above 1 in y pixels.
    const int radius_x = aspect < 1.0 ? radius : static_cast<int>(std::lround(radius / aspect));
    const int radius_y = aspect < 1.0 ? static_cast<int>(std::lround(radius * aspect)) : radius;
    int cx = static_cast<int>(std::lround(x_value.AsNumber()));
    int cy = static_cast<int>(std::lround(y_value.AsNumber()));
    if ((in.a & 1) != 0) { // STEP
      cx += last_x_;
      cy += last_y_;
    }
    // Under a WINDOW the centre maps through it and the radius scales with the x
    // axis; the aspect still applies, because it compensates for the display shape.
    int sx = 0;
    int sy = 0;
    MapGraphicsPoint(static_cast<double>(cx), static_cast<double>(cy), &sx, &sy);
    int scaled_x = radius_x;
    int scaled_y = radius_y;
    if (window_active_) {
      const screen::Canvas& window_canvas = screen_.Graphics();
      const double span = window_x2_ - window_x1_;
      if (span != 0.0) {
        const double scale = static_cast<double>(window_canvas.Width() - 1) / span;
        const int base = static_cast<int>(std::lround(radius * scale));
        scaled_x = aspect < 1.0 ? base : static_cast<int>(std::lround(base / aspect));
        scaled_y = aspect < 1.0 ? static_cast<int>(std::lround(base * aspect)) : base;
      }
    }
    screen_.DrawGraphicsEllipse(sx, sy, scaled_x, scaled_y, start_value.AsNumber(),
                                end_value.AsNumber(), colour);
    // After a CIRCLE, the last referenced point is the centre.
    last_x_ = cx;
    last_y_ = cy;
    return true;
  }
  case Op::Pcopy: {
    // PCOPY source, destination: page 0 is the displayed canvas, 1-7 are snapshots.
    const Value destination_value = Pop();
    const Value source_value = Pop();
    const int destination = static_cast<int>(std::lround(destination_value.AsNumber()));
    const int source = static_cast<int>(std::lround(source_value.AsNumber()));
    if (!screen_.CopyGraphicsPage(source, destination)) {
      return Fail(kErrIllegalFunctionCall,
                  "PCOPY needs pages 0 to 7, and a source page that has been filled");
    }
    return true;
  }
  case Op::GetImage: {
    // GET (x1,y1)-(x2,y2), array: capture a rectangle into an integer array.
    // Layout (a documented divergence from the packed GW-BASIC image): element 0 is
    // the width, element 1 the height, then one palette index per pixel, row major.
    const Value y2_value = Pop();
    const Value x2_value = Pop();
    const Value y1_value = Pop();
    const Value x1_value = Pop();
    int ax = 0;
    int ay = 0;
    int bx = 0;
    int by = 0;
    MapGraphicsPoint(x1_value.AsNumber(), y1_value.AsNumber(), &ax, &ay);
    MapGraphicsPoint(x2_value.AsNumber(), y2_value.AsNumber(), &bx, &by);
    const screen::Canvas& canvas = screen_.Graphics();
    const int left = std::max(0, std::min(ax, bx));
    const int right = std::min(canvas.Width() - 1, std::max(ax, bx));
    const int top = std::max(0, std::min(ay, by));
    const int bottom = std::min(canvas.Height() - 1, std::max(ay, by));
    if (right < left || bottom < top)
      return Fail(kErrIllegalFunctionCall);
    const int width = right - left + 1;
    const int height = bottom - top + 1;
    ArrayValue& array = arrays_[CanonicalVarName(in.name)];
    array.kind = Value::Kind::Int;
    array.base = 0;
    array.dims = {width * height + 1};
    array.data.assign(static_cast<std::size_t>(width * height + 2), Value::Int(0));
    array.data[0] = Value::Int(static_cast<std::int16_t>(width));
    array.data[1] = Value::Int(static_cast<std::int16_t>(height));
    for (int row = 0; row < height; ++row) {
      for (int column = 0; column < width; ++column) {
        array.data[static_cast<std::size_t>(2 + row * width + column)] =
            Value::Int(canvas.Pixel(left + column, top + row));
      }
    }
    return true;
  }
  case Op::PutImage: {
    // PUT (x,y), array [, action]: actions are PSET (0), PRESET (1), AND (2), OR (3)
    // and XOR (4), the last three combining with what is already on screen.
    const Value y_value = Pop();
    const Value x_value = Pop();
    int px = 0;
    int py = 0;
    MapGraphicsPoint(x_value.AsNumber(), y_value.AsNumber(), &px, &py);
    const auto found = arrays_.find(CanonicalVarName(in.name));
    if (found == arrays_.end())
      return Fail(kErrIllegalFunctionCall, "PUT needs an array filled by GET");
    const ArrayValue& array = found->second;
    if (array.data.size() < 2)
      return Fail(kErrIllegalFunctionCall, "PUT needs an array filled by GET");
    const int width = static_cast<int>(std::lround(array.data[0].AsNumber()));
    const int height = static_cast<int>(std::lround(array.data[1].AsNumber()));
    if (width <= 0 || height <= 0 ||
        static_cast<std::size_t>(2 + width * height) > array.data.size()) {
      return Fail(kErrIllegalFunctionCall, "PUT array does not hold an image");
    }
    const screen::Canvas& canvas = screen_.Graphics();
    for (int row = 0; row < height; ++row) {
      for (int column = 0; column < width; ++column) {
        const int source =
            static_cast<int>(std::lround(
                array.data[static_cast<std::size_t>(2 + row * width + column)].AsNumber())) &
            0x0F;
        const int dest_x = px + column;
        const int dest_y = py + row;
        const int existing =
            (dest_x >= 0 && dest_y >= 0 && dest_x < canvas.Width() && dest_y < canvas.Height())
                ? canvas.Pixel(dest_x, dest_y)
                : 0;
        int result = source;
        switch (in.a) {
        case 1:
          result = 0;
          break; // PRESET
        case 2:
          result = existing & source;
          break; // AND
        case 3:
          result = existing | source;
          break; // OR
        case 4:
          result = existing ^ source;
          break; // XOR
        default:
          break; // PSET
        }
        screen_.Plot(dest_x, dest_y, result & 0x0F);
      }
    }
    return true;
  }
  case Op::View: {
    // VIEW [(x1,y1)-(x2,y2)[,[fill][,[border]]]]; no arguments restores the full
    // canvas. Drawing is clipped to the viewport and coordinates are relative to it.
    if (in.a == 0) {
      screen_.ResetGraphicsViewport();
      return true;
    }
    const Value border_value = Pop();
    const Value fill_value = Pop();
    const Value y2_value = Pop();
    const Value x2_value = Pop();
    const Value y1_value = Pop();
    const Value x1_value = Pop();
    int x1 = static_cast<int>(std::lround(x1_value.AsNumber()));
    int y1 = static_cast<int>(std::lround(y1_value.AsNumber()));
    int x2 = static_cast<int>(std::lround(x2_value.AsNumber()));
    int y2 = static_cast<int>(std::lround(y2_value.AsNumber()));
    if (x1 == x2 || y1 == y2)
      return Fail(kErrIllegalFunctionCall);
    int fill = static_cast<int>(std::lround(fill_value.AsNumber()));
    int border = static_cast<int>(std::lround(border_value.AsNumber()));
    if (fill > screen_.MaxColour() || border > screen_.MaxColour())
      return Fail(kErrIllegalFunctionCall);
    if (fill < 0)
      fill = -1;
    if (border < 0)
      border = -1;
    screen_.SetGraphicsViewport(x1, y1, x2, y2, fill, border);
    return true;
  }
  case Op::Window: {
    // WINDOW [(x1,y1)-(x2,y2)], or WINDOW SCREEN (...). No arguments restores plain
    // screen coordinates.
    if (in.a == 0) {
      window_active_ = false;
      return true;
    }
    const Value y2_value = Pop();
    const Value x2_value = Pop();
    const Value y1_value = Pop();
    const Value x1_value = Pop();
    const double x1 = x1_value.AsNumber();
    const double y1 = y1_value.AsNumber();
    const double x2 = x2_value.AsNumber();
    const double y2 = y2_value.AsNumber();
    if (x1 == x2 || y1 == y2)
      return Fail(kErrIllegalFunctionCall);
    window_active_ = true;
    window_screen_ = in.a == 2;
    window_x1_ = x1;
    window_y1_ = y1;
    window_x2_ = x2;
    window_y2_ = y2;
    return true;
  }
  case Op::Palette: {
    // PALETTE attribute, colour; no arguments restores the default palette.
    if (in.a == 0) {
      screen_.ResetPalette();
      return true;
    }
    const Value colour_value = Pop();
    const Value attribute_value = Pop();
    const int colour = static_cast<int>(std::lround(colour_value.AsNumber()));
    const int attribute = static_cast<int>(std::lround(attribute_value.AsNumber()));
    // The wide mode takes 256 attributes and an 18-bit colour; the compatible modes keep
    // the EGA limits of 16 attributes and a 6-bit colour.
    if (screen_.IsTrueColourMode()) {
      if (attribute < 0 || attribute > 255 || colour < 0 || colour > 0x3FFFFF) {
        return Fail(kErrIllegalFunctionCall);
      }
    } else if (attribute < 0 || attribute > 15 || colour < 0 || colour > 63) {
      return Fail(kErrIllegalFunctionCall);
    }
    screen_.SetPaletteEntry(attribute, colour);
    return true;
  }
  case Op::Draw: {
    // DRAW macro$: the graphics macro language. `X variable;` is expanded here,
    // because the interpreter deliberately knows nothing about variables.
    const Value macro_value = Pop();
    if (!macro_value.IsString())
      return Fail(kErrTypeMismatch);
    // The current position defaults to the centre of the canvas, and is reset when
    // the mode changes size.
    const screen::Canvas& canvas = screen_.Graphics();
    if (draw_state_.x < 0 || draw_width_ != canvas.Width() || draw_height_ != canvas.Height()) {
      draw_state_ = DrawState{};
      draw_state_.x = canvas.Width() / 2;
      draw_state_.y = canvas.Height() / 2;
      draw_width_ = canvas.Width();
      draw_height_ = canvas.Height();
    }
    std::string macro = macro_value.AsString();
    for (int depth = 0; depth < 8; ++depth) {
      const std::size_t marker = macro.find_first_of("Xx");
      if (marker == std::string::npos)
        break;
      const std::size_t end = macro.find(';', marker);
      if (end == std::string::npos)
        return Fail(kErrIllegalFunctionCall);
      const std::string name = macro.substr(marker + 1, end - marker - 1);
      const auto value = vars_.find(name);
      const std::string replacement = (value != vars_.end() && value->second.IsString())
                                          ? value->second.AsString()
                                          : std::string();
      macro = macro.substr(0, marker) + replacement + macro.substr(end + 1);
    }
    std::string error;
    screen::Canvas& writable = screen_.Graphics();
    if (!ExecuteDrawMacro(&writable, macro, &draw_state_, &error)) {
      return Fail(kErrIllegalFunctionCall, error);
    }
    screen_.InvalidateGraphics();
    return true;
  }
  case Op::Paint: {
    const Value border_value = Pop();
    const Value fill_value = Pop();
    const Value y_value = Pop();
    const Value x_value = Pop();
    const int fill = static_cast<int>(std::lround(fill_value.AsNumber()));
    const int border = static_cast<int>(std::lround(border_value.AsNumber()));
    if (fill < 0 || fill > screen_.MaxColour() || border > screen_.MaxColour())
      return Fail(kErrIllegalFunctionCall);
    double world_x = x_value.AsNumber();
    double world_y = y_value.AsNumber();
    if ((in.a & 1) != 0) { // STEP
      world_x += last_x_;
      world_y += last_y_;
    }
    int px = 0;
    int py = 0;
    MapGraphicsPoint(world_x, world_y, &px, &py);
    screen_.FillGraphics(px, py, fill, border);
    last_x_ = static_cast<int>(std::lround(world_x));
    last_y_ = static_cast<int>(std::lround(world_y));
    return true;
  }
  case Op::Line: {
    const Value colour_value = Pop();
    const Value y2_value = Pop();
    const Value x2_value = Pop();
    const Value y1_value = Pop();
    const Value x1_value = Pop();
    const int colour = static_cast<int>(std::lround(colour_value.AsNumber()));
    if (colour < 0 || colour > screen_.MaxColour())
      return Fail(kErrIllegalFunctionCall);
    // Flags: 1 = a first coordinate was given, 2 = it was relative, 4 = the second
    // was relative, 8 = box, 16 = filled box, 32 = a style mask was given.
    const bool has_first = (in.a & 1) != 0;
    double world_x1 = x1_value.AsNumber();
    double world_y1 = y1_value.AsNumber();
    if (has_first) {
      if ((in.a & 2) != 0) {
        world_x1 += last_x_;
        world_y1 += last_y_;
      }
    } else {
      world_x1 = last_x_;
      world_y1 = last_y_;
    }
    double world_x2 = x2_value.AsNumber();
    double world_y2 = y2_value.AsNumber();
    if ((in.a & 4) != 0) {
      // Relative to the first coordinate, per the reference.
      world_x2 += world_x1;
      world_y2 += world_y1;
    }
    // The last referenced point is the second coordinate, in world units.
    const int x2 = static_cast<int>(std::lround(world_x2));
    const int y2 = static_cast<int>(std::lround(world_y2));
    // WINDOW, when set, maps the world coordinates onto canvas pixels.
    int sx1 = 0;
    int sy1 = 0;
    int sx2 = 0;
    int sy2 = 0;
    MapGraphicsPoint(world_x1, world_y1, &sx1, &sy1);
    MapGraphicsPoint(world_x2, world_y2, &sx2, &sy2);
    if ((in.a & 16) != 0) {
      screen_.FillGraphicsBox(sx1, sy1, sx2, sy2, colour);
    } else if ((in.a & 8) != 0) {
      screen_.DrawGraphicsBox(sx1, sy1, sx2, sy2, colour);
    } else {
      const int style = (in.a & 32) != 0 ? in.b : 0xFFFF;
      screen_.DrawGraphicsStyledLine(sx1, sy1, sx2, sy2, colour, style);
    }
    // After a LINE, the last referenced point is the second coordinate.
    last_x_ = x2;
    last_y_ = y2;
    return true;
  }
  case Op::Cls: {
    // CLS [n]: 0 clears the screen, 1 the graphics viewport (none exists here, so
    // nothing is cleared), 2 only the text window.
    const Value mode_value = Pop();
    const int mode = static_cast<int>(std::lround(mode_value.AsNumber()));
    if (mode == 2) {
      screen_.ClearTextWindow();
    } else if (mode == 1) {
      screen_.ClearGraphics(); // CLS 1: the graphics viewport
    } else {
      // CLS 0 (and bare CLS) clears the text screen; in a graphics mode the canvas
      // goes too, which is what the reference describes for "text and graphics".
      screen_.Clear();
      if (screen_.GraphicsMode() != screen::Screen::kTextMode)
        screen_.ClearGraphics();
    }
    at_line_start_ = true;
    pending_sep_ = Sep::None;
    break;
  }
  case Op::ViewPrint: {
    const Value bottom_value = Pop();
    const Value top_value = Pop();
    const int bottom = static_cast<int>(std::lround(bottom_value.AsNumber()));
    const int top = static_cast<int>(std::lround(top_value.AsNumber()));
    if (top <= 0 && bottom <= 0) {
      screen_.SetTextWindow(0, screen_.Height() - 1); // whole screen
    } else {
      if (top < 1 || bottom < top || bottom > screen_.Height())
        return Fail(kErrIllegalFunctionCall);
      screen_.SetTextWindow(top - 1, bottom - 1);
    }
    break;
  }
  case Op::Width: {
    const Value width_value = Pop();
    const int width = static_cast<int>(std::lround(width_value.AsNumber()));
    if (width < 1 || width > screen::Screen::kCols)
      return Fail(kErrIllegalFunctionCall);
    screen_.SetGridSize(width, screen_.Height());
    break;
  }
  case Op::KeySet: {
    // KEY n, "string": n is 1-10 or 15-20; the value is truncated to 15
    // characters, and an empty value disables the key.
    const Value value = Pop();
    const Value number = Pop();
    const int n = static_cast<int>(std::lround(number.AsNumber()));
    if (!((n >= 1 && n <= 10) || (n >= 15 && n <= 20)))
      return Fail(kErrIllegalFunctionCall);
    if (!value.IsString())
      return Fail(kErrTypeMismatch);
    std::string text = value.AsString();
    if (text.size() > 15)
      text.resize(15);
    key_values_[static_cast<std::size_t>(n - 1)] = text;
    if (key_bar_)
      DrawKeyBar();
    return true;
  }
  case Op::KeyOn:
    // The bar occupies the last screen line, which then belongs to the display
    // rather than to the program.
    key_bar_ = true;
    screen_.SetTextWindow(0, screen_.Height() - 2);
    DrawKeyBar();
    return true;
  case Op::KeyOff: {
    key_bar_ = false;
    for (int col = 0; col < screen_.Width(); ++col)
      screen_.PutCharAt(screen_.Height() - 1, col, L' ');
    screen_.SetTextWindow(0, screen_.Height() - 1); // the line is available again
    return true;
  }
  case Op::KeyList: {
    for (std::size_t i = 0; i < 10; ++i) {
      const std::string line = "F" + std::to_string(i + 1) + " " + key_values_[i];
      screen_.WriteAtCursor(screen::DecodeCp437(line));
      screen_.NewLine();
    }
    at_line_start_ = true;
    pending_sep_ = Sep::None;
    return true;
  }
  case Op::OnKey: {
    // ON KEY(n) GOSUB line: remember the handler (a) for key n (b).
    if (in.b < 1 || in.b > 10)
      return Fail(kErrIllegalFunctionCall);
    on_key_pc_[static_cast<std::size_t>(in.b - 1)] = in.a;
    return true;
  }
  case Op::KeyControl: {
    // KEY(n) ON/OFF/STOP
    if (in.a < 1 || in.a > 10)
      return Fail(kErrIllegalFunctionCall);
    const std::size_t index = static_cast<std::size_t>(in.a - 1);
    if (in.b == 1) {
      if (on_key_pc_[index] < 0)
        return Fail(kErrIllegalFunctionCall); // no handler yet
      key_state_[index] = 1;
    } else if (in.b == 2) {
      key_state_[index] = 2; // STOP: stay armed but do not fire
    } else {
      key_state_[index] = 0;
    }
    return true;
  }
  case Op::SaveProgram: {
    if (store_ == nullptr)
      return Fail(kErrAdvancedFeature, "SAVE needs the program store");
    const Value path_value = Pop();
    std::string path;
    if (!String(path_value, &path))
      return false;
    int error_code = 0;
    std::string error;
    if (!SaveProgramToFile(path, *store_, &error_code, &error))
      return Fail(error_code, error);
    return true;
  }
  case Op::LoadProgram:
  case Op::Chain: {
    if (store_ == nullptr)
      return Fail(kErrAdvancedFeature, "this needs the program store");
    const bool chain = in.op == Op::Chain;
    int start_line = 0;
    if (chain && in.a == 1) {
      const Value line_value = Pop();
      double line_number = 0.0;
      if (!Number(line_value, &line_number))
        return false;
      start_line = static_cast<int>(RoundToInt(line_number));
    }
    const Value path_value = Pop();
    std::string path;
    if (!String(path_value, &path))
      return false;
    int error_code = 0;
    std::string error;
    if (!LoadProgramFromFile(path, store_, &error_code, &error))
      return Fail(error_code, error);
    Program loaded;
    CompileError compile_error;
    if (!Compile(*store_, &loaded, &compile_error)) {
      return Fail(compile_error.code, "the loaded program does not compile");
    }
    // CHAIN keeps only the COMMON variables; LOAD starts clean.
    if (!chain) {
      vars_.clear();
    } else {
      std::map<std::string, Value> kept;
      for (const std::string& name : common_vars_) {
        const auto found = vars_.find(name);
        if (found != vars_.end())
          kept[name] = found->second;
      }
      vars_ = std::move(kept);
    }
    program_ = std::move(loaded);
    stack_.clear();
    for_stack_.clear();
    gosub_stack_.clear();
    func_stack_.clear();
    data_index_ = 0;
    ResetPrintState();
    using_active_ = false;
    halted_ = false;
    pc_ = 0;
    if (start_line > 0) {
      bool found = false;
      for (std::size_t i = 0; i < program_.instrs_.size(); ++i) {
        if (LineAtPc(static_cast<int>(i)) == start_line) {
          pc_ = static_cast<int>(i);
          found = true;
          break;
        }
      }
      if (!found)
        return Fail(kErrUndefinedLineNumber);
    }
    return true;
  }
  case Op::Common: {
    // COMMON var[, var]: recorded as a name list; CHAIN keeps those variables.
    std::size_t pos = 0;
    while (pos <= in.name.size()) {
      const std::size_t comma = in.name.find(',', pos);
      const std::size_t end = comma == std::string::npos ? in.name.size() : comma;
      if (end > pos)
        common_vars_.insert(in.name.substr(pos, end - pos));
      if (comma == std::string::npos)
        break;
      pos = comma + 1;
    }
    return true;
  }
  case Op::LList: {
    // LLIST: the same listing, sent to the printer instead of the screen. a and b bound the
    // line numbers when the statement carried a range; both zero means the whole program.
    if (store_ == nullptr)
      return Fail(kErrAdvancedFeature, "LLIST needs the program store");
    std::string listing;
    for (const SourceLine& line : store_->Lines()) {
      if (in.a != 0 && line.line_number < in.a)
        continue;
      if (in.b != 0 && line.line_number > in.b)
        continue;
      listing += std::to_string(line.line_number) + " " + line.source + "\n";
    }
    host_->WriteToPrinter(listing);
    return true;
  }
  case Op::LPrint:
    // Start (or continue) a printer-bound print list. An open line is NOT cleared: a
    // trailing ';' on the previous LPRINT leaves the line open, and the next LPRINT
    // continues it, exactly as PRINT does.
    printer_mode_ = true;
    return true;
  case Op::List: {
    // LIST prints the stored lines; a and b are the range (0 = no bound).
    if (store_ == nullptr)
      return Fail(kErrAdvancedFeature, "LIST needs the program store");
    for (const SourceLine& line : store_->Lines()) {
      if (in.a != 0 && line.line_number < in.a)
        continue;
      if (in.b != 0 && line.line_number > in.b)
        continue;
      screen_.WriteAtCursor(
          screen::DecodeCp437(std::to_string(line.line_number) + " " + line.source));
      screen_.NewLine();
    }
    at_line_start_ = true;
    pending_sep_ = Sep::None;
    return true;
  }
  case Op::Delete: {
    if (store_ == nullptr)
      return Fail(kErrAdvancedFeature, "DELETE needs the program store");
    // Collect first: removing while iterating would invalidate the range.
    std::vector<int> doomed;
    for (const SourceLine& line : store_->Lines()) {
      if (line.line_number >= in.a && line.line_number <= in.b)
        doomed.push_back(line.line_number);
    }
    for (const int number : doomed)
      store_->Remove(number);
    return true;
  }
  case Op::New:
    if (store_ != nullptr)
      store_->Clear();
    // Clearing the running program ends execution at the next step.
    program_ = Program{};
    pc_ = 0;
    return true;
  case Op::Run: {
    // RUN [line]: restart, clearing variables and stacks (Vm::Reset does both).
    int start_line = 0;
    if (in.a == 1) {
      const Value line_value = Pop();
      double line_number = 0.0;
      if (!Number(line_value, &line_number))
        return false;
      start_line = static_cast<int>(RoundToInt(line_number));
    }
    Reset();
    if (start_line > 0) {
      // Start at the first instruction of that line, as RUN <line> does.
      bool found = false;
      for (std::size_t i = 0; i < program_.instrs_.size(); ++i) {
        if (LineAtPc(static_cast<int>(i)) == start_line) {
          pc_ = static_cast<int>(i);
          found = true;
          break;
        }
      }
      if (!found)
        return Fail(kErrUndefinedLineNumber);
    }
    traced_line_ = -1;
    return true;
  }
  case Op::Tron:
    trace_ = true;
    traced_line_ = -1;
    return true;
  case Op::Troff:
    trace_ = false;
    return true;
  case Op::Fail:
    return Fail(static_cast<int>(in.a), in.name);
  case Op::Field: {
    // FIELD #n, width AS var$...: allocate record space for string variables.
    const Value number = Pop();
    double number_value = 0.0;
    if (!Number(number, &number_value))
      return false;
    const int handle = static_cast<int>(RoundToInt(number_value));
    const auto file = random_files_.find(handle);
    if (file == random_files_.end())
      return Fail(kErrBadFileNumber);
    std::size_t pos = 0;
    while (pos < in.name.size()) {
      const std::size_t colon = in.name.find(':', pos);
      if (colon == std::string::npos)
        return Fail(kErrInternal);
      const std::size_t comma = in.name.find(',', colon);
      const std::size_t end = comma == std::string::npos ? in.name.size() : comma;
      const std::string name = in.name.substr(pos, colon - pos);
      const int width = std::atoi(in.name.substr(colon + 1, end - colon - 1).c_str());
      if (width < 0)
        return Fail(kErrIllegalFunctionCall);
      if (file->second.allocated + width > file->second.record_length) {
        return Fail(kErrFieldOverflow); // "Field overflow"
      }
      file->second.fields.push_back({name, file->second.allocated, width});
      file->second.allocated += width;
      // A fielded variable starts as spaces of its field width.
      vars_[name] = Value::Str(std::string(static_cast<std::size_t>(width), ' '));
      pos = comma == std::string::npos ? in.name.size() : comma + 1;
    }
    return true;
  }
  case Op::LSet: {
    const Value value = Pop();
    if (!value.IsString())
      return Fail(kErrTypeMismatch);
    std::string text = value.AsString();
    const bool right_justify = in.a == 1;
    // A fielded variable writes into its slice of the record buffer.
    FieldBinding* binding = nullptr;
    RandomFile* owner = nullptr;
    for (auto& entry : random_files_) {
      for (auto& field : entry.second.fields) {
        if (field.name == in.name) {
          binding = &field;
          owner = &entry.second;
        }
      }
    }
    if (binding != nullptr && owner != nullptr) {
      const std::size_t width = static_cast<std::size_t>(binding->width);
      if (text.size() > width)
        text.resize(width); // drop characters from the right
      std::string padded(width, ' ');
      padded.replace(right_justify ? width - text.size() : 0, text.size(), text);
      owner->buffer.replace(static_cast<std::size_t>(binding->offset), width, padded);
      vars_[in.name] = Value::Str(padded);
      return true;
    }
    // Otherwise justify inside the variable's own current length.
    std::size_t width = text.size();
    const auto existing = vars_.find(in.name);
    if (existing != vars_.end() && existing->second.IsString()) {
      width = std::max(width, existing->second.AsString().size());
    }
    if (text.size() > width)
      text.resize(width);
    std::string padded(width, ' ');
    padded.replace(right_justify ? width - text.size() : 0, text.size(), text);
    vars_[in.name] = Value::Str(padded);
    return true;
  }
  case Op::GetRecord:
  case Op::PutRecord: {
    const bool writing = in.op == Op::PutRecord;
    int record = 0;
    if (in.a == 1) {
      const Value record_value = Pop();
      double record_number = 0.0;
      if (!Number(record_value, &record_number))
        return false;
      record = static_cast<int>(RoundToInt(record_number));
    }
    const Value handle_value = Pop();
    double handle_number = 0.0;
    if (!Number(handle_value, &handle_number))
      return false;
    const int handle = static_cast<int>(RoundToInt(handle_number));
    const auto file = random_files_.find(handle);
    if (file == random_files_.end())
      return Fail(kErrBadFileNumber);
    RandomFile& state = file->second;
    if (record <= 0)
      record = state.last_record + 1; // sequential access
    if (record <= 0)
      return Fail(kErrIllegalFunctionCall);
    const auto stream = files_.find(handle);
    if (stream == files_.end() || !stream->second.is_open())
      return Fail(kErrBadFileNumber);
    const std::streamoff offset = static_cast<std::streamoff>(record - 1) * state.record_length;
    state.buffer.assign(static_cast<std::size_t>(state.record_length), '\0');
    stream->second.clear();
    stream->second.seekg(offset);
    stream->second.read(state.buffer.data(), state.record_length);
    stream->second.clear(); // a short read at EOF is not an error here
    if (!writing) {
      // GET publishes the fields into their variables.
      for (const FieldBinding& field : state.fields) {
        vars_[field.name] = Value::Str(state.buffer.substr(static_cast<std::size_t>(field.offset),
                                                           static_cast<std::size_t>(field.width)));
      }
    } else {
      // PUT copies the variables into the buffer, then writes the record.
      for (const FieldBinding& field : state.fields) {
        std::string text;
        const auto var = vars_.find(field.name);
        if (var != vars_.end() && var->second.IsString())
          text = var->second.AsString();
        const std::size_t width = static_cast<std::size_t>(field.width);
        if (text.size() > width)
          text.resize(width);
        text.resize(width, ' ');
        state.buffer.replace(static_cast<std::size_t>(field.offset), width, text);
      }
    }
    state.last_record = record;
    if (writing) {
      stream->second.seekp(0, std::ios::end);
      const std::streamoff end = stream->second.tellp();
      if (end < offset) { // fill any gap so the record lands at its own offset
        const std::string padding(static_cast<std::size_t>(offset - end), '\0');
        stream->second.write(padding.data(), static_cast<std::streamsize>(padding.size()));
      }
      stream->second.seekp(offset);
      stream->second.write(state.buffer.data(), state.record_length);
      stream->second.flush();
    }
    return true;
  }
  case Op::Locate: {
    // LOCATE row, col, cursor, start, stop. An argument of -1 is "unchanged"; row and
    // column are 1-based. The scan-line arguments are read and ignored: there is no CRT
    // beam to shape on a windowed or console surface.
    const Value stop_value = Pop();
    const Value start_value = Pop();
    const Value cursor_value = Pop();
    const Value column_value = Pop();
    const Value row_value = Pop();
    double row = 0.0;
    double column = 0.0;
    double cursor = 0.0;
    double scan_start = 0.0;
    double scan_stop = 0.0;
    if (!Number(row_value, &row) || !Number(column_value, &column) ||
        !Number(cursor_value, &cursor) || !Number(start_value, &scan_start) ||
        !Number(stop_value, &scan_stop))
      return false;
    static_cast<void>(scan_start);
    static_cast<void>(scan_stop);
    if (row >= 0.0 || column >= 0.0) {
      const int target_row =
          row < 0.0 ? screen_.CursorRow() : static_cast<int>(std::lround(row)) - 1;
      const int target_column =
          column < 0.0 ? screen_.CursorCol() : static_cast<int>(std::lround(column)) - 1;
      screen_.SetCursor(target_row, target_column);
    }
    if (cursor >= 0.0)
      screen_.SetCursorVisible(cursor != 0.0);
    at_line_start_ = false;
    pending_sep_ = Sep::None;
    break;
  }

  case Op::Color: {
    const Value background = Pop();
    const Value foreground = Pop();
    double bg = 0.0;
    double fg = 0.0;
    if (!Number(foreground, &fg) || !Number(background, &bg))
      return false;
    const int fg_value = static_cast<int>(std::lround(fg));
    const int bg_value = static_cast<int>(std::lround(bg));
    if (screen_.GraphicsMode() != screen::Screen::kTextMode) {
      // In a graphics mode COLOR means COLOR [background][,[palette]]: the first
      // argument is the background and the second selects one of the two colour sets.
      if (fg_value >= 0 && fg_value <= 15)
        screen_.SetGraphicsBackground(fg_value);
      if (bg_value >= 0)
        screen_.SetGraphicsColourSet(bg_value);
      break;
    }
    screen_.SetColor(fg_value < 0 ? screen_.ColorFg() : fg_value,
                     bg_value < 0 ? screen_.ColorBg() : bg_value);
    break;
  }
  case Op::Input:
    return DoInput(in, false);
  case Op::LineInput:
    return DoInput(in, true);
  case Op::ReadArray: {
    // The front end pushed the subscript expressions; the value comes from DATA, in order.
    std::vector<int> subscripts(static_cast<std::size_t>(in.a > 0 ? in.a : 0));
    for (std::size_t i = subscripts.size(); i-- > 0;) {
      const Value subscript = Pop();
      double number = 0.0;
      if (!Number(subscript, &number))
        return false;
      subscripts[i] = static_cast<int>(RoundToInt(number));
    }
    const bool is_string = VarTypeSuffix(in.name) == '$';
    if (data_index_ >= program_.data_items_.size())
      return Fail(kErrOutOfData);
    const DataItem& item = program_.data_items_[data_index_++];
    if (!is_string && !item.numeric)
      return Fail(kErrSyntax);
    ArrayValue* array = FindArray(in.name);
    if (array == nullptr) {
      array = EnsureArray(in.name, std::vector<int>(subscripts.size(), 10), in.b);
      if (array == nullptr)
        return Fail(kErrSubscriptOutOfRange);
    }
    std::size_t offset = 0;
    if (!ArrayOffset(*array, subscripts, &offset))
      return Fail(kErrSubscriptOutOfRange);
    Value converted;
    if (!ConvertForName(in.name, is_string ? Value::Str(item.text) : item.number, &converted))
      return false;
    array->data[offset] = converted;
    break;
  }
  case Op::Read:
    return DoRead(in);
  case Op::Restore: {
    if (in.a == 0) {
      data_index_ = 0;
      break;
    }
    const auto it = program_.line_data_index_.find(in.a);
    if (it == program_.line_data_index_.end())
      return Fail(kErrUndefinedLineNumber);
    data_index_ = static_cast<std::size_t>(it->second);
    break;
  }
  case Op::Beep:
    // Reference: the speaker at 800 Hz for one-quarter of a second.
    host_->PlayTone(800, 250);
    break;
  case Op::Sound: {
    const Value duration_value = Pop();
    const Value frequency_value = Pop();
    double duration = 0.0;
    double frequency = 0.0;
    if (!Number(duration_value, &duration) || !Number(frequency_value, &frequency))
      return false;
    // A duration of 0 stops any sound; playback is synchronous here, so there is
    // nothing to stop and it is a no-op. The "infinite" case (duration below
    // 0.022) becomes a short bounded tone - a documented divergence.
    const int milliseconds = TicksToMilliseconds(duration);
    if (milliseconds == 0)
      break;
    host_->PlayTone(ClampFrequency(static_cast<int>(std::lround(frequency))), milliseconds);
    break;
  }
  case Op::Play: {
    const Value music = Pop();
    std::string text;
    if (!String(music, &text))
      return false;
    std::vector<Tone> tones;
    std::string parse_error;
    if (!ParsePlayString(text, &tones, &parse_error))
      return Fail(kErrIllegalFunctionCall);
    for (const Tone& tone : tones) {
      host_->PlayTone(tone.frequency_hz, tone.duration_ms);
    }
    break;
  }
  case Op::Jump:
    if (in.a < 0)
      return Fail(kErrUndefinedLineNumber);
    pc_ = in.a;
    break;
  case Op::JumpIfFalse: {
    const Value v = Pop();
    double value = 0.0;
    if (!Number(v, &value))
      return false;
    if (value == 0.0) {
      if (in.a < 0)
        return Fail(kErrUndefinedLineNumber);
      pc_ = in.a;
    }
    break;
  }
  case Op::Gosub:
    if (in.a < 0)
      return Fail(kErrUndefinedLineNumber);
    gosub_stack_.push_back(pc_);
    pc_ = in.a;
    break;
  case Op::Return:
    if (gosub_stack_.empty())
      return Fail(kErrReturnWithoutGosub);
    in_event_ = false; // a timer handler finished
    pc_ = gosub_stack_.back();
    gosub_stack_.pop_back();
    break;
  case Op::ForStart:
    return DoForStart(in);
  case Op::ForNext:
    return DoForNext(in);
  case Op::OnJump: {
    const Value v = Pop();
    double value = 0.0;
    if (!Number(v, &value))
      return false;
    const long index = std::lround(value);
    if (index < 0 || index > 255)
      return Fail(kErrIllegalFunctionCall);
    const std::vector<int>& table = program_.jump_tables_[static_cast<std::size_t>(in.a)];
    if (index >= 1 && static_cast<std::size_t>(index) <= table.size()) {
      const int target = table[static_cast<std::size_t>(index - 1)];
      if (target < 0)
        return Fail(kErrUndefinedLineNumber);
      if (in.b == 1)
        gosub_stack_.push_back(pc_);
      pc_ = target;
    }
    break;
  }
  case Op::CallUser: {
    if (in.a < 0 || static_cast<std::size_t>(in.a) >= program_.user_funcs_.size()) {
      return Fail(kErrUndefinedUserFunction);
    }
    const UserFunc& fn = program_.user_funcs_[static_cast<std::size_t>(in.a)];
    std::vector<Value> args(static_cast<std::size_t>(in.b > 0 ? in.b : 0));
    for (std::size_t i = args.size(); i-- > 0;)
      args[i] = Pop();
    FuncFrame frame;
    frame.return_pc = pc_;
    for (const std::string& param : fn.params) {
      frame.saved.emplace_back(param, LoadVariable(param));
    }
    for (std::size_t i = 0; i < fn.params.size() && i < args.size(); ++i) {
      if (!StoreVariable(fn.params[i], args[i]))
        return false;
    }
    func_stack_.push_back(std::move(frame));
    pc_ = fn.body_pc;
    break;
  }
  case Op::UserReturn: {
    const Value result = Pop();
    if (func_stack_.empty())
      return Fail(kErrInternal);
    FuncFrame frame = std::move(func_stack_.back());
    func_stack_.pop_back();
    for (const auto& entry : frame.saved) {
      vars_[CanonicalVarName(entry.first)] = entry.second;
    }
    pc_ = frame.return_pc;
    Push(result);
    break;
  }
  case Op::OnError:
    error_handler_pc_ = in.a; // -1 = ON ERROR GOTO 0 (trapping disabled)
    handling_error_ = false;
    break;
  case Op::Resume: {
    if (!handling_error_ && error_pc_ < 0)
      return Fail(kErrResumeWithoutError);
    handling_error_ = false;
    switch (in.b) {
    case 0: // RESUME: retry the statement that failed
      pc_ = error_pc_;
      break;
    case 1: // RESUME NEXT
      pc_ = error_next_pc_;
      break;
    default: // RESUME <line>
      if (in.a < 0)
        return Fail(kErrUndefinedLineNumber);
      pc_ = in.a;
      break;
    }
    break;
  }
  case Op::ErrorStmt: {
    const Value v = Pop();
    double code = 0.0;
    if (!Number(v, &code))
      return false;
    return Fail(static_cast<int>(RoundToInt(code)));
  }
  case Op::Shell: {
    std::string command;
    if (in.b != 0) {
      const Value v = Pop();
      if (!String(v, &command))
        return false;
    }
    const int exit_code = host_->RunShell(command);
    if (exit_code < 0) {
      return Fail(kErrAdvancedFeature, "SHELL could not start the command interpreter");
    }
    break;
  }
  case Op::Open: {
    Value name_value;
    Value number_value;
    int mode = in.b;
    int record_length = 128; // GW-BASIC's default record length
    if (in.a == 1) {         // LEN= was given, pushed after the file number
      const Value length_value = Pop();
      double length = 0.0;
      if (!Number(length_value, &length))
        return false;
      record_length = static_cast<int>(RoundToInt(length));
      if (record_length < 1)
        return Fail(kErrIllegalFunctionCall);
    }
    if (in.b == 3) { // short form: stack is [mode, number, name]
      name_value = Pop();
      number_value = Pop();
      const Value mode_value = Pop();
      std::string text;
      if (!String(mode_value, &text))
        return false;
      const char c =
          text.empty() ? 'I' : static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
      mode = c == 'O' ? 1 : (c == 'A' ? 2 : 0);
    } else { // long form: stack is [name, number]
      number_value = Pop();
      name_value = Pop();
    }
    std::string name;
    if (!String(name_value, &name))
      return false;
    double number = 0.0;
    if (!Number(number_value, &number))
      return false;
    const int handle = static_cast<int>(RoundToInt(number));
    if (handle < 1 || handle > 255)
      return Fail(kErrBadFileNumber);
    if (files_.count(handle) != 0)
      return Fail(kErrFileAlreadyOpen);
    std::ios::openmode flags = std::ios::binary;
    switch (mode) {
    case 1:
      flags |= std::ios::out | std::ios::trunc;
      break;
    case 2:
      flags |= std::ios::out | std::ios::app;
      break;
    case 4: // RANDOM: read and write, created when missing
      flags |= std::ios::in | std::ios::out;
      break;
    default:
      flags |= std::ios::in;
      break;
    }
    if (mode == 4) {
      std::ifstream probe(name);
      if (!probe.is_open()) {
        std::ofstream create(name); // random files are created by OPEN
      } else {
        probe.close();
      }
    }
    std::fstream stream(name, flags);
    if (!stream.is_open()) {
      return Fail(mode == 0 ? kErrFileNotFound : kErrPathFileAccess);
    }
    files_.emplace(handle, std::move(stream));
    if (mode == 4) {
      RandomFile state;
      state.record_length = record_length;
      state.buffer.assign(static_cast<std::size_t>(record_length), '\0');
      random_files_[handle] = std::move(state);
    }
    break;
  }
  case Op::Close: {
    if (in.b == 0) {
      for (auto& entry : files_)
        entry.second.close();
      files_.clear();
      random_files_.clear();
      break;
    }
    const Value v = Pop();
    double number = 0.0;
    if (!Number(v, &number))
      return false;
    const int handle = static_cast<int>(RoundToInt(number));
    const auto target = files_.find(handle);
    if (target == files_.end())
      return Fail(kErrBadFileNumber);
    target->second.close();
    files_.erase(target);
    random_files_.erase(handle);
    break;
  }
  case Op::FilePrint: {
    const Value v = Pop();
    double number = 0.0;
    if (!Number(v, &number))
      return false;
    const int handle = static_cast<int>(RoundToInt(number));
    const auto target = files_.find(handle);
    if (target == files_.end() || !target->second.is_open())
      return Fail(kErrBadFileNumber);
    print_file_ = handle;
    write_mode_ = in.a != 0;
    print_buffer_.clear();
    break;
  }
  case Op::FileInput: {
    const Value v = Pop();
    double number = 0.0;
    if (!Number(v, &number))
      return false;
    const int handle = static_cast<int>(RoundToInt(number));
    const auto target = files_.find(handle);
    if (target == files_.end() || !target->second.is_open())
      return Fail(kErrBadFileNumber);
    std::string line;
    if (!std::getline(target->second, line))
      return Fail(kErrInputPastEnd);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const std::vector<std::string> names = SplitCommas(in.name);
    if (in.b == 1) { // LINE INPUT #: the whole record is one string
      if (!names.empty() && !StoreVariable(names[0], Value::Str(line)))
        return false;
      break;
    }
    const std::vector<std::string> fields = ParseInputFields(line);
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (i >= fields.size())
        break; // a short record leaves the rest unchanged
      if (VarTypeSuffix(names[i]) == '$') {
        if (!StoreVariable(names[i], Value::Str(fields[i])))
          return false;
        continue;
      }
      if (fields[i].empty())
        continue;
      double field = 0.0;
      if (!ParseFieldNumber(fields[i], &field))
        return Fail(kErrSyntax);
      if (!StoreVariable(names[i], Value::Sgl(field)))
        return false;
    }
    break;
  }
  case Op::OnTimer: {
    const Value v = Pop();
    double interval = 0.0;
    if (!Number(v, &interval))
      return false;
    timer_interval_ = interval;
    timer_handler_pc_ = in.a;
    break;
  }
  case Op::TimerControl:
    if (in.a == 1) {
      timer_enabled_ = true;
      timer_stopped_ = false;
      timer_last_ = host_->SecondsSinceMidnight();
    } else if (in.a == 0) {
      timer_enabled_ = false;
    } else {
      timer_stopped_ = true;
    }
    break;
  case Op::Randomize: {
    double seed_value = 0.0;
    if (in.b == 0) {
      const Value v = Pop();
      if (!Number(v, &seed_value))
        return false;
    } else {
      // Without a seed expression GW-BASIC prompts interactively; lwbasic
      // derives the seed from the host clock instead (documented divergence).
      seed_value = host_->SecondsSinceMidnight() * 1000.0;
    }
    rng_.seed(static_cast<std::uint32_t>(std::llround(seed_value)));
    break;
  }
  case Op::Swap: {
    const std::vector<std::string> names = SplitCommas(in.name);
    if (names.size() != 2)
      return Fail(kErrSyntax);
    // Subscript expressions for both sides are on the stack, the second side's last, so they come
    // off in reverse statement order. A count of zero means a plain scalar.
    auto pop_subscripts = [this](int count, std::vector<int>* out) -> bool {
      out->assign(static_cast<std::size_t>(count > 0 ? count : 0), 0);
      for (std::size_t i = out->size(); i-- > 0;) {
        const Value subscript = Pop();
        double number = 0.0;
        if (!Number(subscript, &number))
          return false;
        (*out)[i] = static_cast<int>(RoundToInt(number));
      }
      return true;
    };
    std::vector<int> second_subscripts;
    std::vector<int> first_subscripts;
    if (!pop_subscripts(in.b, &second_subscripts) || !pop_subscripts(in.a, &first_subscripts))
      return false;
    // Resolve both array slots before either value moves, so a bad subscript cannot leave the
    // pair half swapped.
    ArrayValue* arrays[2] = {nullptr, nullptr};
    std::size_t offsets[2] = {0, 0};
    const std::vector<int>* lists[2] = {&first_subscripts, &second_subscripts};
    for (int side = 0; side < 2; ++side) {
      if (lists[side]->empty())
        continue;
      ArrayValue* array = FindArray(names[side]);
      if (array == nullptr) {
        array = EnsureArray(names[side], std::vector<int>(lists[side]->size(), 10), 0);
        if (array == nullptr)
          return Fail(kErrSubscriptOutOfRange);
      }
      if (!ArrayOffset(*array, *lists[side], &offsets[side]))
        return Fail(kErrSubscriptOutOfRange);
      arrays[side] = array;
    }
    const Value first_value =
        arrays[0] != nullptr ? arrays[0]->data[offsets[0]] : LoadVariable(names[0]);
    const Value second_value =
        arrays[1] != nullptr ? arrays[1]->data[offsets[1]] : LoadVariable(names[1]);
    Value converted;
    if (arrays[0] != nullptr) {
      if (!ConvertForName(names[0], second_value, &converted))
        return false;
      arrays[0]->data[offsets[0]] = converted;
    } else if (!StoreVariable(names[0], second_value)) {
      return false;
    }
    if (arrays[1] != nullptr) {
      if (!ConvertForName(names[1], first_value, &converted))
        return false;
      arrays[1]->data[offsets[1]] = converted;
    } else if (!StoreVariable(names[1], first_value)) {
      return false;
    }
    break;
  }
  case Op::Unsupported:
    return Fail(kErrAdvancedFeature, in.name + " is not implemented in lwbasic yet");
  case Op::Stop:
    last_.status = RunResult::Status::Break;
    last_.line = LineAtPc(pc_ - 1);
    return false;
  case Op::End:
    last_.status = RunResult::Status::Done;
    last_.line = LineAtPc(pc_ - 1);
    return false;
  case Op::Nop:
    break;
  }
  return true;
}

RunResult Vm::Run() {
  halted_ = false; // a fresh run re-arms execution after a previous halt
  for (;;) {
    // TRON: echo the line number when execution enters a new line, in the [line]
    // form GW-BASIC uses.
    if (trace_) {
      const int line = LineAtPc(pc_);
      if (line > 0 && line != traced_line_) {
        traced_line_ = line;
        screen_.WriteAtCursor(screen::DecodeCp437("[" + std::to_string(line) + "]"));
        screen_.NewLine();
        at_line_start_ = true;
        pending_sep_ = Sep::None;
        screen_.PresentIfDirty();
      }
    }
    const int pc_before = pc_;
    const bool event_instruction = in_event_;
    if (!Step()) {
      if (trap_pending_) {
        // An error was trapped: continue at the ON ERROR handler.
        trap_pending_ = false;
        if (pc_ == pc_before)
          break; // no progress: stop instead of spinning
        screen_.PresentIfDirty();
        continue;
      }
      break;
    }
    // Progress is recorded here, where an instruction really executed, never in
    // the poll below: setting the flag in the poll made it true immediately
    // after a handler's RETURN, so an event could fire again before the
    // interrupted statement ran and the program never advanced.
    if (!event_instruction)
      main_step_done_ = true;
    screen_.PresentIfDirty();
    // Timer events are polled between instructions, and only after a real main
    // instruction has run since the last event, so firing is bounded by the
    // program's own length. The handler runs as a subroutine; its RETURN clears
    // in_event_.
    if (timer_enabled_ && !timer_stopped_ && !in_event_ && main_step_done_ &&
        timer_handler_pc_ >= 0 && timer_interval_ > 0.0) {
      const double now = host_->SecondsSinceMidnight();
      if (now - timer_last_ >= timer_interval_) {
        timer_last_ = now;
        main_step_done_ = false;
        in_event_ = true;
        gosub_stack_.push_back(pc_);
        pc_ = timer_handler_pc_;
      }
    }
    // KEY(n) ON: fire the handler when a key is pending. Our input model delivers
    // characters rather than scan codes, so any pending key triggers the lowest
    // armed trap; the character is kept in pending_key_ and handed back to INKEY$.
    if (!in_event_ && main_step_done_) {
      int armed = -1;
      for (std::size_t index = 0; index < key_state_.size(); ++index) {
        if (key_state_[index] == 1 && on_key_pc_[index] >= 0) {
          armed = static_cast<int>(index);
          break;
        }
      }
      if (armed >= 0) {
        wchar_t key = 0;
        if (screen_.TryGetKey(&key)) {
          pending_key_ = key;
          main_step_done_ = false;
          in_event_ = true;
          gosub_stack_.push_back(pc_);
          pc_ = on_key_pc_[static_cast<std::size_t>(armed)];
        }
      }
    }
    // A break reaches us two ways: the console control handler sets the flag, and the
    // windowed terminal leaves Ctrl-C (0x03) in its key queue.
    const bool break_requested =
        (break_flag_ != nullptr && break_flag_->load(std::memory_order_relaxed)) ||
        screen_.TakeBreak();
    if (break_requested) {
      last_.status = RunResult::Status::Break;
      last_.line = LineAtPc(pc_ - 1);
      return last_;
    }
    if (screen_.Closed()) {
      // The user closed the window. Ending the run is what "wait until it is closed"
      // means; without this the program would keep looping into a destroyed window.
      last_.status = RunResult::Status::Done;
      return last_;
    }
  }
  screen_.PresentIfDirty();
  return last_;
}

int Vm::LineAtPc(int pc) const {
  const std::optional<int> line = program_.LineForPc(pc);
  return line.value_or(0);
}

} // namespace lwbasic
