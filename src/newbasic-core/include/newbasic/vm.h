#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "newbasic/draw.h"
#include "newbasic/host.h"
#include "newbasic/screen/screen.h"
#include "newbasic/value.h"

namespace newbasic {

class LineStore; // stored source lines, used by the program-editing statements

// Built-in functions (Op::CallFunc operand a).
enum class Func : std::int32_t {
  Abs = 0,
  Asc,
  Atn,
  Cdbl,
  Chr,
  Cint,
  Cos,
  Csng,
  Csrlin,
  Cvd,
  Cvi,
  Cvs,
  Date,
  Eof,
  Err,
  Erl,
  Exp,
  Fix,
  Hex,
  Inkey,
  Instr,
  Int,
  Left,
  Len,
  Lof,
  Log,
  Loc,
  Mid,
  Mkd,
  Mki,
  Mks,
  Oct,
  Pos,
  Right,
  Rnd,
  Sgn,
  Sin,
  Space,
  Sqr,
  Str,
  String,
  Tan,
  Time,
  Timer,
  Val,
  // Machine-level functions (M13): recognised so they produce a clear refusal
  // instead of a syntax error. There is no machine model to expose.
  Fre,
  Inp,
  Peek,
  Point,
  Pmap, // PMAP(x, which): world to screen (0/2) or screen to world (1/3)
        // Pointing device (extension): MOUSE(n) plus MOUSEX/MOUSEY/MOUSEB aliases.
  Mouse,
  MouseX,
  MouseY,
  MouseB,
  Stick,
  Strig,
  Usr,
  Varptr,
  Count,
};

const char* FuncName(Func f);
// True when the function returns a string.
bool FuncIsString(Func f);
// Fixed argument count, or -1 when the function takes a variable count.
int FuncArity(Func f);

// Stack-based instruction set (plan section 3).
enum class Op : std::uint8_t {
  // constants / variables
  PushInt,
  PushLong,
  PushSgl,
  PushDbl,
  PushStr,
  LoadVar,  // name
  StoreVar, // name
  // arrays (M2): a = subscript/dimension count, b = lower bound (OPTION BASE)
  ArrayLoad,
  ArrayStore,
  ArrayDim,
  Erase, // name = comma-separated array names
  // arithmetic
  Add,
  Sub,
  Mul,
  Div,
  IntDiv,
  Mod,
  Pow,
  Neg,
  // relational (result Int: -1 true / 0 false)
  Eq,
  Ne,
  Lt,
  Gt,
  Le,
  Ge,
  // logical / bitwise
  And,
  Or,
  Xor,
  Eqv,
  Imp,
  Not,
  // built-in function call (a = Func, b = argument count)
  CallFunc,
  // PRINT machinery
  Emit,      // print top of stack with the pending separator
  SepComma,  // next value starts at the next 14-column zone
  SepSemi,   // next value starts immediately
  LineEnd,   // CR/LF after the printed list
  NoLineEnd, // suppress the line end (trailing ';')
  Tab,       // print TAB(n): move to column n
  Spc,       // print SPC(n): n spaces
  Using,     // PRINT USING: pop the format string, set field formatting
  // screen
  Cls,
  Clear,       // CLEAR: variables, arrays, stacks and files (not the program)
  ScreenMode,  // SCREEN n: pop the mode (0 text, 1/2/9 graphics)
  PSet,        // PSET (x,y)[,c]: pop colour, y, x
  Line,        // LINE (x1,y1)-(x2,y2)[,c]: pop colour, y2, x2, y1, x1
  Circle,      // CIRCLE (x,y),r[,...]: pop aspect, end, start, colour, r, y, x
  Paint,       // PAINT (x,y)[,fill[,border]]: pop border, fill, y, x
  Draw,        // DRAW macro$: pop the macro and interpret it
  Palette,     // PALETTE [attribute, colour]: a = 1 when arguments were given
  Window,      // WINDOW [[SCREEN](x1,y1)-(x2,y2)]: a = 1 for SCREEN, 0 for no args
  View,        // VIEW [[SCREEN](x1,y1)-(x2,y2)[,[fill][,[border]]]]: a = 0 for no args
  GetImage,    // GET (x1,y1)-(x2,y2), array: name = array
  PutImage,    // PUT (x,y), array [, action]: name = array, a = action
  Pcopy,       // PCOPY source, destination: copies a graphics page
  ViewPrint,   // pops bottom, top (1-based; both 0 restores the whole screen)
  Width,       // pops the column count
  KeySet,      // pops value, key number (KEY n, "string")
  KeyOn,       // show the function-key bar on the last line
  KeyOff,      // clear the bar and give the last line back
  KeyList,     // list the key assignments
  Field,       // FIELD #n: pop file number; name = "var:width,..." bindings
  LSet,        // LSET/RSET: pop value; name = variable; a = 1 for RSET
  GetRecord,   // GET #n[, rec]: a = 1 when a record number was given
  PutRecord,   // PUT #n[, rec]
  Run,         // RUN [line]: restart the program (a = 1 when a line was given)
  Tron,        // TRON: trace line numbers as they execute
  Troff,       // TROFF
  List,        // LIST [[first][-[last]]]
  LList,       // LLIST: the same listing, to the printer
  LPrint,      // LPRINT: start a printer-bound print list
  Delete,      // DELETE first[-last]
  New,         // NEW: clear the stored program and stop
  SaveProgram, // pops a path: write the stored program as text
  LoadProgram, // pops a path: load it, replace the program, clear variables
  Chain,       // CHAIN: pop path (and optional line); keep COMMON variables
  Common,      // COMMON var[, var]: names that survive CHAIN
  OnKey,       // ON KEY(n) GOSUB: a = handler pc, b = key number (1-based)
  KeyControl,  // KEY(n) ON/OFF/STOP: a = key number, b = 1 on, 0 off, 2 stop
  Fail,        // report a specific error: a = error code, name = message
  Locate,      // pops column then row (both 1-based)
  Color,       // pops background then foreground (-1 = leave unchanged)
  // input
  Input,     // name = comma-separated variable list; b = 1 to print "? "
  LineInput, // name = string variable
  // sound (M7): all three go through Host::PlayTone
  Beep,  // 800 Hz for a quarter second
  Sound, // pops duration (clock ticks) then frequency (Hz) and plays it
  Play,  // pops a PLAY string and plays the tones it describes
  // control flow
  Jump,        // a = target pc
  JumpIfFalse, // a = target pc
  Gosub,       // a = target pc
  Return,
  ForStart,   // name; a = pc to skip to when the loop body is skipped
  ForNext,    // name (empty = innermost frame)
  OnJump,     // a = jump-table index, b = 0 GOTO / 1 GOSUB
  Randomize,  // b = 0 seed from the stack, 1 seed from the host clock
  Swap,       // name = "A,B"
  Read,       // name = comma-separated variable list
  ReadArray,  // name = array, a = subscript count already pushed, value from DATA
  Restore,    // a = line number, 0 = first DATA item
  CallUser,   // a = user-function index, b = argument count (DEF FN)
  UserReturn, // end of a DEF FN body: returns the top of stack
  // error handling (M3)
  OnError,   // a = handler pc, -1 disables trapping (ON ERROR GOTO 0)
  Resume,    // b = 0 retry, 1 next, 2 line (a = target pc)
  ErrorStmt, // pops an error code and raises it
  Shell,     // b = 1: pops a command string; 0: interactive interpreter
  // sequential files (M3): b = mode 0 INPUT / 1 OUTPUT / 2 APPEND / 3 from a string
  Open,
  Close,     // b = 1: pops a file number; 0: close all
  FilePrint, // pops a file number; the PRINT list then goes to that file
             // (a = 1 for WRITE#, which quotes strings and uses commas)
  FileInput, // pops a file number; name = variable list, b = 1 for LINE INPUT#
  // event trapping (M3): timer events only (KEY/PLAY/COM/STRIG/PEN are not
  // implemented)
  OnTimer,      // pops the interval; a = handler pc
  TimerControl, // a = 1 ON, 0 OFF, 2 STOP
  // recognized but unimplemented statement (a = 0 GOTO / 1 direct error)
  Unsupported, // name = statement name
  End,
  Stop,
  Nop,
};

struct Instr {
  Op op = Op::End;
  Value value{};      // constant operands
  std::string name;   // variable / statement name
  std::int32_t a = 0; // target pc, function id, row, ...
  std::int32_t b = 0; // argument count, column, flags, ...
};

// Line number -> inclusive pc range.
struct LineSpan {
  int line = 0;
  int pc_begin = 0;
  int pc_end = 0;
};

// DATA item (M2): the raw text (quotes stripped) plus its parsed value when
// it is numeric. READ converts a numeric item into the target variable's
// type; reading a non-numeric item into a numeric variable is a syntax error.
struct DataItem {
  std::string text;
  Value number{};
  bool numeric = false;
};

// User-defined function (M2, DEF FN): a compiled body in the instruction
// stream plus its parameter names.
struct UserFunc {
  std::string name;                // canonical, e.g. "FNA!"
  std::vector<std::string> params; // canonical parameter names
  int body_pc = 0;
};

class Program {
public:
  const std::vector<Instr>& Instrs() const { return instrs_; }
  const std::vector<LineSpan>& LineTable() const { return line_table_; }
  // ON ... GOTO/GOSUB destination tables (patched like line targets).
  const std::vector<std::vector<int>>& JumpTables() const { return jump_tables_; }

