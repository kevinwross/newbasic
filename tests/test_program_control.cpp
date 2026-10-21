// M10: program control and editing statements.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

TEST_CASE("M10: RUN clears variables", "[m10]") {
  // RUN <line> restarts at that line with variables cleared, so A prints as 0.
  // (A program that reaches a plain RUN again would restart forever, which is why
  // the restart-from-the-top case below uses the screen position to stop.)
  CHECK(lwtest::Row0("10 A=5\n20 RUN 40\n30 PRINT \"NOT HERE\"\n40 PRINT A\n") == " 0");
}

TEST_CASE("M10: RUN restarts at the first line", "[m10]") {
  // Each pass prints one X and restarts; POS survives RUN (it is screen state, not
  // a variable), so the program stops after a few passes instead of looping.
  const std::vector<std::string> rows =
      lwtest::RunRows("10 PRINT \"X\";\n20 IF POS(0)>3 THEN END\n30 RUN\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "XXX");
}

TEST_CASE("M10: RUN with a line number starts there", "[m10]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 PRINT \"A\";\n"
                                                        "20 RUN 40\n"
                                                        "30 PRINT \"B\";\n"
                                                        "40 PRINT \"C\"\n");
  REQUIRE(rows.size() > 0);
  // PRINT "A"; leaves the cursor on the same row, so "C" continues it.
  CHECK(rows[0] == "AC");
}

TEST_CASE("M10: RUN with an undefined line is Undefined line number", "[m10]") {
  CHECK(lwtest::RunErrorCode("10 RUN 999\n") == newbasic::kErrUndefinedLineNumber);
}

TEST_CASE("M10: TRON traces line numbers, TROFF stops", "[m10]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 TROFF\n"
                                                        "20 TRON\n"
                                                        "30 A=1\n"
                                                        "40 TROFF\n"
                                                        "50 A=2\n");
  // The trace echoes the line number as execution enters each traced line. Line 30
  // runs while tracing; line 50 does not.
  std::string all;
  for (const std::string& row : rows)
    all += row + "\n";
  CHECK(all.find("[30]") != std::string::npos);
  CHECK(all.find("[50]") == std::string::npos);
}

TEST_CASE("M10: direct-mode-only statements report Illegal direct", "[m10]") {
  CHECK(lwtest::RunErrorCode("10 AUTO\n") == newbasic::kErrIllegalDirect);
  CHECK(lwtest::RunErrorCode("10 EDIT 10\n") == newbasic::kErrIllegalDirect);
}

TEST_CASE("M10: unimplemented program statements report a clear error", "[m10]") {
  // CONT only makes sense after a stop; without one the reference says Cannot
  // continue. LLIST needs a printer, which newbasic does not have. RENUM is a
  // direct-mode command now, so inside a program it is Illegal direct.
  CHECK(lwtest::RunErrorCode("10 CONT\n") == newbasic::kErrCantContinue);
  CHECK(lwtest::RunErrorCode("10 LLIST\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 RENUM\n") == newbasic::kErrIllegalDirect);
  CHECK(lwtest::RunErrorCode("10 SAVE \"x\"\n") == newbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 CHAIN \"x\"\n") == newbasic::kErrAdvancedFeature);
}

// The program-editing statements act on the stored source lines, so they are
// exercised through the REPL, which owns the store.
TEST_CASE("M10: LIST inside a program prints the stored lines", "[m10]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"A\"");
  repl.Process("20 LIST");
  repl.Process("30 END");
  screen.Clear();
  repl.Process("RUN");
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(rows.size() >= 4);
  // Line 10 prints first; the listing follows it.
  CHECK(rows[0] == "A");
  CHECK(rows[1] == "10 PRINT \"A\"");
  CHECK(rows[2] == "20 LIST");
  CHECK(rows[3] == "30 END");
}

TEST_CASE("M10: LIST with a line number lists just that line", "[m10]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"A\"");
  repl.Process("20 LIST 30");
  repl.Process("30 END");
  screen.Clear();
  repl.Process("RUN");
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "A"); // line 10 printed before the LIST at line 20
  CHECK(rows[1] == "30 END");
}

TEST_CASE("M10: DELETE removes lines from the store but not the running code", "[m10]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 DELETE 40");
  repl.Process("20 LIST");
  repl.Process("30 PRINT \"GONE\"");
  repl.Process("40 END");
  screen.Clear();
  repl.Process("RUN");
  std::string all;
  for (const std::string& row : screen.RenderRows())
    all += row + "\n";
  // Line 40 is no longer listed, but the compiled program still executes the
  // lines it was compiled from, so GONE still prints.
  CHECK(all.find("40 END") == std::string::npos);
  CHECK(all.find("GONE") != std::string::npos);
  CHECK(repl.Store().Size() == 3);
}

TEST_CASE("M10: NEW clears the stored program and stops execution", "[m10]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 NEW");
  repl.Process("20 PRINT \"AFTER\"");
  screen.Clear();
  repl.Process("RUN");
  std::string all;
  for (const std::string& row : screen.RenderRows())
    all += row + "\n";
  // Execution ends at NEW, so the following line never runs, and the store is
  // empty afterwards.
  CHECK(all.find("AFTER") == std::string::npos);
  CHECK(repl.Store().Size() == 0);
}
