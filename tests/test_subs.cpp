// SUB/FUNCTION, LOCAL variables, and the procedure calling convention.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/debugger.h"
#include "newbasic/errors.h"
#include "newbasic/frontend.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"
#include "test_helpers.h"

using lwtest::CompileCode;
using lwtest::Row0;
using lwtest::RunErrorCode;
using lwtest::RunRows;

TEST_CASE("SUB: parameters, locals, and caller state", "[subs]") {
  const std::vector<std::string> rows = RunRows(
      "10 X% = 5\n"
      "20 CALL Show(X%)\n"
      "30 PRINT X%\n"
      "40 END\n"
      "100 SUB Show(N%)\n"
      "110 LOCAL T%\n"
      "120 T% = N% + 1\n"
      "130 PRINT T%\n"
      "140 END SUB\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 6");
  CHECK(rows[1] == " 5");
}

TEST_CASE("SUB: forward CALL and string parameters", "[subs]") {
  const std::vector<std::string> rows = RunRows(
      "10 CALL Greet(\"Bob\")\n"
      "20 END\n"
      "100 SUB Greet(S$)\n"
      "110 LOCAL T$\n"
      "120 T$ = \"Hi \" + S$\n"
      "130 PRINT T$\n"
      "140 END SUB\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "Hi Bob");
}

TEST_CASE("FUNCTION: forward call, result, and default result", "[subs]") {
  CHECK(Row0("10 PRINT Twice%(4)\n"
             "20 END\n"
             "100 FUNCTION Twice%(N%)\n"
             "110 Twice% = N% * 2\n"
             "120 END FUNCTION\n") == " 8");
  CHECK(Row0("10 PRINT Empty%()\n"
             "20 END\n"
             "100 FUNCTION Empty%()\n"
             "110 END FUNCTION\n") == " 0");
  CHECK(Row0("10 PRINT Join$(\"a\", \"b\")\n"
             "20 END\n"
             "100 FUNCTION Join$(A$, B$)\n"
             "110 Join$ = A$ + B$\n"
             "120 END FUNCTION\n") == "ab");
}

TEST_CASE("SUB/FUNCTION: recursion gets fresh locals", "[subs]") {
  CHECK(Row0("10 PRINT Fact%(5)\n"
             "20 END\n"
             "100 FUNCTION Fact%(N%)\n"
             "110 IF N% <= 1 THEN Fact% = 1 : EXIT FUNCTION\n"
             "120 Fact% = N% * Fact%(N% - 1)\n"
             "130 END FUNCTION\n") == " 120");
}

TEST_CASE("SUB: callee cannot see caller locals", "[subs]") {
  const std::vector<std::string> rows = RunRows(
      "10 CALL A\n"
      "20 END\n"
      "100 SUB A\n"
      "110 LOCAL X%\n"
      "120 X% = 42\n"
      "130 CALL B\n"
      "140 PRINT X%\n"
      "150 END SUB\n"
      "200 SUB B\n"
      "210 PRINT X%\n"
      "220 END SUB\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 0");  // B sees the global X%, not A's local
  CHECK(rows[1] == " 42"); // A still sees its own local
}

TEST_CASE("SUB: GOSUB helper inside the same procedure sees locals", "[subs]") {
  const std::vector<std::string> rows = RunRows(
      "10 CALL Foo\n"
      "20 END\n"
      "100 SUB Foo\n"
      "110 LOCAL X%\n"
      "120 X% = 7\n"
      "130 GOSUB 160\n"
      "140 PRINT X%\n"
      "150 EXIT SUB\n"
      "160 PRINT X%\n"
      "170 X% = X% + 1\n"
      "180 RETURN\n"
      "190 END SUB\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 7");
  CHECK(rows[1] == " 8");
}

TEST_CASE("SUB/FUNCTION: EXIT and FOR cleanup", "[subs]") {
  const std::vector<std::string> rows = RunRows(
      "10 CALL Early(0)\n"
      "20 CALL Early(1)\n"
      "30 CALL LoopTest\n"
      "40 END\n"
      "100 SUB Early(Flag%)\n"
      "110 IF Flag% = 0 THEN PRINT \"SKIP\" : EXIT SUB\n"
      "120 PRINT \"RUN\"\n"
      "130 END SUB\n"
      "200 SUB LoopTest\n"
      "210 LOCAL I%, T%\n"
      "220 FOR I% = 1 TO 3\n"
      "230 T% = T% + I%\n"
      "240 NEXT I%\n"
      "250 PRINT T%\n"
      "260 END SUB\n");
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == "SKIP");
  CHECK(rows[1] == "RUN");
  CHECK(rows[2] == " 6");
}