  // Pc range for a line number, or nullopt if the line has no code.
  std::optional<std::pair<int, int>> PcRangeForLine(int line) const;
  // Line number containing pc (by instruction), or nullopt.
  std::optional<int> LineForPc(int pc) const;

  std::vector<Instr> instrs_;
  std::vector<LineSpan> line_table_; // ascending by line
  std::vector<std::vector<int>> jump_tables_;
  std::vector<DataItem> data_items_;   // DATA values in program order
  std::map<int, int> line_data_index_; // line with DATA -> first item index
  std::vector<UserFunc> user_funcs_;   // DEF FN definitions
  // First pc of every statement, ascending: ON ERROR/RESUME work at
  // statement granularity (RESUME retries a statement, RESUME NEXT skips it).
  std::vector<int> statement_pcs_;
};

// Result of a Run() call.
struct RunResult {
  enum class Status { Done, Error, Break } status = Status::Done;
  int line = 0;
  int code = 0; // GW-BASIC error code
  std::string message;
};

// FOR/NEXT frame (kept in a vector: VM state stays cheap to snapshot, which
// time-travel debugging in M7 needs).
struct ForFrame {
  std::string name; // canonical variable name
  double end = 0.0;
  double step = 1.0;
  int body_pc = 0; // pc just after ForStart
};

// Array storage (M2): typed by the variable's type suffix, with a per-array
// lower bound (OPTION BASE) and an upper bound of 10 per dimension when first
// used without DIM.
struct ArrayValue {
  Value::Kind kind = Value::Kind::Sgl;
  int base = 0;
  std::vector<int> dims; // upper bounds
  std::vector<Value> data;
};

// Canonical variable key: upper-cased, with the default type character (!)
// appended when no type suffix is given, so `A` and `A!` are the same
// variable and `A%`/`A#`/`A$`/`A&` stay distinct.
std::string CanonicalVarName(const std::string& name);
// Declared type character of a variable name ('%', '&', '!', '#', '$').
char VarTypeSuffix(const std::string& name);

class Vm {
public:
  explicit Vm(screen::Screen& screen, Host* host = nullptr,
              std::atomic<bool>* break_flag = nullptr);

