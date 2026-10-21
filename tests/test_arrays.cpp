// M2: arrays (DIM/OPTION BASE/ERASE) and DEF* default types.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

using lwtest::Row0;
using lwtest::RunErrorCode;

TEST_CASE("Arrays: DIM, store and load", "[arrays]") {
  CHECK(Row0("10 DIM A(5)\n20 A(0)=1: A(5)=2\n30 PRINT A(0);A(5)\n") == " 1  2");
  // The default upper bound is 10 per dimension.
  CHECK(Row0("10 A(10)=7: PRINT A(10)\n") == " 7");
  // Elements start at zero / empty.
  CHECK(Row0("10 DIM A(3): PRINT A(2)\n") == " 0");
  CHECK(Row0("10 DIM A$(3): PRINT \"[\";A$(2);\"]\"\n") == "[]");
  // Multi-dimensional arrays.
  CHECK(Row0("10 DIM M(2,3)\n20 M(1,2)=9\n30 PRINT M(1,2)\n") == " 9");
  // String arrays.
  CHECK(Row0("10 DIM S$(3): S$(1)=\"hi\": PRINT S$(1)\n") == "hi");
  // Element stores convert to the array's type.
  CHECK(Row0("10 DIM A%(2): A%(1)=3.7: PRINT A%(1)\n") == " 4");
  // Subscripts round to the nearest integer.
  CHECK(Row0("10 DIM A(5): A(2.4)=3: PRINT A(2)\n") == " 3");
  // An undimensioned array is created on first use (10 per dimension).
  CHECK(Row0("10 A(2,2)=5: PRINT A(2,2)\n") == " 5");
  // Arrays work inside expressions and loops.
  CHECK(Row0("10 DIM A(5)\n20 FOR I=1 TO 5: A(I)=I*I: NEXT I\n30 PRINT A(4)\n") == " 16");
  // The same array can be cleared and reused with ERASE.
  CHECK(Row0("10 DIM A(3): A(1)=5: ERASE A: A(1)=7: PRINT A(1)\n") == " 7");
}

TEST_CASE("Arrays: OPTION BASE", "[arrays]") {
  CHECK(Row0("10 OPTION BASE 1\n20 DIM A(5)\n30 A(1)=1: A(5)=2\n40 PRINT A(1);A(5)\n") == " 1  2");
  CHECK(RunErrorCode("10 OPTION BASE 1\n20 DIM A(5)\n30 A(0)=1\n") ==
        newbasic::kErrSubscriptOutOfRange);
  CHECK(lwtest::CompileCode("10 OPTION BASE 2\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 OPTION BASE\n") == newbasic::kErrSyntax);
}

TEST_CASE("Arrays: runtime errors", "[arrays][errors]") {
  CHECK(RunErrorCode("10 DIM A(5)\n20 A(6)=1\n") == newbasic::kErrSubscriptOutOfRange);
  CHECK(RunErrorCode("10 DIM A(5)\n20 A(-1)=1\n") == newbasic::kErrSubscriptOutOfRange);
  CHECK(RunErrorCode("10 DIM A(5)\n20 PRINT A(200)\n") == newbasic::kErrSubscriptOutOfRange);
  CHECK(RunErrorCode("10 DIM A(2,2)\n20 A(1)=1\n") == newbasic::kErrSubscriptOutOfRange);
  CHECK(RunErrorCode("10 DIM A(2)\n20 A(1,1)=1\n") == newbasic::kErrSubscriptOutOfRange);
  CHECK(RunErrorCode("10 DIM A(5)\n20 DIM A(5)\n") == newbasic::kErrDuplicateDefinition);
  CHECK(RunErrorCode("10 DIM A$(2)\n20 A$(1)=5\n") == newbasic::kErrTypeMismatch);
  CHECK(RunErrorCode("10 DIM A(2)\n20 A(1)=\"x\"\n") == newbasic::kErrTypeMismatch);
  int line = 0;
  CHECK(RunErrorCode("10 DIM A(2)\n20 PRINT A(5)\n", &line) == newbasic::kErrSubscriptOutOfRange);
  CHECK(line == 20);
}

TEST_CASE("Types: DEF* default types", "[types]") {
  CHECK(Row0("10 DEFINT A-Z: A=3.7: PRINT A\n") == " 4");
  CHECK(Row0("10 DEFSTR A: A=\"hi\": PRINT A\n") == "hi");
  CHECK(Row0("10 DEFDBL A: A=1/3: PRINT A\n") == " .3333333333333333");
  CHECK(Row0("10 DEFINT A-C\n20 A=1: D=1.5\n30 PRINT A;D\n") == " 1  1.5");
  // An explicit suffix beats the default.
  CHECK(Row0("10 DEFINT A: A$=\"x\": PRINT A$\n") == "x");
  CHECK(Row0("10 DEFSTR A: A%=2: PRINT A%\n") == " 2");
  // The default applies to arrays and FOR variables too.
  CHECK(Row0("10 DEFINT A\n20 DIM A(2)\n30 A(1)=2.6\n40 PRINT A(1)\n") == " 3");
  CHECK(Row0("10 DEFINT I: FOR I=1 TO 2: PRINT I: NEXT I\n") == " 1");
  CHECK(lwtest::CompileCode("10 DEFINT\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 DEFINT 1\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 DIM\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 DEF X=1\n") == newbasic::kErrSyntax);
}

TEST_CASE("Arrays: two dimensions and computed subscripts", "[arrays]") {
  CHECK(Row0("10 DIM A(2,3)\n20 A(1,2)=5\n30 IF A(1,2)=5 THEN PRINT \"OK\";\n") == "OK");
  // Subscript expressions are evaluated at the store.
  CHECK(Row0("10 DIM A(5)\n20 I=2\n30 A(I+1)=7\n40 IF A(3)=7 THEN PRINT \"OK\";\n") == "OK");
  // A string array holds strings.
  CHECK(Row0("10 DIM S$(2)\n20 S$(1)=\"hi\"\n30 IF S$(1)=\"hi\" THEN PRINT \"OK\";\n") == "OK");
  // A 2D element outside the first dimension is still caught.
  CHECK(RunErrorCode("10 DIM A(2,3)\n20 A(3,1)=5\n") == newbasic::kErrSubscriptOutOfRange);
  // NOTE: SWAP of array elements reports Advanced Feature; recorded in plan.md section A.
}

TEST_CASE("SWAP works on array elements", "[arrays]") {
  CHECK(Row0("10 DIM A(2)\n20 A(0)=1: A(1)=2\n30 SWAP A(0), A(1)\n"
             "40 IF A(0)=2 AND A(1)=1 THEN PRINT \"OK\";\n") == "OK");
  // Mixed: a scalar and an element.
  CHECK(Row0("10 DIM A(2)\n20 A(1)=5: X=9\n30 SWAP X, A(1)\n"
             "40 IF X=5 AND A(1)=9 THEN PRINT \"OK\";\n") == "OK");
  // Two-dimensional subscripts, given as expressions.
  CHECK(Row0("10 DIM A(2,2)\n20 I=1: J=2\n30 A(I,J)=4: A(2,1)=7\n40 SWAP A(I,J), A(2,1)\n"
             "50 IF A(1,2)=7 AND A(2,1)=4 THEN PRINT \"OK\";\n") == "OK");
  // The plain scalar form still works.
  CHECK(Row0("10 X=1: Y=2\n20 SWAP X, Y\n30 IF X=2 AND Y=1 THEN PRINT \"OK\";\n") == "OK");
}
