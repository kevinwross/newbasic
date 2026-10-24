// Named line labels: `LABEL name` gives GOTO/GOSUB line targets a symbolic
// spelling that RENUM can leave alone (the label definition moves with its line).
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

TEST_CASE("LABEL: forward GOSUB and RETURN", "[label]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 GOSUB Work\n"
                                                        "20 PRINT \"BACK\"\n"
                                                        "30 END\n"
                                                        "100 LABEL Work\n"
                                                        "110 PRINT \"SUB\"\n"
                                                        "120 RETURN\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "SUB");
  CHECK(rows[1] == "BACK");
}

TEST_CASE("LABEL: GOTO, IF THEN, and case-insensitive names", "[label]") {
  CHECK(lwtest::Row0("10 GOTO Forward\n"
                     "20 PRINT \"SKIP\"\n"
                     "30 END\n"
                     "100 LABEL Forward\n"
                     "110 PRINT \"OK\"\n") == "OK");

  CHECK(lwtest::Row0("10 X = 1\n"
                     "20 IF X THEN Target\n"
                     "30 PRINT \"SKIP\"\n"
                     "40 END\n"
                     "100 LABEL Target\n"
                     "110 PRINT \"OK\"\n") == "OK");

  CHECK(lwtest::Row0("10 GOSUB target\n"
                     "20 PRINT \"BACK\"\n"
                     "30 END\n"
                     "100 LABEL TARGET\n"
                     "110 PRINT \"SUB\"\n"
                     "120 RETURN\n") == "SUB");
}

TEST_CASE("LABEL: label shares a line with code", "[label]") {
  CHECK(lwtest::Row0("10 GOSUB Work\n"
                     "20 END\n"
                     "100 LABEL Work: PRINT \"SUB\": RETURN\n") == "SUB");
}

TEST_CASE("LABEL: 32-bit label line", "[label]") {
  CHECK(lwtest::Row0("10 GOSUB Far\n"
                     "20 PRINT \"BACK\"\n"
                     "30 END\n"
                     "2000000000 LABEL Far\n"
                     "2000000010 PRINT \"SUB\"\n"
                     "2000000020 RETURN\n") == "SUB");
}

TEST_CASE("LABEL: ON ERROR and ON GOTO may use labels", "[label]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO Handler\n"
                     "20 ERROR 5\n"
                     "30 END\n"
                     "100 LABEL Handler\n"
                     "110 PRINT ERR\n"
                     "120 RESUME NEXT\n") == " 5");

  CHECK(lwtest::Row0("10 ON 2 GOTO One, Two\n"
                     "20 END\n"
                     "100 LABEL One\n"
                     "110 PRINT \"ONE\": END\n"
                     "200 LABEL Two\n"
                     "210 PRINT \"TWO\": END\n") == "TWO");
}

TEST_CASE("LABEL: duplicate and undefined labels are compile errors", "[label]") {
  CHECK(lwtest::CompileCode("10 LABEL A\n20 LABEL A\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 GOTO Missing\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(lwtest::CompileCode("10 GOSUB Missing\n") == newbasic::kErrUndefinedLineNumber);
}

TEST_CASE("LABEL: direct mode rejects LABEL", "[label]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  const newbasic::RunResult result = repl.Process("LABEL X");
  CHECK(result.status == newbasic::RunResult::Status::Error);
  CHECK(result.code == newbasic::kErrIllegalDirect);
}

TEST_CASE("LABEL: RENUM moves the definition without rewriting the call", "[label][renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 GOSUB Work");
  repl.Process("20 END");
  repl.Process("100 LABEL Work");
  repl.Process("110 PRINT \"OK\"");
  repl.Process("120 RETURN");
  const newbasic::RunResult renumbered = repl.Process("RENUM 1000,,100");
  CHECK(renumbered.status == newbasic::RunResult::Status::Done);
  screen.Clear();
  CHECK(repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(screen.RenderRows()[0] == "OK");
}

TEST_CASE("LABEL: computed GOTO/GOSUB still works when no label matches", "[label]") {
  CHECK(lwtest::Row0("10 T% = 200\n"
                     "20 GOSUB T%\n"
                     "30 PRINT \"BACK\"\n"
                     "40 END\n"
                     "200 PRINT \"SUB\"\n"
                     "210 RETURN\n") == "SUB");
}
