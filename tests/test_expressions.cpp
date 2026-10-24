// M1 expression evaluation: operator precedence, typing, built-in functions.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/host.h"
#include "test_helpers.h"

using lwtest::Row0;
using lwtest::RunRows;

TEST_CASE("Expressions: arithmetic precedence", "[expr]") {
  CHECK(Row0("10 PRINT 2+3*4\n") == " 14");
  CHECK(Row0("10 PRINT (2+3)*4\n") == " 20");
  CHECK(Row0("10 PRINT 10-2-3\n") == " 5");
  CHECK(Row0("10 PRINT 2^3^2\n") == " 512"); // right associative
  CHECK(Row0("10 PRINT -2^2\n") == "-4");    // ^ binds tighter than unary minus
  CHECK(Row0("10 PRINT 7/2\n") == " 3.5");
  CHECK(Row0("10 PRINT 7\\2\n") == " 3");
  CHECK(Row0("10 PRINT 7 MOD 2\n") == " 1");
  CHECK(Row0("10 PRINT 2*3 MOD 4\n") == " 2");
  CHECK(Row0("10 PRINT 1+2=3\n") == "-1");
}

TEST_CASE("Expressions: relational and logical operators", "[expr]") {
  CHECK(Row0("10 PRINT 1=1\n") == "-1");
  CHECK(Row0("10 PRINT 1=2\n") == " 0");
  CHECK(Row0("10 PRINT 2>1\n") == "-1");
  CHECK(Row0("10 PRINT 2<>2\n") == " 0");
  CHECK(Row0("10 PRINT NOT 0\n") == "-1");
  CHECK(Row0("10 PRINT NOT 1\n") == "-2");
  CHECK(Row0("10 PRINT 5 AND 3\n") == " 1");
  CHECK(Row0("10 PRINT 5 OR 3\n") == " 7");
  CHECK(Row0("10 PRINT 5 XOR 3\n") == " 6");
  CHECK(Row0("10 PRINT \"A\"<\"B\"\n") == "-1");
  CHECK(Row0("10 PRINT \"B\"<\"A\"\n") == " 0");
}

TEST_CASE("Expressions: string functions", "[expr]") {
  CHECK(Row0("10 PRINT \"AB\"+\"CD\"\n") == "ABCD");
  CHECK(Row0("10 PRINT LEN(\"HELLO\")\n") == " 5");
  CHECK(Row0("10 PRINT LEFT$(\"HELLO\",2)\n") == "HE");
  CHECK(Row0("10 PRINT RIGHT$(\"HELLO\",2)\n") == "LO");
  CHECK(Row0("10 PRINT MID$(\"HELLO\",2,3)\n") == "ELL");
  CHECK(Row0("10 PRINT MID$(\"HELLO\",3)\n") == "LLO");
  CHECK(Row0("10 PRINT STR$(42)\n") == " 42");
  CHECK(Row0("10 PRINT ASC(\"A\")\n") == " 65");
  CHECK(Row0("10 PRINT CHR$(65)\n") == "A");
  CHECK(Row0("10 PRINT VAL(\"12.5\")\n") == " 12.5");
  CHECK(Row0("10 PRINT VAL(\"x\")\n") == " 0");
  CHECK(Row0("10 PRINT INSTR(\"HELLO\",\"LL\")\n") == " 3");
  CHECK(Row0("10 PRINT INSTR(4,\"HELLO\",\"L\")\n") == " 4");
  CHECK(Row0("10 PRINT INSTR(\"HELLO\",\"Z\")\n") == " 0");
  CHECK(Row0("10 PRINT HEX$(255)\n") == "FF");
  CHECK(Row0("10 PRINT OCT$(8)\n") == "10");
  CHECK(Row0("10 PRINT LEN(SPACE$(3))\n") == " 3");
  CHECK(Row0("10 PRINT STRING$(3,\"*\")\n") == "***");
}

TEST_CASE("Expressions: math functions", "[expr]") {
  CHECK(Row0("10 PRINT ABS(-3)\n") == " 3");
  CHECK(Row0("10 PRINT INT(-2.5)\n") == "-3");
  CHECK(Row0("10 PRINT FIX(-2.5)\n") == "-2");
  CHECK(Row0("10 PRINT SGN(-5)\n") == "-1");
  CHECK(Row0("10 PRINT SGN(0)\n") == " 0");
  CHECK(Row0("10 PRINT SQR(16)\n") == " 4");
  CHECK(Row0("10 PRINT CINT(2.6)\n") == " 3");
  CHECK(Row0("10 PRINT CSNG(1)/3\n") == " .3333333");
  CHECK(Row0("10 PRINT CDBL(1)/3\n") == " .3333333333333333");
  CHECK(Row0("10 PRINT 1E10\n") == " 1E+10");
  CHECK(Row0("10 PRINT SIN(0)\n") == " 0");
  CHECK(Row0("10 PRINT ATN(1)*4\n") == " 3.141593");
  CHECK(Row0("10 PRINT LOG(1)\n") == " 0");
  CHECK(Row0("10 PRINT EXP(0)\n") == " 1");
}

