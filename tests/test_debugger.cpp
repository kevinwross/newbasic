// M5: in-process debugger library (breakpoints, stepping, inspection).
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/debugger.h"
#include "newbasic/errors.h"
#include "test_helpers.h"

namespace {

struct DebugSession {
  explicit DebugSession(const std::string& source) {
    newbasic::CompileError error;
    if (!lwtest::CompileSource(source, &program, &error)) {
      FAIL("Compile failed (line " << error.line << "): " << error.message);
    }
    vm.Load(program);
  }

  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Program program;
  newbasic::Vm vm{screen, &host};
};

} // namespace

TEST_CASE("Debugger: breakpoints stop execution on a line", "[debug]") {
  DebugSession s("10 A=1\n20 A=2\n30 A=3\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(20);
  CHECK(debugger.HasBreakpoint(20));
  CHECK(debugger.Breakpoints().size() == 1);

  const newbasic::debug::StopState first = debugger.Continue();
  CHECK(first.reason == newbasic::debug::StopReason::Breakpoint);
  CHECK(first.line == 20);
  // Line 10 has run, line 20 has not.
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);

  const newbasic::debug::StopState second = debugger.Continue();
  CHECK(second.reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == 3.0);
}

TEST_CASE("Debugger: clearing breakpoints resumes to the end", "[debug]") {
  DebugSession s("10 A=1\n20 A=2\n30 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(10);
  debugger.SetBreakpoint(20);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);
  debugger.SetCondition(10, "A=1");
  debugger.ClearBreakpoint(10);
  CHECK_FALSE(debugger.HasBreakpoint(10));
  CHECK_FALSE(debugger.HasCondition(10));
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);
  debugger.SetCondition(20, "A=1");
  debugger.ClearBreakpoints();
  CHECK(debugger.Breakpoints().empty());
  CHECK_FALSE(debugger.HasCondition(20));
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
}

TEST_CASE("Debugger: single stepping", "[debug]") {
  DebugSession s("10 A=1\n20 A=2\n30 PRINT A\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  // Instruction stepping starts at line 10.
  const newbasic::debug::StopState one = debugger.StepInstruction();
  CHECK(one.reason == newbasic::debug::StopReason::Step);
  // Line stepping lands on each subsequent line.
  CHECK(debugger.StepLine().line == 20);
  CHECK(debugger.StepLine().line == 30);
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);
  CHECK(debugger.StepLine().line == 40); // the END statement
  const newbasic::debug::StopState last = debugger.StepLine();
  CHECK(last.AtEnd());
}

TEST_CASE("Debugger: errors stop with the GW-BASIC code", "[debug]") {
  DebugSession s("10 A=1\n20 PRINT 1/0\n30 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  const newbasic::debug::StopState state = debugger.Continue();
  CHECK(state.reason == newbasic::debug::StopReason::Error);
  CHECK(state.result.code == newbasic::kErrDivisionByZero);
  CHECK(state.line == 20);
}

TEST_CASE("Debugger: inspection", "[debug]") {
  DebugSession s("10 A=1\n20 B$=\"hi\"\n30 DIM C(3)\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  CHECK(debugger.ProgramLines() == std::vector<int>{10, 20, 30, 40});
  debugger.SetBreakpoint(30);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);
  CHECK(debugger.Variable("B$")->AsString() == "hi");
  CHECK_FALSE(debugger.Variable("NOPE").has_value());
  const std::vector<std::string> names = debugger.VariableNames();
  bool saw_a = false;
  bool saw_b = false;
  for (const std::string& name : names) {
    if (name == "A!")
      saw_a = true;
    if (name == "B$")
      saw_b = true;
  }
  CHECK(saw_a); // canonical names carry the default type suffix
  CHECK(saw_b);
  CHECK(debugger.CurrentPc() >= 0);
}

TEST_CASE("Debugger: stepping over a trapped error", "[debug]") {
  DebugSession s("10 ON ERROR GOTO 100\n20 ERROR 7\n30 A=1\n40 END\n100 RESUME NEXT\n");
  newbasic::debug::Debugger debugger(s.vm);
  const newbasic::debug::StopState state = debugger.Continue();
  CHECK(state.reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);
}

TEST_CASE("Debugger: a breakpoint in a loop hits on every iteration", "[debug]") {
  DebugSession s("10 FOR I=1 TO 3\n20 A=A+1\n30 NEXT I\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(20);
  // Stop-before-the-line semantics: at the first stop, line 20 has not run.
  CHECK(debugger.Continue().line == 20);
  // A has not been assigned yet at the first stop (the variable may not even
  // exist), so only assert the safe part.
  const std::optional<newbasic::Value> initial = debugger.Variable("A");
  CHECK((!initial.has_value() || initial->AsNumber() == 0.0));
  CHECK(debugger.Continue().line == 20);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);
  CHECK(debugger.Continue().line == 20);
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);
  const newbasic::debug::StopState last = debugger.Continue();
  CHECK(last.reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == 3.0);
}