  // Replaces the program without touching variables (direct-mode statements
  // run in the same VM so variables persist).
  void Load(Program program);
  // RUN: clears variables, stacks and PRINT state, and rewinds to the first
  // instruction. CLEAR does the same without rewinding.
  void Reset();
  void ClearState();
  RunResult Run();
  // Debugger seam (M5): execute one instruction. Returns false when the VM
  // halts (End/Stop/Break/Error) and keeps returning false afterwards: a
  // debugger stepping past the end must not execute whatever code happens to
  // follow the last instruction (handler bodies, for instance).
  bool Step();

private:
  // One instruction, without the halted guard that Step() applies.
  bool StepImpl();
  bool halted_ = false; // set once Step() has reported a halt

public:
  // --- inspection (M5) ---
  // Active GOSUB/FOR frames, innermost last within each kind. GOSUB frames
  // come first because a subroutine call is the outer context of a loop.
  struct CallFrame {
    enum class Kind { Gosub, For_ } kind = Kind::Gosub;
    int line = 0;     // GOSUB: return line; FOR: the loop's line
    std::string name; // FOR: canonical loop variable
  };
  std::vector<CallFrame> CallStack() const {
    std::vector<CallFrame> frames;
    for (int return_pc : gosub_stack_) {
      CallFrame frame;
      frame.kind = CallFrame::Kind::Gosub;
      frame.line = LineAtPc(return_pc);
      frames.push_back(frame);
    }
    for (const ForFrame& loop : for_stack_) {
      CallFrame frame;
      frame.kind = CallFrame::Kind::For_;
      frame.name = loop.name;
      frame.line = LineAtPc(loop.body_pc - 1);
      frames.push_back(frame);
    }
    return frames;
  }

