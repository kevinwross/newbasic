// M2: DEF FN user-defined functions.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "lwbasic/errors.h"
#include "lwbasic/repl.h"
#include "test_helpers.h"

using lwtest::Row0;

TEST_CASE("DEF FN: definition and calls", "[fn]") {
  CHECK(Row0("10 DEF FNA(X)=X*2+1\n20 PRINT FNA(5)\n") == " 11");
  CHECK(Row0("10 DEF FNS(X)=X*X\n20 PRINT FNS(3)\n") == " 9");
  CHECK(Row0("10 DEF FNADD(X,Y)=X+Y\n20 PRINT FNADD(2,3)\n") == " 5");
  // The body may use globals and other built-ins.
  CHECK(Row0("10 DEF FNX(A)=A+K\n20 K=10\n30 PRINT FNX(1)\n") == " 11");
  CHECK(Row0("10 DEF FNR(X)=SQR(X)+1\n20 PRINT FNR(9)\n") == " 4");
  // String-valued functions.
  CHECK(Row0("10 DEF FNB$(S$)=\"[\"+S$+\"]\"\n20 PRINT FNB$(\"hi\")\n") == "[hi]");
  // Parameters are local: the caller's variables are restored afterwards.
  CHECK(Row0("10 X=99\n20 DEF FNA(X)=X+1\n30 PRINT FNA(1);X\n") == " 2  99");
  // Calls nest inside expressions.
  CHECK(Row0("10 DEF FNA(X)=X+1\n20 PRINT FNA(FNA(1))\n") == " 3");
  // The DEF line itself is inert when execution reaches it.
  CHECK(Row0("10 PRINT \"A\"\n20 DEF FNA(X)=X\n30 PRINT FNA(7)\n") == "A");
  // Redefinition: the last definition wins.
  CHECK(Row0("10 DEF FNA(X)=1\n20 DEF FNA(X)=2\n30 PRINT FNA(0)\n") == " 2");
  // Functions work inside loops.
  CHECK(Row0("10 DEF FND(X)=X*2\n20 FOR I=1 TO 3: PRINT FND(I): NEXT I\n") == " 2");
}

TEST_CASE("DEF FN: errors", "[fn][errors]") {
  CHECK(lwtest::CompileCode("10 PRINT FNZ(1)\n") == lwbasic::kErrUndefinedUserFunction);
  CHECK(lwtest::CompileCode("10 DEF FNA(X)=X\n20 PRINT FNA(1,2)\n") == lwbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 DEF FNA(X)=X\n20 PRINT FNA()\n") == lwbasic::kErrSyntax);
  // The parameter list (and the call's parentheses) are optional.
  CHECK(Row0("10 DEF FNA=5\n20 PRINT FNA\n") == " 5");
  CHECK(lwtest::CompileCode("10 DEF FNA(X)=X*\"a\"\n20 PRINT FNA(1)\n") == 0);
  CHECK(lwtest::RunErrorCode("10 DEF FNA(X)=X*\"a\"\n20 PRINT FNA(1)\n") ==
        lwbasic::kErrTypeMismatch);
  // DEF FN is illegal in direct mode.
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  CHECK(repl.Process("DEF FNA(X)=X").code == lwbasic::kErrIllegalDirect);
}

TEST_CASE("DEF FN: composition, arguments and redefinition", "[fn]") {
  // One user function may call another.
  CHECK(Row0("10 DEF FNG(X)=X*2\n20 DEF FNH(X)=FNG(X)+1\n30 PRINT FNH(3)\n") == " 7");
  // Arguments are expressions, evaluated by the caller.
  CHECK(Row0("10 DEF FNA(X)=X+1\n20 PRINT FNA(2*3)\n") == " 7");
  // The body may read a global that changes between calls.
  CHECK(Row0("10 DEF FNP(X)=X*K\n20 K=3\n25 A=FNP(2)\n30 K=4\n35 B=FNP(2)\n"
             "40 IF A=6 AND B=8 THEN PRINT \"OK\";\n") == "OK");
  // Redefining replaces the earlier definition.
  CHECK(Row0("10 DEF FNA(X)=X+1\n20 DEF FNA(X)=X+100\n30 PRINT FNA(1)\n") == " 101");
}