TEST_CASE("Expressions: variables and assignment typing", "[expr]") {
  CHECK(Row0("10 A=5: PRINT A\n") == " 5");
  CHECK(Row0("10 A$=\"HI\": PRINT A$\n") == "HI");
  CHECK(Row0("10 A%=3.7: PRINT A%\n") == " 4");
  CHECK(Row0("10 A%=3.2: PRINT A%\n") == " 3");
  CHECK(Row0("10 A#=1/3: PRINT A#\n") == " .3333333333333333");
  CHECK(Row0("10 A!=1/3: PRINT A!\n") == " .3333333");
  CHECK(Row0("10 PRINT A\n") == " 0");
  CHECK(Row0("10 PRINT A$\n") == "");
  CHECK(Row0("10 LET B=2: PRINT B\n") == " 2");
  CHECK(Row0("10 A=1: B=2: SWAP A,B: PRINT A;B\n") == " 2  1");
  CHECK(Row0("10 A=1: A=A+1: PRINT A\n") == " 2");
  // A and A! are the same variable; A$ and A% are distinct.
  CHECK(Row0("10 A=7: PRINT A!\n") == " 7");
  CHECK(Row0("10 A=7: A$=\"x\": PRINT A;A$\n") == " 7 x");
}

TEST_CASE("Expressions: host functions and screen state", "[expr]") {
  newbasic::ScriptedHost host;
  host.timer = 1234.5;
  const std::vector<std::string> rows =
      lwtest::RunRowsWithInput("10 PRINT TIMER\n20 PRINT DATE$\n30 PRINT TIME$\n", {}, &host);
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == " 1234.5");
  CHECK(rows[1] == "01-01-1980");
  CHECK(rows[2] == "00:00:00");
  CHECK(Row0("10 PRINT CSRLIN\n") == " 1");
  CHECK(Row0("10 PRINT POS(0)\n") == " 1");
}

TEST_CASE("Expressions: RND is reproducible and RND(0) repeats", "[expr][rnd]") {
  newbasic::ScriptedHost host;
  const std::vector<std::string> rows =
      lwtest::RunRowsWithInput("10 RANDOMIZE 42\n20 A=RND\n30 B=RND(0)\n40 PRINT A=B\n", {}, &host);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "-1"); // RND(0) returns the previous value
}

TEST_CASE("Expressions: runtime errors", "[expr][errors]") {
  CHECK(lwtest::RunErrorCode("10 PRINT 1/0\n") == newbasic::kErrDivisionByZero);
  CHECK(lwtest::RunErrorCode("10 PRINT 1+\"A\"\n") == newbasic::kErrTypeMismatch);
  CHECK(lwtest::RunErrorCode("10 A$=1\n") == newbasic::kErrTypeMismatch);
  CHECK(lwtest::Row0("10 A%=40000\n20 PRINT A%\n") == " 40000");
  CHECK(lwtest::Row0("10 A&=40000\n20 PRINT A&\n") == " 40000");
  CHECK(lwtest::Row0("10 A%=2147483647\n20 PRINT A%\n") == " 2147483647");
  CHECK(lwtest::Row0("10 PRINT CINT(99999)\n") == " 99999");
  CHECK(lwtest::RunErrorCode("10 A%=2147483648\n") == newbasic::kErrOverflow);
  CHECK(lwtest::RunErrorCode("10 A&=2147483648\n") == newbasic::kErrOverflow);
  CHECK(lwtest::RunErrorCode("10 A%=2147483647\n20 A%=A%+1\n") == newbasic::kErrOverflow);
  CHECK(lwtest::RunErrorCode("10 PRINT CINT(2147483648)\n") == newbasic::kErrOverflow);
  CHECK(lwtest::Row0("10 DEFINT A\n20 A=40000\n30 PRINT A\n") == " 40000");
  CHECK(lwtest::Row0("10 PRINT CINT(-2147483648)\n") == "-2147483648");
  {
    const std::vector<std::string> rows =
        lwtest::RunRows("10 FOR I%=2147483646 TO 2147483647\n20 PRINT I%\n30 NEXT\n");
    REQUIRE(rows.size() >= 2);
    CHECK(rows[0] == " 2147483646");
    CHECK(rows[1] == " 2147483647");
  }
  CHECK(lwtest::RunErrorCode("10 PRINT SQR(-1)\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT LOG(0)\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT CHR$(-1)\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT MID$(\"abc\",0)\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT LEFT$(\"abc\",-1)\n") == newbasic::kErrIllegalFunctionCall);
  // Errors report the line they occurred in.
  int line = 0;
  CHECK(lwtest::RunErrorCode("10 A=1\n20 PRINT 1/0\n", &line) == newbasic::kErrDivisionByZero);
  CHECK(line == 20);
}

TEST_CASE("Expressions: unsupported statements fail predictably", "[expr]") {
  int line = 0;
  // PEN is still unimplemented (it needs light-pen hardware); it reports Advanced
  // Feature at run time and names the line.
  CHECK(lwtest::RunErrorCode("10 PEN ON\n", &line) == newbasic::kErrAdvancedFeature);
  CHECK(line == 10);
  // SOUND and PLAY are implemented (M7); the graphics statements, WINDOW/PMAP and PCOPY
  // since M12.
  CHECK(lwtest::RunErrorCode("10 CALL X\n") == newbasic::kErrAdvancedFeature);
}
