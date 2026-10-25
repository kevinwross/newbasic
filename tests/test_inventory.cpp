// Statement inventory (plan.md 7b): keeps the list of unimplemented statements and
// the compiler's actual behaviour in step. A statement that gains an implementation
// must be removed from kUnimplementedStatements, and this test fails if it is not -
// which is exactly how the table rotted unnoticed during M8-M11.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

namespace {

// Statements with an explicit branch: implemented, or refused with a documented
// error. None of these may report Advanced Feature for the bare form, because the
// branch is expected to run first.
const char* const kImplemented[] = {
    "CHAIN",
    "CLEAR",
    "CLOSE",
    "COMMON",
    "CONT",
    "DELETE",
    "FIELD",
    "GET",
    "LOAD",
    "LOC",
    "LOF",
    "LSET",
    "MERGE",
    "NEW",
    "OPEN",
    "PLAY",
    "PUT",
    "RSET",
    "SAVE",
    "SOUND",
    "TRON",
    "TROFF",
    "WIDTH",
    "WRITE",
    "LPRINT",
    // 2026-10-24 extensions.
    "SELECT",
    "CASE",
    "SLEEP",
    "PUSH",
    "POP",
    "LABEL",
    "CONST",
    "STACK",
    "SUB",
    "FUNCTION",
    "LOCAL",
    "EXIT",
    "CALL",
    // Graphics: phases 1 to 3.
    "SCREEN",
    "PSET",
    "PRESET",
    "LINE",
    "CIRCLE",
    "PAINT",
    "DRAW",
    "DRAWTEXT",
    "PALETTE",
};

// LIST and LLIST are deliberately absent: both need the stored program, and this
// harness runs without a store, so the correct behaviour there is to refuse. Both
// are covered by the REPL-driven tests (M10 for LIST, the printer tests for LLIST).
// RUN is deliberately absent: a bare RUN inside a program restarts it forever, so
// it cannot be probed by running it. RUN <line> and variable clearing are covered
// by the M10 tests.

// Statements with no implementation: they report Advanced Feature. This list must
// match kUnimplementedStatements in frontend.cpp.
const char* const kStubs[] = {
    "CHDIR", "COM", "ENVIRON", "FILES", "INP",   "IOCTL", "KEY", "KILL",   "MKDIR", "MOTOR",
    "NAME",  "OUT", "PEN",     "POKE",  "RMDIR", "STICK", "STRIG", "USR", "VARPTR", "WAIT",
};

} // namespace

TEST_CASE("Inventory: implemented statements do not report Advanced Feature", "[inventory]") {
  for (const char* keyword : kImplemented) {
    const int code = lwtest::RunErrorCode(std::string("10 ") + keyword + "\n");
    INFO("keyword: " << keyword << " code: " << code);
    // A missing operand is fine (a syntax error proves the branch exists); the
    // fault would be reporting that the statement is unimplemented.
    CHECK(code != newbasic::kErrAdvancedFeature);
  }
}

TEST_CASE("Inventory: unimplemented statements report Advanced Feature", "[inventory]") {
  for (const char* keyword : kStubs) {
    const int code = lwtest::RunErrorCode(std::string("10 ") + keyword + "\n");
    INFO("keyword: " << keyword << " code: " << code);
    CHECK(code == newbasic::kErrAdvancedFeature);
  }
}

TEST_CASE("Inventory: the removed KEY family refuses every form", "[inventory][key]") {
  CHECK(lwtest::RunErrorCode("10 KEY 1,\"HELP\"\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY ON\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY OFF\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY LIST\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY(1) ON\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY(1) OFF\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 KEY(1) STOP\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 ON KEY(1) GOSUB 100\n100 RETURN\n") ==
        newbasic::kErrAdvancedFeature);
}

TEST_CASE("Inventory: refusals carry their documented error, not Advanced Feature", "[inventory]") {
  // Statements that exist but cannot be done in a program still say why.
  CHECK(lwtest::RunErrorCode("10 AUTO\n") == newbasic::kErrIllegalDirect);
  CHECK(lwtest::RunErrorCode("10 EDIT 10\n") == newbasic::kErrIllegalDirect);
  CHECK(lwtest::RunErrorCode("10 MERGE \"x\"\n") == newbasic::kErrIllegalDirect);
  CHECK(lwtest::RunErrorCode("10 CONT\n") == newbasic::kErrCantContinue);
  CHECK(lwtest::RunErrorCode("10 LLIST\n") == newbasic::kErrAdvancedFeature);
  // RENUM is a direct-mode command now, so inside a program it is Illegal direct.
  CHECK(lwtest::RunErrorCode("10 RENUM\n") == newbasic::kErrIllegalDirect);
}

TEST_CASE("Inventory: statements converted to functions are reachable", "[inventory]") {
  // These were once stub *statements*; they are functions now (M9).
  CHECK(lwtest::Row0("10 PRINT LEN(MKI$(1));LEN(MKS$(1));LEN(MKD$(1))\n") == " 2  4  8");
  CHECK(lwtest::Row0("10 PRINT CVI(MKI$(7))\n") == " 7");
  CHECK(lwtest::Row0("10 PRINT CVS(MKS$(1.5))\n") == " 1.5");
  CHECK(lwtest::Row0("10 PRINT CVD(MKD$(1.5))\n") == " 1.5");
}

TEST_CASE("Inventory: machine-level access refuses clearly (M13)", "[inventory]") {
  // The M13 decision: no machine model, so these report Advanced Feature with the
  // function name. Before this they were syntax errors, which is the gap it closed.
  // POINT is implemented now (M12 phase 1), so it is not in this list.
  const char* const kMachineFunctions[] = {"PEEK(100)", "FRE(0)",   "VARPTR(A)", "INP(60)",
                                           "STICK(0)",  "STRIG(0)", "USR(1)"};
  for (const char* call : kMachineFunctions) {
    const int code = lwtest::RunErrorCode(std::string("10 PRINT ") + call + "\n");
    INFO("call: " << call << " code: " << code);
    CHECK(code == newbasic::kErrAdvancedFeature);
  }
  // The statements keep reporting Advanced Feature too.
  CHECK(lwtest::RunErrorCode("10 POKE 100,1\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 OUT 60,1\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 WAIT 60,1\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 CALL X\n") == newbasic::kErrUndefinedProcedure);
  CHECK(lwtest::RunErrorCode("10 MOTOR\n") == newbasic::kErrAdvancedFeature);
}

TEST_CASE("Inventory: partially implemented statements still do their part", "[inventory]") {
  // VIEW PRINT is the text window; VIEW (no arguments) resets the graphics viewport,
  // both of which work now.
  CHECK(lwtest::Row0("10 VIEW PRINT 1 TO 5\n20 PRINT \"OK\"\n") == "OK");
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 VIEW\n") == 0);
  // CLEAR works bare but not with arguments.
  CHECK(lwtest::Row0("10 A=1\n20 CLEAR\n30 PRINT A\n") == " 0");
  CHECK(lwtest::RunErrorCode("10 CLEAR ,,4096\n") == newbasic::kErrAdvancedFeature);
}