  // Dimensioned (or auto-created) arrays.
  struct ArrayInfo {
    std::string name;
    Value::Kind kind = Value::Kind::Sgl;
    int base = 0;
    std::vector<int> dims; // upper bounds
    std::size_t elements = 0;
  };
  std::vector<ArrayInfo> Arrays() const {
    std::vector<ArrayInfo> out;
    for (const auto& entry : arrays_) {
      ArrayInfo info;
      info.name = entry.first;
      info.kind = entry.second.kind;
      info.base = entry.second.base;
      info.dims = entry.second.dims;
      info.elements = entry.second.data.size();
      out.push_back(info);
    }
    return out;
  }

  // One array element, or nullopt when the array or subscript is invalid.
  std::optional<Value> ArrayElement(const std::string& name,
                                    const std::vector<int>& subscripts) const {
    const auto it = arrays_.find(CanonicalVarName(name));
    if (it == arrays_.end())
      return std::nullopt;
    const ArrayValue& array = it->second;
    if (subscripts.size() != array.dims.size())
      return std::nullopt;
    std::size_t offset = 0;
    for (std::size_t i = 0; i < subscripts.size(); ++i) {
      const int extent = array.dims[i] - array.base + 1;
      const int index = subscripts[i] - array.base;
      if (index < 0 || index >= extent)
        return std::nullopt;
      offset = offset * static_cast<std::size_t>(extent) + static_cast<std::size_t>(index);
    }
    if (offset >= array.data.size())
      return std::nullopt;
    return array.data[offset];
  }

  int Pc() const { return pc_; }
  void SetPc(int pc) {
    pc_ = pc;
    halted_ = false; // a debugger rewind re-arms execution
  }

  // --- snapshots (M5) ---
  // Diagnostic (M3): the timer poll's runtime state, used by the timer tests to
  // see the values the poll actually compares rather than the ones the source
  // implies.
  std::string TimerDebug() const {
    return "enabled=" + std::to_string(timer_enabled_ ? 1 : 0) +
           " stopped=" + std::to_string(timer_stopped_ ? 1 : 0) +
           " interval=" + std::to_string(timer_interval_) +
           " handler_pc=" + std::to_string(timer_handler_pc_) +
           " last=" + std::to_string(timer_last_) +
           " in_event=" + std::to_string(in_event_ ? 1 : 0) +
           " step_done=" + std::to_string(main_step_done_ ? 1 : 0);
  }
  // A complete copy of the interpreter's execution state (everything except
  // open files, which stay attached to the process). Taking and restoring
  // snapshots is what makes rewind/time-travel debugging possible later.
  struct Snapshot {
    int pc = 0;
    std::vector<Value> stack;
    std::map<std::string, Value> vars;
    std::map<std::string, ArrayValue> arrays;
    std::vector<ForFrame> for_stack;
    std::vector<int> gosub_stack;
    // DEF FN frames flattened to basic types (FuncFrame is declared below).
    std::vector<std::pair<int, std::vector<std::pair<std::string, Value>>>> func_stack;
    std::size_t data_index = 0;
    bool at_line_start = true;
    int pending_sep = 0; // Sep as an int (the enum is private)
    bool trap_pending = false;
    bool handling_error = false;
    int error_handler_pc = -1;
    int error_pc = -1;
    int error_next_pc = -1;
    int last_error_code = 0;
    int last_error_line = 0;
    std::mt19937 rng;
    double last_rnd = 0.0;
    RunResult last;
    bool halted = false; // part of the state: a rewind may resume a halted run
  };

  Snapshot SaveState() const {
    Snapshot state;
    state.pc = pc_;
    state.stack = stack_;
    state.vars = vars_;
    state.arrays = arrays_;
    state.for_stack = for_stack_;
    state.gosub_stack = gosub_stack_;
    for (const FuncFrame& frame : func_stack_) {
      state.func_stack.emplace_back(frame.return_pc, frame.saved);
    }
    state.data_index = data_index_;
    state.at_line_start = at_line_start_;
    state.pending_sep = static_cast<int>(pending_sep_);
    state.trap_pending = trap_pending_;
    state.handling_error = handling_error_;
    state.error_handler_pc = error_handler_pc_;
    state.error_pc = error_pc_;
    state.error_next_pc = error_next_pc_;
    state.last_error_code = last_error_code_;
    state.last_error_line = last_error_line_;
    state.rng = rng_;
    state.last_rnd = last_rnd_;
    state.last = last_;
    state.halted = halted_;
    return state;
  }