TEST_CASE("Debugger: line stepping follows GOSUB and RETURN", "[debug]") {
  DebugSession s("10 GOSUB 100\n20 PRINT \"B\"\n30 END\n100 A=7\n110 RETURN\n");
  newbasic::debug::Debugger debugger(s.vm);
  CHECK(debugger.StepLine().line == 100); // into the subroutine
  CHECK(debugger.StepLine().line == 110);
  CHECK(debugger.StepLine().line == 20); // back from RETURN
  CHECK(debugger.Variable("A")->AsNumber() == 7.0);
  CHECK(debugger.StepLine().line == 30);
  CHECK(debugger.StepLine().AtEnd());
}

TEST_CASE("Debugger: variables set inside a subroutine stay visible", "[debug]") {
  DebugSession s("10 GOSUB 100\n20 B=A*2\n30 END\n100 A=21\n110 RETURN\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(20);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);
  CHECK(debugger.Variable("A")->AsNumber() == 21.0);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("B")->AsNumber() == 42.0);
}

TEST_CASE("Inspection: GOSUB and FOR call frames", "[debug]") {
  DebugSession s("10 DIM A(3)\n"
                 "20 A(2)=7\n"
                 "30 FOR I=1 TO 2\n"
                 "40 GOSUB 100\n"
                 "50 NEXT I\n"
                 "60 END\n"
                 "100 B=1\n"
                 "110 RETURN\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(100);
  CHECK(debugger.Continue().line == 100);

  const std::vector<newbasic::Vm::CallFrame> frames = s.vm.CallStack();
  REQUIRE(frames.size() == 2);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::Gosub);
  CHECK(frames[0].line == 50); // returns to the statement after the GOSUB
  CHECK(frames[1].kind == newbasic::Vm::CallFrame::Kind::For_);
  CHECK(frames[1].name == "I!"); // canonical name keeps the type suffix
}

TEST_CASE("Inspection: RETURN discards loops opened by the subroutine", "[debug]") {
  DebugSession s("10 FOR I=1 TO 2\n"
                 "20 GOSUB 100\n"
                 "30 NEXT I\n"
                 "40 END\n"
                 "100 FOR J=1 TO 10\n"
                 "110 RETURN\n"
                 "120 NEXT J\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(30);
  CHECK(debugger.Continue().line == 30);

  // The caller's I loop survives, but the subroutine's abandoned J loop
  // must not leak into the caller.
  std::vector<newbasic::Vm::CallFrame> frames = s.vm.CallStack();
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::For_);
  CHECK(frames[0].name == "I!");

  // The caller still runs its second iteration.
  CHECK(debugger.Continue().line == 30);
  CHECK(debugger.Variable("I")->AsNumber() == 2.0);
  frames = s.vm.CallStack();
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].name == "I!");
  CHECK(debugger.Continue().AtEnd());
}

TEST_CASE("Inspection: RETURN unwinds all loops opened by the subroutine", "[debug]") {
  DebugSession s("10 FOR I=1 TO 2\n"
                 "20 GOSUB 100\n"
                 "30 NEXT I\n"
                 "40 END\n"
                 "100 FOR J=1 TO 2\n"
                 "110 FOR K=1 TO 2\n"
                 "120 RETURN\n"
                 "130 NEXT K\n"
                 "140 NEXT J\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(30);
  CHECK(debugger.Continue().line == 30);

  const std::vector<newbasic::Vm::CallFrame> frames = s.vm.CallStack();
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::For_);
  CHECK(frames[0].name == "I!");
}

TEST_CASE("Inspection: RETURN preserves a caller loop around the call", "[debug]") {
  DebugSession s("10 FOR I=1 TO 2\n"
                 "20 GOSUB 100\n"
                 "30 NEXT I\n"
                 "40 END\n"
                 "100 RETURN\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(30);
  CHECK(debugger.Continue().line == 30);

  const std::vector<newbasic::Vm::CallFrame> frames = s.vm.CallStack();
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::For_);
  CHECK(frames[0].name == "I!");
}

