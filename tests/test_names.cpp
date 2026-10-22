// Whole-word keyword lexing: names may start with or contain reserved words,
// while exact keywords remain reserved and run-together forms need spaces.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "test_helpers.h"

TEST_CASE("Names: keywords are whole words", "[names][lexer]") {
  CHECK(lwtest::Row0("10 TOTAL=41\n20 PRINT TOTAL\n") == " 41");
  CHECK(lwtest::Row0("10 FORT=42\n20 PRINT FORT\n") == " 42");
  CHECK(lwtest::Row0("10 PRINTX=43\n20 PRINT PRINTX\n") == " 43");
  CHECK(lwtest::Row0("10 ANDY=44\n20 PRINT ANDY\n") == " 44");
  CHECK(lwtest::Row0("10 LENGTH=45\n20 PRINT LENGTH\n") == " 45");
  CHECK(lwtest::Row0("10 A.B=46\n20 PRINT A.B\n") == " 46");
}

TEST_CASE("Names: exact keywords stay reserved", "[names]") {
  CHECK(lwtest::CompileCode("10 DIM PRINT(3)\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 GOTO 10\n") == 0);
}

TEST_CASE("Names: run-together keywords now need spaces", "[names]") {
  CHECK(lwtest::CompileCode("10 FORI=1TO10\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 GOTO100\n") == newbasic::kErrSyntax);
  CHECK(lwtest::Row0("10 FOR I=1 TO 3\n20 X=X+I\n30 NEXT I\n40 PRINT X\n") == " 6");
  CHECK(lwtest::Row0("10 GOTO 20\n20 PRINT 7\n") == " 7");
  CHECK(lwtest::Row0("10 IF 1 THEN 20\n20 PRINT 8\n") == " 8");
}

TEST_CASE("Names: FN user functions survive whole-word lexing", "[names][fn]") {
  CHECK(lwtest::Row0("10 DEF FNA(X)=X+1\n20 PRINT FNA(2)\n") == " 3");
  CHECK(lwtest::Row0("10 DEF FN A(X)=X+2\n20 PRINT FNA(2)\n") == " 4");
  // Without a matching DEF FN, FNA is an ordinary scalar name.
  CHECK(lwtest::Row0("10 FNA=5\n20 PRINT FNA\n") == " 5");
}
