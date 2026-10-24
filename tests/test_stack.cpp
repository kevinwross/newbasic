// STACK: user-visible LIFO stack. The list forms are atomic: a failed POP/TOP
// leaves both the stack and every target unchanged.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

using newbasic::screen::HeadlessScreen;

TEST_CASE("STACK: STACK is a reserved word", "[stack][lexer]") {
  CHECK(newbasic::Lexer::IsKeyword("STACK"));
}

TEST_CASE("STACK: PUSH copies the value, not the variable", "[stack]") {
  CHECK(lwtest::Row0("10 A = 1\n"
                     "20 STACK PUSH A\n"
                     "30 A = 2\n"
                     "40 STACK POP B\n"
                     "50 IF B = 1 AND A = 2 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: PUSH and POP use list order", "[stack]") {
  CHECK(lwtest::Row0("10 STACK PUSH 1, 2, 3\n"
                     "20 STACK POP C, B, A\n"
                     "30 IF A = 1 AND B = 2 AND C = 3 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: TOP is non-destructive and uses the same target order", "[stack]") {
  CHECK(lwtest::Row0("10 STACK PUSH 1, 2, 3\n"
                     "20 STACK TOP C, B, A\n"
                     "30 IF A = 1 AND B = 2 AND C = 3 THEN PRINT \"T\";\n"
                     "40 STACK POP C, B, A\n"
                     "50 IF A = 1 AND B = 2 AND C = 3 THEN PRINT \"P\"\n") == "TP");
}

TEST_CASE("STACK: mixed types convert on POP", "[stack]") {
  CHECK(lwtest::Row0("10 STACK PUSH \"hi\", 2.7\n"
                     "20 STACK POP A%, A$\n"
                     "30 IF A% = 3 AND A$ = \"hi\" THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: array targets and auto-dimensioning", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 STACK PUSH 7, 8\n"
                     "30 STACK POP A(2), A(1)\n"
                     "40 IF A(2) = 8 AND A(1) = 7 THEN PRINT \"A\";\n"
                     "50 STACK PUSH 9\n"
                     "60 STACK POP B(2)\n"
                     "70 IF B(2) = 9 THEN PRINT \"B\"\n") == "AB");
}

TEST_CASE("STACK: OPTION BASE applies to an auto-created array", "[stack]") {
  CHECK(lwtest::Row0("10 OPTION BASE 1\n"
                     "20 STACK PUSH 5\n"
                     "30 STACK POP A(1)\n"
                     "40 IF A(1) = 5 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: a failed conversion leaves the stack intact", "[stack]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 100\n"
                     "20 STACK PUSH 1, \"x\"\n"
                     "30 STACK POP A%, B\n"
                     "40 PRINT \"BAD\"\n"
                     "50 END\n"
                     "100 STACK POP S$, N%\n"
                     "110 IF S$ = \"x\" AND N% = 1 THEN PRINT \"OK\"\n"
                     "120 END\n") == "OK");
}

TEST_CASE("STACK: a failed array subscript leaves the stack intact", "[stack]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 100\n"
                     "20 DIM A(2)\n"
                     "30 STACK PUSH 7, 8\n"
                     "40 STACK POP A(3), B\n"
                     "50 END\n"
                     "100 STACK POP B, C\n"
                     "110 IF B = 8 AND C = 7 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: a later conversion failure does not commit earlier targets", "[stack]") {
  CHECK(lwtest::Row0("10 A = -1\n"
                     "20 ON ERROR GOTO 100\n"
                     "30 STACK PUSH \"x\", 5\n"
                     "40 STACK POP A, B%\n"
                     "50 END\n"
                     "100 IF A = -1 THEN PRINT \"U\";\n"
                     "110 STACK POP N%, S$\n"
                     "120 IF N% = 5 AND S$ = \"x\" THEN PRINT \"OK\"\n") == "UOK");
}

TEST_CASE("STACK: a failed list removes arrays it auto-created", "[stack]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 100\n"
                     "20 STACK PUSH \"x\", 7\n"
                     "30 STACK POP A(1), B%\n"
                     "40 END\n"
                     "100 DIM A(2)\n"
                     "110 PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: PEEK is an alias for TOP", "[stack]") {
  CHECK(lwtest::Row0("10 STACK PUSH 1, 2\n"
                     "20 STACK PEEK B, A\n"
                     "30 IF A = 1 AND B = 2 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: an underflowing list leaves the stack intact", "[stack]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 100\n"
                     "20 STACK PUSH 1\n"
                     "30 STACK POP A, B\n"
                     "40 END\n"
                     "100 STACK POP C\n"
                     "110 IF C = 1 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: whole arrays copy by value", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 A(1) = 101 : A(2) = 102\n"
                     "30 STACK PUSH A()\n"
                     "40 A(1) = 999\n"
                     "50 STACK POP B()\n"
                     "60 IF B(1) = 101 AND B(2) = 102 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: TOP copies a whole array without removing it", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(1)\n"
                     "20 A(1) = 7\n"
                     "30 STACK PUSH A()\n"
                     "40 STACK TOP B()\n"
                     "50 STACK POP C()\n"
                     "60 IF B(1) = 7 AND C(1) = 7 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: whole arrays can mix with scalars in a list", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 A(1) = 7 : A(2) = 8\n"
                     "30 STACK PUSH 5, A(), \"x\"\n"
                     "40 STACK POP S$, B(), N%\n"
                     "50 IF S$ = \"x\" AND N% = 5 AND B(1) = 7 AND B(2) = 8 THEN PRINT \"OK\"\n") ==
        "OK");
}

TEST_CASE("STACK: whole arrays convert to the target element type", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 A(1) = 2.7 : A(2) = 3.2\n"
                     "30 STACK PUSH A()\n"
                     "40 STACK POP B%()\n"
                     "50 IF B%(1) = 3 AND B%(2) = 3 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: a whole-array conversion failure leaves the stack intact", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 A(1) = 7 : A(2) = 8\n"
                     "30 ON ERROR GOTO 100\n"
                     "40 STACK PUSH A()\n"
                     "50 STACK POP B$()\n"
                     "60 END\n"
                     "100 STACK POP C()\n"
                     "110 IF C(1) = 7 AND C(2) = 8 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: string arrays copy by value", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A$(1)\n"
                     "20 A$(1) = \"hello\"\n"
                     "30 STACK PUSH A$()\n"
                     "40 A$(1) = \"changed\"\n"
                     "50 STACK POP B$()\n"
                     "60 IF B$(1) = \"hello\" THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: whole arrays preserve dimensions and OPTION BASE", "[stack]") {
  CHECK(lwtest::Row0("10 OPTION BASE 1\n"
                     "20 DIM A(2,2)\n"
                     "30 A(1,2) = 7\n"
                     "40 STACK PUSH A()\n"
                     "50 STACK POP B()\n"
                     "60 IF B(1,2) = 7 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: a whole-array target needs an array source", "[stack]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 100\n"
                     "20 STACK PUSH 5\n"
                     "30 STACK POP B()\n"
                     "40 END\n"
                     "100 STACK POP C\n"
                     "110 IF C = 5 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: a scalar target cannot receive a whole array", "[stack]") {
  CHECK(lwtest::Row0("10 DIM A(2)\n"
                     "20 A(1) = 7\n"
                     "30 ON ERROR GOTO 100\n"
                     "40 STACK PUSH A()\n"
                     "50 STACK POP B\n"
                     "60 END\n"
                     "100 STACK POP C()\n"
                     "110 IF C(1) = 7 THEN PRINT \"OK\"\n") == "OK");
}

TEST_CASE("STACK: PUSH A() needs an existing array", "[stack]") {
  CHECK(lwtest::RunErrorCode("10 STACK PUSH A()\n") == newbasic::kErrSubscriptOutOfRange);
}

TEST_CASE("STACK: empty stack raises Stack empty", "[stack]") {
  CHECK(lwtest::RunErrorCode("10 STACK POP A\n") == newbasic::kErrStackEmpty);
  CHECK(lwtest::RunErrorCode("10 STACK TOP A\n") == newbasic::kErrStackEmpty);
  CHECK(lwtest::RunErrorCode("10 STACK POP A, B\n") == newbasic::kErrStackEmpty);
}

TEST_CASE("STACK: syntax errors", "[stack]") {
  CHECK(lwtest::CompileCode("10 STACK PUSH\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 STACK POP\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 STACK POP 1\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 STACK WAT 1\n") == newbasic::kErrSyntax);
}

TEST_CASE("STACK: direct mode persists until CLEAR", "[stack]") {
  HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  CHECK(repl.Process("STACK PUSH 1, 2").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Process("STACK POP A, B").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Process("PRINT A;B").status == newbasic::RunResult::Status::Done);
  CHECK(screen.RenderRows()[0] == " 2  1");

  CHECK(repl.Process("STACK PUSH 5").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Process("CLEAR").status == newbasic::RunResult::Status::Done);
  const newbasic::RunResult empty = repl.Process("STACK POP A");
  CHECK(empty.status == newbasic::RunResult::Status::Error);
  CHECK(empty.code == newbasic::kErrStackEmpty);

  CHECK(repl.Process("10 END").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Process("STACK PUSH 5").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  const newbasic::RunResult run_empty = repl.Process("STACK POP A");
  CHECK(run_empty.status == newbasic::RunResult::Status::Error);
  CHECK(run_empty.code == newbasic::kErrStackEmpty);
}

TEST_CASE("STACK: a VM snapshot includes the user stack", "[stack]") {
  HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Vm vm(screen, &host);
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource("10 STACK PUSH 1, 2\n20 END\n", &program, &error));
  vm.Load(std::move(program));
  REQUIRE(vm.Run().status == newbasic::RunResult::Status::Done);

  const newbasic::Vm::Snapshot snapshot = vm.SaveState();
  vm.ClearState();
  CHECK(vm.UserStack().empty());
  vm.RestoreState(snapshot);
  REQUIRE(vm.UserStack().size() == 2);
  CHECK(vm.UserStack()[0].scalar.AsNumber() == 1.0);
  CHECK(vm.UserStack()[1].scalar.AsNumber() == 2.0);
}

TEST_CASE("STACK: a VM snapshot includes arrays on the user stack", "[stack]") {
  HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Vm vm(screen, &host);
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource("10 DIM A(2)\n"
                                "20 A(1) = 7 : A(2) = 8\n"
                                "30 STACK PUSH A()\n"
                                "40 END\n",
                                &program, &error));
  vm.Load(std::move(program));
  REQUIRE(vm.Run().status == newbasic::RunResult::Status::Done);

  const newbasic::Vm::Snapshot snapshot = vm.SaveState();
  vm.ClearState();
  CHECK(vm.UserStack().empty());
  vm.RestoreState(snapshot);
  REQUIRE(vm.UserStack().size() == 1);
  REQUIRE(vm.UserStack()[0].is_array);
  REQUIRE(vm.UserStack()[0].array.data.size() == 3);
  CHECK(vm.UserStack()[0].array.data[1].AsNumber() == 7.0);
  CHECK(vm.UserStack()[0].array.data[2].AsNumber() == 8.0);
}