  void RestoreState(const Snapshot& state) {
    pc_ = state.pc;
    stack_ = state.stack;
    vars_ = state.vars;
    arrays_ = state.arrays;
    for_stack_ = state.for_stack;
    gosub_stack_ = state.gosub_stack;
    func_stack_.clear();
    for (const auto& entry : state.func_stack) {
      FuncFrame frame;
      frame.return_pc = entry.first;
      frame.saved = entry.second;
      func_stack_.push_back(std::move(frame));
    }
    data_index_ = state.data_index;
    at_line_start_ = state.at_line_start;
    pending_sep_ = static_cast<Sep>(state.pending_sep);
    trap_pending_ = state.trap_pending;
    handling_error_ = state.handling_error;
    error_handler_pc_ = state.error_handler_pc;
    error_pc_ = state.error_pc;
    error_next_pc_ = state.error_next_pc;
    last_error_code_ = state.last_error_code;
    last_error_line_ = state.last_error_line;
    rng_ = state.rng;
    last_rnd_ = state.last_rnd;
    last_ = state.last;
    halted_ = state.halted;
  }

  const std::vector<Value>& Stack() const { return stack_; }
  int LineAtPc(int pc) const;
  const Program& ProgramRef() const { return program_; }
  // The stored source lines, needed by the program-editing statements (LIST,
  // DELETE, NEW). Optional: without one, those statements report Advanced Feature.
  void SetLineStore(LineStore* store) { store_ = store; }
  bool AtLineStart() const { return at_line_start_; }
  const RunResult& Last() const { return last_; }
  // True when the last Step() trapped an error and jumped to the ON ERROR
  // handler; the Run loop (or the debugger) clears it to keep going.
  bool TrapPending() const { return trap_pending_; }
  bool TakeTrap() {
    const bool value = trap_pending_;
    trap_pending_ = false;
    return value;
  }

  // Variables (tests + debugger).
  const std::map<std::string, Value>& Vars() const { return vars_; }
  std::optional<Value> GetVar(const std::string& name) const;
  void SetVar(const std::string& name, const Value& value);

  // Deterministic RND (tests). GW-BASIC seeds its generator identically on
  // startup, so RND is reproducible until RANDOMIZE.
  void SeedRandom(std::uint32_t seed);
  screen::Screen& ScreenRef() { return screen_; }
  Host& HostRef() { return *host_; }

private:
  enum class Sep { None, Comma, Semi };

  // --- stack ---
  Value Pop();
  void Push(const Value& v);
  // --- error handling ---
  bool Fail(int code);
  bool Fail(int code, const std::string& message);
  // --- helpers ---
  bool Number(const Value& v, double* out);      // error 13 on strings
  bool String(const Value& v, std::string* out); // error 13 on numbers
  bool StoreVariable(const std::string& name, const Value& v);
  Value LoadVariable(const std::string& name);
  bool EmitValue(const Value& v); // PRINT one value (used by Emit and WRITE)
  bool CallFunction(Func f, int argc);
  bool DoForStart(const Instr& in);
  bool DoForNext(const Instr& in);
  bool DoInput(const Instr& in, bool line_input);
  bool DoRead(const Instr& in);
  void CloseAllFiles();
  // Statement boundaries around an instruction (error handling).
  int StatementStart(int pc) const;
  int StatementNext(int pc) const;
  // Store `v` converted to the kind implied by a variable name.
  bool ConvertForName(const std::string& name, const Value& v, Value* out);
  void ResetPrintState();
  // --- arrays ---
  ArrayValue* FindArray(const std::string& name);
  ArrayValue* EnsureArray(const std::string& name, const std::vector<int>& upper_bounds, int base);
  bool ArrayOffset(const ArrayValue& array, const std::vector<int>& subscripts, std::size_t* out);