TEST_CASE("Inspection: array contents and bounds", "[debug]") {
  DebugSession s("10 DIM A(3)\n20 A(2)=7\n30 DIM S$(1)\n40 S$(0)=\"hi\"\n50 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(50);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);

  const std::vector<newbasic::Vm::ArrayInfo> arrays = s.vm.Arrays();
  REQUIRE(arrays.size() == 2);
  CHECK(arrays[0].name == "A!");
  CHECK(arrays[0].kind == newbasic::Value::Kind::Sgl);
  CHECK(arrays[0].base == 0);
  CHECK(arrays[0].dims == std::vector<int>{3});
  CHECK(arrays[0].elements == 4); // OPTION BASE 0: indices 0..3
  CHECK(arrays[1].name == "S$");

  CHECK(s.vm.ArrayElement("A", {2})->AsNumber() == 7.0);
  CHECK(s.vm.ArrayElement("A", {0})->AsNumber() == 0.0);
  CHECK(s.vm.ArrayElement("S$", {0})->AsString() == "hi");
  // Out of range, wrong subscript count, and unknown arrays report nothing.
  CHECK_FALSE(s.vm.ArrayElement("A", {4}).has_value());
  CHECK_FALSE(s.vm.ArrayElement("A", {-1}).has_value());
  CHECK_FALSE(s.vm.ArrayElement("A", {1, 1}).has_value());
  CHECK_FALSE(s.vm.ArrayElement("NOPE", {0}).has_value());
}

TEST_CASE("Snapshots: restoring replays the same continuation", "[debug][snapshot]") {
  DebugSession s("10 A=1\n20 A=A+2\n30 A=A*3\n40 PRINT A\n50 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  CHECK(debugger.StepLine().line == 20); // line 10 has run, A == 1
  const newbasic::Vm::Snapshot snapshot = s.vm.SaveState();
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);

  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
  const double first = debugger.Variable("A")->AsNumber();
  CHECK(first == 9.0); // (1 + 2) * 3

  // Rewind and run again: identical state and result.
  s.vm.RestoreState(snapshot);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);
  CHECK(debugger.CurrentLine() == 20);
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == first);
}

TEST_CASE("Snapshots: capture loops, arrays and DATA position", "[debug][snapshot]") {
  DebugSession s("10 DIM A(3)\n"
                 "20 DATA 5,6,7\n"
                 "30 FOR I=1 TO 3\n"
                 "40 READ X: A(I)=X\n"
                 "50 NEXT I\n"
                 "60 PRINT A(1);A(3)\n"
                 "70 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(50); // fires before each NEXT
  CHECK(debugger.Continue().line == 50);
  const newbasic::Vm::Snapshot snapshot = s.vm.SaveState();
  // The first iteration has run; the rest have not.
  CHECK(s.vm.ArrayElement("A", {1})->AsNumber() == 5.0);
  CHECK(s.vm.ArrayElement("A", {2})->AsNumber() == 0.0);
  const std::vector<newbasic::Vm::CallFrame> frames = s.vm.CallStack();
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::For_);

  // Run to completion (a stop per remaining iteration, then the end).
  newbasic::debug::StopState state = debugger.Continue();
  while (!state.AtEnd())
    state = debugger.Continue();
  CHECK(s.vm.ArrayElement("A", {3})->AsNumber() == 7.0);

  // Restoring brings back the loop frame, the array contents, the DATA
  // cursor and the instruction pointer together, so the run replays.
  s.vm.RestoreState(snapshot);
  const std::vector<newbasic::Vm::CallFrame> restored = s.vm.CallStack();
  REQUIRE(restored.size() == 1);
  CHECK(restored[0].name == frames[0].name);
  CHECK(s.vm.ArrayElement("A", {2})->AsNumber() == 0.0);
  CHECK(s.vm.ArrayElement("A", {3})->AsNumber() == 0.0); // rewound with the VM
  state = debugger.Continue();
  while (!state.AtEnd())
    state = debugger.Continue();
  CHECK(s.vm.ArrayElement("A", {3})->AsNumber() == 7.0);
}

TEST_CASE("Debugger: surfaces frames, arrays and snapshots", "[debug]") {
  DebugSession s("10 DIM A(2)\n"
                 "20 A(1)=4\n"
                 "30 FOR I=1 TO 2\n"
                 "40 GOSUB 100\n"
                 "50 NEXT I\n"
                 "60 END\n"
                 "100 B=1\n"
                 "110 RETURN\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(100);
  CHECK(debugger.Continue().line == 100);

  // Structural inspection through the debugger itself (the DAP surface).
  const std::vector<newbasic::Vm::CallFrame> frames = debugger.CallFrames();
  REQUIRE(frames.size() == 2);
  CHECK(frames[0].kind == newbasic::Vm::CallFrame::Kind::Gosub);
  CHECK(frames[1].kind == newbasic::Vm::CallFrame::Kind::For_);
  const std::vector<newbasic::Vm::ArrayInfo> arrays = debugger.Arrays();
  REQUIRE(arrays.size() == 1);
  CHECK(arrays[0].name == "A!");
  CHECK(arrays[0].elements == 3);
  CHECK(debugger.ArrayElement("A", {1})->AsNumber() == 4.0);
  CHECK_FALSE(debugger.ArrayElement("A", {9}).has_value());

  // Snapshots are reachable through the debugger as well.
  const newbasic::Vm::Snapshot snapshot = debugger.SaveState();
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::Breakpoint);
  debugger.RestoreState(snapshot);
  CHECK(debugger.CurrentLine() == 100);
  CHECK(debugger.CallFrames().size() == 2);
}

