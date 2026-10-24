// CONST: a read-only global initialized before the program starts.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

TEST_CASE("CONST: numeric and string values are readable", "[const]") {
  CHECK(lwtest::Row0("10 CONST MAX% = 100\n20 PRINT MAX\n") == " 100");
  CHECK(lwtest::Row0("10 CONST BASE& = 40000\n20 PRINT BASE&\n") == " 40000");
  CHECK(lwtest::Row0("10 CONST TITLE$ = \"ADA\"\n20 PRINT TITLE$\n") == "ADA");
  CHECK(lwtest::Row0("10 CONST TITLE$ = \"ADA\"\n20 PRINT TITLE\n") == "ADA");
  CHECK(lwtest::Row0("10 CONST BIG& = 40000\n20 PRINT BIG\n") == " 40000");
  CHECK(lwtest::Row0("10 CONST PI = 3.5\n20 PRINT PI\n") == " 3.5");
  CHECK(lwtest::Row0("10 CONST NEG% = -7\n20 PRINT NEG\n") == "-7");
}

TEST_CASE("CONST: values exist before the declaration line", "[const]") {
  CHECK(lwtest::Row0("10 PRINT ANSWER\n20 CONST ANSWER% = 42\n") == " 42");
}

TEST_CASE("CONST: a shared line works", "[const]") {
  CHECK(lwtest::Row0("10 CONST A% = 1: PRINT A\n") == " 1");
}

TEST_CASE("CONST: DEFINT does not change an explicit constant", "[const]") {
  CHECK(lwtest::Row0("10 DEFINT A\n20 CONST A = 40000\n30 PRINT A\n") == " 40000");
}

TEST_CASE("CONST: duplicate declarations are rejected", "[const]") {
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 CONST A% = 2\n") ==
        newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 CONST A = 1\n20 CONST A$ = \"x\"\n") ==
        newbasic::kErrSyntax);
}

TEST_CASE("CONST: assignment and other writes are rejected", "[const]") {
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 A% = 2\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 A = 2\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 FOR A = 1 TO 2\n30 NEXT\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 INPUT A\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 READ A\n30 DATA 2\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 DIM A(2)\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 B = 2\n30 SWAP A, B\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 STACK POP A\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 CONST A% = 1\n20 STACK TOP A\n") ==
        newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("CONST: value errors are compile-time", "[const]") {
  CHECK(lwtest::CompileCode("10 CONST A$ = 1\n") == newbasic::kErrTypeMismatch);
  CHECK(lwtest::CompileCode("10 CONST A% = \"x\"\n") == newbasic::kErrTypeMismatch);
  CHECK(lwtest::CompileCode("10 CONST A% = 2147483648\n") == newbasic::kErrOverflow);
  CHECK(lwtest::CompileCode("10 CONST A% = 1 + 2\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 CONST A% = 2147483647\n") == 0);
}

TEST_CASE("CONST: direct mode rejects CONST", "[const]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  const newbasic::RunResult result = repl.Process("CONST A = 1");
  CHECK(result.status == newbasic::RunResult::Status::Error);
  CHECK(result.code == newbasic::kErrIllegalDirect);
}

TEST_CASE("CONST: CLEAR reinitializes the constant", "[const]") {
  CHECK(lwtest::Row0("10 CONST A% = 7\n20 CLEAR\n30 PRINT A\n") == " 7");
}
