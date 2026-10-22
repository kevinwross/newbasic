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

StopState Debugger::Continue() {
  // A breakpoint on the line we are stopped at is not re-reported until
  // execution leaves that line (so continuing always makes progress), while
  // a fresh Continue reports the first breakpoint it reaches.
  int skip_line = resume_line_;
  resume_line_ = 0;
  for (;;) {
    const StopState state = StepInstruction();
    if (state.AtEnd()) {
      vm_.PresentNow();
      return state;
    }
    const int line = CurrentLine();
    if (skip_line != 0 && line != skip_line)
      skip_line = 0;
    if (line != 0 && line != skip_line && HasBreakpoint(line) &&
        (!HasCondition(line) || ConditionTrue(Condition(line)))) {
      resume_line_ = line;
      StopState hit = state;
      hit.reason = StopReason::Breakpoint;
      hit.line = line;
      vm_.PresentNow();
      return hit;
    }
  }
}

StopState Debugger::StepBack() {
  if (history_.empty())
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
