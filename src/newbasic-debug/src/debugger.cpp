#include "newbasic/debugger.h"

namespace newbasic::debug {

StopState Debugger::StateFor(StopReason reason) const {
  StopState state;
  state.reason = reason;
  state.pc = vm_.Pc();
  state.line = vm_.LineAtPc(vm_.Pc());
  state.result = vm_.Last();
  return state;
}

void Debugger::RecordHistory() {
  if (!history_enabled_)
    return;
  const int line = CurrentLine();
  // One snapshot per source line: enough for line-granularity rewind without
  // copying state for every instruction.
  if (line == last_recorded_line_ && !history_.empty())
    return;
  last_recorded_line_ = line;
  history_.push_back(vm_.SaveState());
  if (history_.size() > history_limit_) {
    history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(
                                                            history_.size() - history_limit_));
  }
}

StopState Debugger::StepInstruction() {
  RecordHistory(); // the state *before* this instruction runs
  const bool more = vm_.Step();
  vm_.Present(); // a windowed debug session must show output as it is produced
  if (more)
    return StateFor(StopReason::Step);
  if (vm_.TakeTrap()) {
    // A trapped error resumed at the ON ERROR handler: still a step.
    return StateFor(StopReason::Step);
  }
  const RunResult& last = vm_.Last();
  return StateFor(last.status == RunResult::Status::Error ? StopReason::Error
                                                          : StopReason::ProgramEnd);
}

StopState Debugger::StepLine() {
  const int start_line = CurrentLine();
  for (;;) {
    const StopState state = StepInstruction();
    if (state.AtEnd()) {
      vm_.PresentNow();
      return state;
    }
    if (CurrentLine() != start_line) {
      vm_.PresentNow();
      return state;
    }
  }
}

StopState Debugger::StepOver() {
  const int start_line = CurrentLine();
  const std::size_t start_depth = vm_.CallDepth();
  for (;;) {
    const StopState state = StepInstruction();
    if (state.AtEnd()) {
      vm_.PresentNow();
      return state;
    }
    // Stop at the next line only after any GOSUB/CALL made from the starting
    // line has returned. A line inside the callee is not a step-over stop.
    if (vm_.CallDepth() <= start_depth && CurrentLine() != start_line) {
      vm_.PresentNow();
      return state;
    }
  }
}

StopState Debugger::StepOut() {
  const std::size_t start_depth = vm_.CallDepth();
  for (;;) {
    const StopState state = StepInstruction();
    if (state.AtEnd()) {
      vm_.PresentNow();
      return state;
    }
    // A top-level Step Out has no caller: run to the end. Otherwise stop as
    // soon as the current GOSUB/CALL activation has returned.
    if (start_depth > 0 && vm_.CallDepth() < start_depth) {
      vm_.PresentNow();
      return state;
    }
  }
}

StopState Debugger::Continue() {
  // A breakpoint on the line we are stopped at is not re-reported until
  // execution leaves that line (so continuing always makes progress), while
  // a fresh Continue reports the first breakpoint it reaches.
  int skip_line = resume_line_;
  resume_line_ = 0;
  int steps_since_present = 0;
  for (;;) {
    // A pause is a one-shot breakpoint at the current instruction. Consume the
    // flag here, on the thread that owns the VM, so the reader thread never
    // touches debugger state.
    if (pause_requested_ != nullptr && pause_requested_->exchange(false)) {
      const int line = CurrentLine();
      resume_line_ = line;
      StopState paused = StateFor(StopReason::Pause);
      paused.line = line;
      vm_.PresentNow();
      return paused;
    }

    // Fast continue path: do not go through StepInstruction (which records
    // history, builds a StopState and presents on every instruction). Snapshot
    // once per line when history is enabled, present only when the line
    // changes, and check breakpoints without constructing a full stop state.
    const int line_before = CurrentLine();
    if (history_enabled_ && line_before != last_recorded_line_)
      RecordHistory();
    const bool more = vm_.Step();
    if (!more && !vm_.TakeTrap()) {
      vm_.PresentNow();
      const RunResult& last = vm_.Last();
      return StateFor(last.status == RunResult::Status::Error ? StopReason::Error
                                                              : StopReason::ProgramEnd);
    }

    const int line = CurrentLine();
    ++steps_since_present;
    if (line != line_before) {
      if (skip_line != 0 && line != skip_line)
        skip_line = 0;
      vm_.Present();
      steps_since_present = 0;
    } else if (steps_since_present >= 256) {
      // Keep a same-line input loop responsive (Present pumps the window)
      // without paying for a present on every instruction.
      vm_.Present();
      steps_since_present = 0;
    }
    if (line != 0 && line != skip_line && HasBreakpoint(line) &&
        (!HasCondition(line) || ConditionTrue(Condition(line)))) {
      resume_line_ = line;
      StopState hit = StateFor(StopReason::Breakpoint);
      hit.line = line;
      vm_.PresentNow();
      return hit;
    }
  }
}

StopState Debugger::StepBack() {
  if (!history_enabled_ || history_.empty())
    return StateFor(StopReason::None);
  const Vm::Snapshot previous = history_.back();
  history_.pop_back();
  vm_.RestoreState(previous);
  vm_.PresentNow();
  // Do not immediately re-record the line we just rewound to.
  last_recorded_line_ = CurrentLine();
  return StateFor(StopReason::Step);
}

StopState Debugger::ReverseContinue() {
  const int line = CurrentLine();
  StopState state = StateFor(StopReason::None);
  while (!history_.empty()) {
    state = StepBack();
    if (state.line != line)
      return state;
  }
  return state; // history exhausted: report wherever the rewind stopped
}

std::vector<std::string> Debugger::VariableNames() const {
  std::vector<std::string> names;
  names.reserve(vm_.Vars().size());
  for (const auto& entry : vm_.Vars())
    names.push_back(entry.first);
  return names;
}

std::vector<int> Debugger::ProgramLines() const {
  std::vector<int> lines;
  lines.reserve(vm_.ProgramRef().LineTable().size());
  for (const LineSpan& span : vm_.ProgramRef().LineTable())
    lines.push_back(span.line);
  return lines;
}

} // namespace newbasic::debug