TEST_CASE("Debugger: conditional breakpoints", "[debug]") {
  DebugSession s("10 FOR I=1 TO 5\n20 A=A+1\n30 NEXT I\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(20);
  debugger.SetCondition(20, "I=3");
  CHECK(debugger.HasCondition(20));
  CHECK(debugger.Condition(20) == "I=3");
  CHECK(debugger.Condition(99).empty());

  // Only the third iteration stops (the first two pass the condition false).
  const newbasic::debug::StopState state = debugger.Continue();
  CHECK(state.reason == newbasic::debug::StopReason::Breakpoint);
  CHECK(state.line == 20);
  CHECK(debugger.Variable("I")->AsNumber() == 3.0);
  // Evaluating the condition did not disturb the program.
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);
  CHECK_FALSE(debugger.Variable("ZZCOND").has_value());

  // A condition that never holds runs to the end.
  debugger.SetCondition(20, "I=99");
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == 5.0);
}

TEST_CASE("Debugger: a malformed condition never breaks", "[debug]") {
  DebugSession s("10 A=1\n20 A=2\n30 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetBreakpoint(20);
  debugger.SetCondition(20, "1 +");
  CHECK(debugger.Continue().reason == newbasic::debug::StopReason::ProgramEnd);
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);
}

TEST_CASE("Time travel: StepBack restores the previous line and its variables", "[debug][travel]") {
  DebugSession s("10 A=1\n20 A=A+1\n30 A=A+1\n40 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  CHECK(debugger.StepLine().line == 20); // line 10 has run: A == 1
  CHECK(debugger.StepLine().line == 30); // line 20 has run: A == 2
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);

  const newbasic::debug::StopState back = debugger.StepBack();
  CHECK(back.reason == newbasic::debug::StopReason::Step);
  CHECK(debugger.CurrentLine() == 20);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0); // the value *before* line 20

  // Replaying forward reproduces the same values, so rewinding is lossless.
  CHECK(debugger.StepLine().line == 30);
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);

  // Undo everything; an empty history reports None rather than inventing a stop.
  while (debugger.HistorySize() > 0)
    debugger.StepBack();
  CHECK(debugger.StepBack().reason == newbasic::debug::StopReason::None);
}

TEST_CASE("Time travel: ReverseContinue stops at the previous line", "[debug][travel]") {
  DebugSession s("10 A=1\n20 A=2\n30 A=3\n40 A=4\n50 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  CHECK(debugger.StepLine().line == 20);
  CHECK(debugger.StepLine().line == 30);
  CHECK(debugger.StepLine().line == 40);
  CHECK(debugger.Variable("A")->AsNumber() == 3.0);

  const newbasic::debug::StopState back = debugger.ReverseContinue();
  CHECK(back.line == 30);
  CHECK(debugger.Variable("A")->AsNumber() == 2.0);
  CHECK(debugger.ReverseContinue().line == 20);
  CHECK(debugger.Variable("A")->AsNumber() == 1.0);
}

TEST_CASE("Time travel: the history ring is bounded and drops the oldest", "[debug][travel]") {
  DebugSession s("10 A=1\n20 A=2\n30 A=3\n40 A=4\n50 END\n");
  newbasic::debug::Debugger debugger(s.vm);
  debugger.SetHistoryLimit(2);
  CHECK(debugger.HistoryLimit() == 2);
  for (int i = 0; i < 3; ++i)
    debugger.StepLine();
  CHECK(debugger.HistorySize() <= 2);

  // Only the retained entries can be undone.
  CHECK(debugger.StepBack().reason == newbasic::debug::StopReason::Step);
  CHECK(debugger.StepBack().reason == newbasic::debug::StopReason::Step);
  CHECK(debugger.StepBack().reason == newbasic::debug::StopReason::None);

  debugger.ClearHistory();
  CHECK(debugger.HistorySize() == 0);
}
