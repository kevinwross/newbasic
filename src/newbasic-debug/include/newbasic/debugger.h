#pragma once

#include <atomic>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "newbasic/frontend.h"
#include "newbasic/value.h"
#include "newbasic/vm.h"

namespace newbasic::debug {

// Why the debugger stopped.
enum class StopReason { None, Step, Breakpoint, Pause, ProgramEnd, Error };

struct StopState {
  StopReason reason = StopReason::None;
  int line = 0;     // source line at the stop (0 = none)
  int pc = 0;       // instruction pointer
  RunResult result; // last run result (filled for Error/ProgramEnd)

  bool AtEnd() const { return reason == StopReason::ProgramEnd || reason == StopReason::Error; }
};

// In-process debugger over a Vm (plan section 5): line breakpoints, stepping
// and inspection. Deliberately free of any DAP concerns so that newbasic-dap
// can drive it, and so it can be unit-tested against a headless screen.
//
// This lives in its own newbasic-debug library, which newbasic-core does not mention:
// newbasic-dap and the tests link it. See docs/m5-dap-debugger.md for how a session runs.
class Debugger {
public:
  explicit Debugger(Vm& vm) : vm_(vm) {}

  // --- breakpoints ---
  void SetBreakpoint(int line) { breakpoints_.insert(line); }
  void ClearBreakpoint(int line) { breakpoints_.erase(line); }
  void ClearBreakpoints() { breakpoints_.clear(); }
  bool HasBreakpoint(int line) const { return breakpoints_.count(line) != 0; }
  const std::set<int>& Breakpoints() const { return breakpoints_; }

  // Conditional breakpoints: the condition is a BASIC expression evaluated in
  // the stopped context, and the break applies only when it is non-zero.
  void SetCondition(int line, std::string condition) { conditions_[line] = std::move(condition); }
  bool HasCondition(int line) const { return conditions_.count(line) != 0; }
  const std::string& Condition(int line) const {
    static const std::string kEmpty;
    const auto it = conditions_.find(line);
    return it == conditions_.end() ? kEmpty : it->second;
  }

  // Evaluates `condition` against the current state without disturbing it:
  // the whole VM (including the loaded program) is restored afterwards.
  bool ConditionTrue(const std::string& condition) {
    Program program;
    CompileError error;
    if (!CompileDirect("ZZCOND = (" + condition + ")", &program, &error)) {
      return false; // a malformed condition never breaks
    }
    Program saved_program = vm_.ProgramRef();
    const Vm::Snapshot saved = vm_.SaveState();
    vm_.Load(std::move(program));
    vm_.Run();
    const std::optional<Value> value = vm_.GetVar("ZZCOND");
    const bool result = value.has_value() && value->AsNumber() != 0.0;
    vm_.Load(std::move(saved_program)); // put the real program back first
    vm_.RestoreState(saved);            // then the execution state
    return result;
  }

  // --- execution ---
  // Optional asynchronous pause request. The DAP reader thread sets the flag;
  // Continue consumes it and stops at the next instruction, like a one-shot
  // breakpoint. The debugger never touches the flag memory itself.
  void SetPauseFlag(std::atomic<bool>* flag) { pause_requested_ = flag; }
  // Drops a pause that arrived while the VM was already stopped, so it does not
  // stop the next Continue immediately.
  void ClearPause() {
    if (pause_requested_ != nullptr)
      pause_requested_->store(false);
  }
  // Runs until a breakpoint, an error or the end of the program.
  StopState Continue();
  // Executes a single instruction.
  StopState StepInstruction();
  // Executes until execution reaches a different source line (or ends).
  StopState StepLine();

  // --- inspection ---
  int CurrentPc() const { return vm_.Pc(); }
  int CurrentLine() const { return vm_.LineAtPc(vm_.Pc()); }
  std::optional<Value> Variable(const std::string& name) const { return vm_.GetVar(name); }
  std::vector<std::string> VariableNames() const;
  const std::vector<Value>& Stack() const { return vm_.Stack(); }

  // Structural inspection: active GOSUB/FOR frames and arrays, surfaced here
  // so the DAP adapter (and anything else) drives one object.
  std::vector<Vm::CallFrame> CallFrames() const { return vm_.CallStack(); }
  std::vector<Vm::ArrayInfo> Arrays() const { return vm_.Arrays(); }
  std::optional<Value> ArrayElement(const std::string& name,
                                    const std::vector<int>& subscripts) const {
    return vm_.ArrayElement(name, subscripts);
  }

  // Execution state snapshots (rewind/time travel).
  Vm::Snapshot SaveState() const { return vm_.SaveState(); }
  void RestoreState(const Vm::Snapshot& state) { vm_.RestoreState(state); }

  // --- time travel (M7) ---
  // A snapshot is recorded at each line boundary while stepping, so undo costs
  // one line's worth of state. The ring is bounded: once the limit is reached
  // the oldest entries are dropped, which keeps memory predictable rather than
  // growing without limit on a long run.
  void SetHistoryLimit(std::size_t max_entries) {
    history_limit_ = max_entries < 1 ? 1 : max_entries;
    if (history_.size() > history_limit_) {
      history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(
                                                              history_.size() - history_limit_));
    }
  }
  std::size_t HistoryLimit() const { return history_limit_; }
  std::size_t HistorySize() const { return history_.size(); }
  void ClearHistory() {
    history_.clear();
    last_recorded_line_ = 0;
  }

  // Undoes the most recent recorded step. Returns reason None when there is
  // nothing left to undo (or no history was recorded).
  StopState StepBack();
  // Rewinds repeatedly until the source line differs from the current one.
  StopState ReverseContinue();

  // Source lines of the loaded program, ascending (LIST/stack traces).
  std::vector<int> ProgramLines() const;

private:
  StopState StateFor(StopReason reason) const;
  // Records the state before an instruction, once per source line.
  void RecordHistory();

  Vm& vm_;
  std::set<int> breakpoints_;
  std::map<int, std::string> conditions_;
  std::atomic<bool>* pause_requested_ = nullptr; // set by the DAP reader thread
  int resume_line_ = 0; // line of the last reported breakpoint stop
  std::vector<Vm::Snapshot> history_;
  std::size_t history_limit_ = 500;
  int last_recorded_line_ = 0;
};

} // namespace newbasic::debug