TEST_CASE("SUB: parameter type conversion", "[subs]") {
  CHECK(Row0("10 CALL Conv(3.6)\n"
             "20 END\n"
             "100 SUB Conv(N%)\n"
             "110 PRINT N%\n"
             "120 END SUB\n") == " 4");
  CHECK(RunErrorCode("10 CALL Conv(\"x\")\n"
                     "20 END\n"
                     "100 SUB Conv(N%)\n"
                     "110 END SUB\n") == newbasic::kErrTypeMismatch);
}

TEST_CASE("SUB/FUNCTION: definition and name errors", "[subs][errors]") {
  CHECK(CompileCode("10 SUB Foo\n20 PRINT 1\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 SUB Bar\n30 END SUB\n40 END SUB\n") ==
        newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 END SUB\n30 SUB Foo\n40 END SUB\n") ==
        newbasic::kErrDuplicateDefinition);
  CHECK(CompileCode("10 SUB Foo\n20 SUB\n30 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 LOCAL\n30 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 LOCAL X\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 PRINT 1\n30 LOCAL X\n40 END SUB\n") ==
        newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo(A%, A%)\n20 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo%\n20 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 FUNCTION Foo\n20 END FUNCTION\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 CALL Missing\n") == newbasic::kErrUndefinedProcedure);
  CHECK(CompileCode("10 FUNCTION Foo%()\n20 END FUNCTION\n"
                    "30 CALL Foo%()\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 END SUB\n30 Foo = 1\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 DIM Foo%(2)\n30 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 DEF FNX(X)=X\n30 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 CLEAR\n30 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 CALL Foo(1)\n30 SUB Foo\n40 END SUB\n") == newbasic::kErrSyntax);
}

TEST_CASE("SUB/FUNCTION: procedure boundaries", "[subs][errors]") {
  CHECK(CompileCode("10 GOTO 110\n20 END\n100 SUB Foo\n110 PRINT 1\n120 END SUB\n") ==
        newbasic::kErrProcedureBoundary);
  CHECK(CompileCode("10 GOSUB 110\n20 END\n100 SUB Foo\n110 RETURN\n120 END SUB\n") ==
        newbasic::kErrProcedureBoundary);
  CHECK(CompileCode("10 SUB Foo\n20 GOTO 100\n30 END SUB\n100 PRINT 1\n110 END\n") ==
        newbasic::kErrProcedureBoundary);
  CHECK(CompileCode("10 SUB Foo\n20 GOTO 100\n30 END SUB\n"
                    "100 SUB Bar\n110 PRINT 1\n120 END SUB\n") ==
        newbasic::kErrProcedureBoundary);
  CHECK(CompileCode("10 SUB Foo\n20 ON ERROR GOTO 30\n"
                    "30 RESUME NEXT\n40 END SUB\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 SUB Foo\n20 ON TIMER(1) GOSUB 30\n"
                    "30 RETURN\n40 END SUB\n") == newbasic::kErrSyntax);
}

TEST_CASE("SUB: RETURN and open-block guards", "[subs][errors]") {
  CHECK(RunErrorCode("10 CALL Foo\n20 END\n"
                     "100 SUB Foo\n110 RETURN\n120 END SUB\n") ==
        newbasic::kErrReturnWithoutGosub);
  CHECK(RunErrorCode("10 CALL Foo\n20 END\n"
                     "100 SUB Foo\n110 SELECT CASE 1\n120 CASE 1\n"
                     "130 EXIT SUB\n140 END SELECT\n150 END SUB\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(RunErrorCode("10 CALL Foo\n20 END\n"
                     "100 SUB Foo\n110 GOSUB 140\n120 EXIT SUB\n130 REM\n"
                     "140 EXIT SUB\n150 RETURN\n160 END SUB\n") ==
        newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("SUB: call depth is bounded", "[subs][errors]") {
  CHECK(RunErrorCode("10 CALL Foo\n20 END\n"
                     "100 SUB Foo\n110 CALL Foo\n120 END SUB\n") ==
        newbasic::kErrCallStackOverflow);
}

TEST_CASE("SUB: ON ERROR in main resumes inside the procedure", "[subs][errors]") {
  const std::vector<std::string> rows = RunRows(
      "10 ON ERROR GOTO 500\n"
      "20 CALL Foo\n"
      "30 PRINT G%\n"
      "40 END\n"
      "100 SUB Foo\n"
      "110 LOCAL X%\n"
      "120 X% = 1\n"
      "130 ERROR 5\n"
      "140 X% = 2\n"
      "150 G% = X%\n"
      "160 END SUB\n"
      "500 PRINT ERR; ERL\n"
      "510 RESUME NEXT\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 5  130");
  CHECK(rows[1] == " 2");
}

TEST_CASE("SUB: call depth, locals, and stepping are visible to the debugger", "[subs][debug]") {
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource(
      "10 X% = 1\n"
      "20 CALL Foo(X%)\n"
      "30 PRINT X%\n"
      "40 END\n"
      "100 SUB Foo(N%)\n"
      "110 LOCAL Y%\n"
      "120 Y% = N% + 1\n"
      "130 PRINT Y%\n"
      "140 END SUB\n",
      &program, &error));
  newbasic::screen::HeadlessScreen screen;
  newbasic::Vm vm(screen);
  vm.Load(std::move(program));
  newbasic::debug::Debugger debugger(vm);
  debugger.SetBreakpoint(130);

  const newbasic::debug::StopState at_body = debugger.Continue();
  REQUIRE(at_body.line == 130);
  const std::vector<newbasic::Vm::CallFrame> frames = debugger.CallFrames();
  REQUIRE(!frames.empty());
  CHECK(frames.back().kind == newbasic::Vm::CallFrame::Kind::Sub);
  CHECK(frames.back().name == "FOO");
  const std::vector<std::pair<std::string, newbasic::Value>> locals =
      debugger.LocalVariables();
  bool found_y = false;
  for (const auto& entry : locals) {
    if (entry.first == "Y%") {
      found_y = true;
      CHECK(entry.second.AsNumber() == 2.0);
    }
  }
  CHECK(found_y);

  const newbasic::debug::StopState out = debugger.StepOut();
  CHECK(out.line == 30);
}

TEST_CASE("SUB: debugger Step Over skips a CALL and snapshots keep locals", "[subs][debug]") {
  const std::string source =
      "10 X% = 1\n"
      "20 CALL Foo(X%)\n"
      "30 PRINT X%\n"
      "40 END\n"
      "100 SUB Foo(N%)\n"
      "110 LOCAL Y%\n"
      "120 Y% = N% + 1\n"
      "130 PRINT Y%\n"
      "140 END SUB\n";

  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource(source, &program, &error));
  newbasic::screen::HeadlessScreen screen;
  newbasic::Vm vm(screen);
  vm.Load(std::move(program));
  newbasic::debug::Debugger over(vm);
  over.SetBreakpoint(20);
  CHECK(over.Continue().line == 20);
  const newbasic::debug::StopState after_over = over.StepOver();
  CHECK(after_over.line == 30);
  CHECK(over.CallFrames().empty());

  newbasic::Program program2;
  REQUIRE(lwtest::CompileSource(source, &program2, &error));
  newbasic::Vm vm2(screen);
  vm2.Load(std::move(program2));
  newbasic::debug::Debugger snap(vm2);
  snap.SetBreakpoint(130);
  CHECK(snap.Continue().line == 130);
  REQUIRE(snap.LocalVariable("Y%").has_value());
  CHECK(snap.LocalVariable("Y%")->AsNumber() == 2.0);
  CHECK(snap.StepLine().line == 140);
  const newbasic::debug::StopState back = snap.StepBack();
  CHECK(back.line == 130);
  REQUIRE(snap.LocalVariable("Y%").has_value());
  CHECK(snap.LocalVariable("Y%")->AsNumber() == 2.0);
  CHECK(snap.CallFrames().size() == 1);
}
