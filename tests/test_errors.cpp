// M3: ON ERROR GOTO / RESUME / ERR / ERL / ERROR.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "test_helpers.h"

using lwtest::Row0;
using lwtest::RunErrorCode;
using lwtest::RunRows;

TEST_CASE("Error trapping: ON ERROR GOTO with RESUME NEXT", "[errors]") {
  const std::vector<std::string> rows = RunRows("10 ON ERROR GOTO 100\n"
                                                "20 PRINT 1/0\n"
                                                "30 PRINT \"AFTER\"\n"
                                                "40 END\n"
                                                "100 PRINT \"HANDLED\";ERR;ERL\n"
                                                "110 RESUME NEXT\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "HANDLED 11  20");
  CHECK(rows[1] == "AFTER");
}

TEST_CASE("Error trapping: RESUME retries the failing statement", "[errors]") {
  const std::vector<std::string> rows = RunRows("10 ON ERROR GOTO 100\n"
                                                "20 A=0\n"
                                                "30 PRINT 5/A\n"
                                                "40 PRINT \"DONE\"\n"
                                                "50 END\n"
                                                "100 A=2\n"
                                                "110 RESUME\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 2.5");
  CHECK(rows[1] == "DONE");
}

TEST_CASE("Error trapping: RESUME <line> and the ERROR statement", "[errors]") {
  const std::vector<std::string> rows = RunRows("10 ON ERROR GOTO 100\n"
                                                "20 ERROR 5\n"
                                                "30 PRINT \"SKIPPED\": END\n"
                                                "100 PRINT ERR;ERL\n"
                                                "110 RESUME 30\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 5  20");
  CHECK(rows[1] == "SKIPPED");
}

TEST_CASE("Error trapping: disabling trapping and handler errors", "[errors]") {
  // ON ERROR GOTO 0 disables trapping, so the next error stops the program.
  CHECK(RunErrorCode("10 ON ERROR GOTO 100\n20 ON ERROR GOTO 0\n30 PRINT 1/0\n"
                     "100 PRINT \"NO\"\n") == newbasic::kErrDivisionByZero);
  // An error raised inside the handler is not trapped again.
  CHECK(RunErrorCode("10 ON ERROR GOTO 100\n20 ERROR 7\n100 ERROR 6\n") == newbasic::kErrOverflow);
  // RESUME with no prior error.
  CHECK(RunErrorCode("10 RESUME\n") == newbasic::kErrResumeWithoutError);
  CHECK(RunErrorCode("10 RESUME NEXT\n") == newbasic::kErrResumeWithoutError);
  // ERR and ERL start at zero and survive RESUME.
  CHECK(Row0("10 PRINT ERR;ERL\n") == " 0  0");
  CHECK(Row0("10 ON ERROR GOTO 100\n20 ERROR 7\n30 PRINT \"OK\"\n40 END\n"
             "100 RESUME NEXT\n") == "OK");
  CHECK(Row0("10 ON ERROR GOTO 100\n20 ERROR 7\n30 PRINT ERR\n40 END\n"
             "100 RESUME NEXT\n") == " 7");
  // The handler can sit anywhere in the program and runs when jumped to.
  CHECK(Row0("10 ON ERROR GOTO 100\n20 ERROR 3\n30 END\n100 PRINT \"H\": RESUME 30\n") == "H");
}

TEST_CASE("Conformance: Appendix A error codes", "[errors][conformance]") {
  // Codes are asserted numerically so the test pins the dialect's numbering
  // (Appendix A) rather than our constant names.
  struct Case {
    const char* source;
    int code;
  };
  const std::vector<Case> cases = {
      {"10 NEXT I\n", 1},                                         // NEXT without FOR
      {"10 PRINT )\n", 2},                                        // Syntax error (compile)
      {"10 RETURN\n", 3},                                         // RETURN without GOSUB
      {"10 READ X\n", 4},                                         // Out of DATA
      {"10 ERROR 7\n", 7},                                        // ERROR n raises code n
      {"10 GOTO 999\n", 8},                                       // Undefined line number
      {"10 DIM A(2)\n20 A(5)=1\n", 9},                            // Subscript out of range
      {"10 PRINT 1/0\n", 11},                                     // Division by zero
      {"10 DIM A(2,2)\n20 A(9,9)=1\n", 9},                        // Subscript out of range, 2-D
      {"10 A$=\"x\"*2\n", 13},                                    // Type mismatch
      {"10 OPEN \"no_such_file_xyz.txt\" FOR INPUT AS #1\n", 53}, // File not found
  };
  for (const Case& test_case : cases) {
    INFO("error source: " << test_case.source);
    CHECK(lwtest::RunErrorCode(test_case.source) == test_case.code);
  }
}

TEST_CASE("Error trapping: direct mode", "[errors]") {
  // ON ERROR GOTO <line> is illegal direct; ERR/ERL still work.
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  CHECK(repl.Process("ON ERROR GOTO 100").code == newbasic::kErrIllegalDirect);
  CHECK(repl.Process("PRINT ERR").status == newbasic::RunResult::Status::Done);
  CHECK(screen.RenderRows()[0] == " 0");
}