  screen::Screen& screen_;
  Host* host_;
  Host default_host_{};
  std::atomic<bool>* break_flag_;
  Program program_;
  std::vector<Value> stack_;
  std::map<std::string, Value> vars_;
  std::map<std::string, ArrayValue> arrays_;
  std::vector<ForFrame> for_stack_;
  std::vector<int> gosub_stack_;
  std::size_t data_index_ = 0; // next DATA item for READ
  // Active DEF FN calls: return address plus the parameters' saved values.
  struct FuncFrame {
    int return_pc = 0;
    std::vector<std::pair<std::string, Value>> saved;
  };
  std::vector<FuncFrame> func_stack_;
  // Sequential files (M3): file number -> stream, plus the redirect state
  // used while a PRINT#/WRITE# list is being built.
  std::map<int, std::fstream> files_;
  // Random-access files (M9): one record buffer per open file, with the FIELD
  // bindings that view into it.
  struct FieldBinding {
    std::string name;
    int offset = 0;
    int width = 0;
  };
  struct RandomFile {
    int record_length = 128; // GW-BASIC's default record length
    int allocated = 0;       // bytes already handed out by FIELD
    std::vector<FieldBinding> fields;
    std::string buffer;
    int last_record = 0; // LOC
  };
  std::map<int, RandomFile> random_files_;
  int print_file_ = -1;
  bool write_mode_ = false;
  std::string print_buffer_;
  // Timer event trap: ON TIMER(n) GOSUB <line> + TIMER ON/OFF/STOP.
  double timer_interval_ = 0.0;
  double timer_last_ = 0.0;
  int timer_handler_pc_ = -1;
  bool timer_enabled_ = false;
  bool timer_stopped_ = false;
  bool in_event_ = false;
  // True once a main-program instruction has run since the last event: an
  // event may only fire after real progress, otherwise a fast clock would
  // re-enter the handler before the program advances (an unbounded loop).
  bool main_step_done_ = false;
  // Error handling: ON ERROR GOTO target, the failing instruction (RESUME)
  // and the one after it (RESUME NEXT), plus ERR/ERL state.
  bool trap_pending_ = false;
  bool handling_error_ = false;
  int error_handler_pc_ = -1;
  int error_pc_ = -1;
  int error_next_pc_ = -1;
  int last_error_code_ = 0;
  int last_error_line_ = 0;
  std::mt19937 rng_;
  double last_rnd_ = 0.0;
  bool has_rnd_ = false;
  int pc_ = 0;
  bool at_line_start_ = true;
  Sep pending_sep_ = Sep::None;
  // PRINT USING state: active while a USING list is being printed.
  bool using_active_ = false;
  std::string using_format_;
  std::size_t using_pos_ = 0;
  // LPRINT collects its items into print_buffer_ and hands them to the printer on
  // the line end, the same way PRINT # collects a record.
  bool printer_mode_ = false;
  // DRAW keeps its own current graphics position between statements (M12 phase 2),
  // re-centred when the canvas changes size.
  DrawState draw_state_;
  int draw_width_ = 0;
  int draw_height_ = 0;
  // The last graphics point referenced, which LINE uses when its first coordinate is
  // omitted and STEP coordinates are relative to.
  int last_x_ = 0;
  // Last click reported by the pointing device, so MOUSEX and MOUSEY can follow MOUSEB.
  int mouse_x_ = 0;
  int mouse_y_ = 0;
  int last_y_ = 0;
  // WINDOW (M12 phase 3): a world-coordinate rectangle mapped onto the canvas, with
  // the SCREEN form putting y downward. Inactive means plain screen coordinates.
  bool window_active_ = false;
  bool window_screen_ = false;
  double window_x1_ = 0.0;
  double window_y1_ = 0.0;
  double window_x2_ = 0.0;
  double window_y2_ = 0.0;
  // Maps a world coordinate to a canvas pixel; the identity when no WINDOW is set.
  void MapGraphicsPoint(double x, double y, int* sx, int* sy) const;
  // KEY: function-key macros (1-10 and 15-20, as the reference allows) and whether
  // the bar is showing on the last screen line.
  std::array<std::string, 20> key_values_{};
  bool key_bar_ = false;
  void DrawKeyBar();
  // TRON state: printing a line number when execution enters a new line.
  bool trace_ = false;
  int traced_line_ = -1;
  // Program-editing statements (M10) work on the stored source lines, which the
  // caller owns; without a store they report that the operation is unavailable.
  class LineStore* store_ = nullptr;
  // COMMON: scalar variables preserved across CHAIN.
  std::set<std::string> common_vars_;
  // KEY(n) trapping (M11). Key numbers are 1..10; state is 0 off, 1 on, 2 stop.
  // Handler slots start at -1: 0 would be a real instruction index.
  std::array<int, 10> on_key_pc_{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::array<int, 10> key_state_{};
  // A key consumed to fire a trap, handed back to INKEY$ so the character is not
  // lost.
  wchar_t pending_key_ = 0;
  RunResult last_;
};

} // namespace newbasic
