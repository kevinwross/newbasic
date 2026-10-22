// M1 direct mode: the REPL session (numbered lines, commands, direct
// statements sharing one VM).
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"

namespace {

struct Session {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl{screen, &host};
};

} // namespace

TEST_CASE("REPL: numbered lines, RUN and LIST", "[repl]") {
  Session s;
  CHECK(s.repl.Process("10 PRINT \"HI\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("20 PRINT \"THERE\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 2);

  const newbasic::RunResult run = s.repl.Process("RUN");
  CHECK(run.status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = s.screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "HI");
  CHECK(rows[1] == "THERE");

  s.screen.Clear();
  CHECK(s.repl.Process("LIST").status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> listed = s.screen.RenderRows();
  REQUIRE(listed.size() >= 2);
  CHECK(listed[0] == "10 PRINT \"HI\"");
  CHECK(listed[1] == "20 PRINT \"THERE\"");
}

TEST_CASE("REPL: direct statements share variables with the program", "[repl]") {
  Session s;
  CHECK(s.repl.Process("A=5").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("PRINT A").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == " 5");
  s.screen.Clear();
  CHECK(s.repl.Process("PRINT A+1").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == " 6");
}

TEST_CASE("REPL: line editing (replace, delete, NEW)", "[repl]") {
  Session s;
  s.repl.Process("10 PRINT 1");
  s.repl.Process("10 PRINT 2");
  CHECK(s.repl.Store().Size() == 1);
  CHECK(s.repl.Store().Lines()[0].source == "PRINT 2");

  s.repl.Process("10"); // a bare line number deletes the line
  CHECK(s.repl.Store().Size() == 0);

  s.repl.Process("10 PRINT 1");
  s.repl.Process("20 PRINT 2");
  s.repl.Process("NEW");
  CHECK(s.repl.Store().Size() == 0);
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
}

TEST_CASE("REPL: CLEAR drops variables", "[repl]") {
  Session s;
  s.repl.Process("A=5");
  s.repl.Process("CLEAR");
  s.repl.Process("PRINT A");
  CHECK(s.screen.RenderRows()[0] == " 0");
}

TEST_CASE("REPL: RUN from a line and undefined lines", "[repl]") {
  Session s;
  s.repl.Process("10 PRINT \"A\"");
  s.repl.Process("20 PRINT \"B\"");
  CHECK(s.repl.Process("RUN 20").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "B");

  const newbasic::RunResult bad = s.repl.Process("RUN 999");
  CHECK(bad.status == newbasic::RunResult::Status::Error);
  CHECK(bad.code == newbasic::kErrUndefinedLineNumber);
}

TEST_CASE("REPL: direct-mode errors", "[repl]") {
  Session s;
  const newbasic::RunResult divide = s.repl.Process("PRINT 1/0");
  CHECK(divide.status == newbasic::RunResult::Status::Error);
  CHECK(divide.code == newbasic::kErrDivisionByZero);

  const newbasic::RunResult illegal = s.repl.Process("FOR I=1 TO 3");
  CHECK(illegal.status == newbasic::RunResult::Status::Error);
  CHECK(illegal.code == newbasic::kErrIllegalDirect);

  CHECK(s.repl.Process("GOTO 10").code == newbasic::kErrIllegalDirect);
  CHECK(s.repl.Process("CONT").code == newbasic::kErrCantContinue);
  CHECK(s.repl.Process("10 FOO").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("RUN").code == newbasic::kErrSyntax);
}

TEST_CASE("REPL: INPUT reads scripted lines", "[repl][input]") {
  Session s;
  s.repl.Process("10 INPUT A");
  s.repl.Process("20 PRINT A*2");
  s.screen.ScriptLine(L"21");
  const newbasic::RunResult run = s.repl.Process("RUN");
  CHECK(run.status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = s.screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "?");
  CHECK(rows[1] == " 42");
}

TEST_CASE("REPL: INPUT redos invalid data", "[repl][input]") {
  Session s;
  s.repl.Process("10 INPUT A,B$");
  s.repl.Process("20 PRINT B$;A");
  s.screen.ScriptLine(L"nope,hi");
  s.screen.ScriptLine(L"7,hi");
  const newbasic::RunResult run = s.repl.Process("RUN");
  CHECK(run.status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = s.screen.RenderRows();
  bool saw_redo = false;
  for (const std::string& row : rows) {
    if (row.find("Redo from start") != std::string::npos)
      saw_redo = true;
  }
  CHECK(saw_redo);
  bool saw_value = false;
  for (const std::string& row : rows) {
    if (row == "hi 7")
      saw_value = true;
  }
  CHECK(saw_value);
}

TEST_CASE("REPL: LINE INPUT keeps commas", "[repl][input]") {
  Session s;
  s.repl.Process("10 LINE INPUT A$");
  s.repl.Process("20 PRINT A$");
  s.screen.ScriptLine(L"a,b,c");
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = s.screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[1] == "a,b,c");
}

// The prompt string is part of the INPUT statement, not an expression: it is printed
// before the read. A semicolon after it adds the automatic "? ", a comma suppresses it.
TEST_CASE("REPL: INPUT prints its prompt string", "[repl][input]") {
  Session semicolon;
  semicolon.repl.Process("10 INPUT \"NAME: \"; A$");
  semicolon.screen.ScriptLine(L"KEVIN");
  CHECK(semicolon.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(semicolon.screen.RenderRows()[0] == "NAME: ?");

  Session comma;
  comma.repl.Process("10 INPUT \"NAME: \", A$");
  comma.screen.ScriptLine(L"KEVIN");
  CHECK(comma.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(comma.screen.RenderRows()[0] == "NAME:");
}

// The reference is explicit: LINE INPUT prints no question mark unless one is part of
// the prompt string itself.
TEST_CASE("REPL: LINE INPUT prints no question mark", "[repl][input]") {
  Session s;
  s.repl.Process("10 LINE INPUT \"ADDRESS: \"; A$");
  s.screen.ScriptLine(L"1 MAIN ST");
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "ADDRESS:");
}

TEST_CASE("REPL: SYSTEM ends the session", "[repl]") {
  Session s;
  CHECK_FALSE(s.repl.ShouldExit());
  s.repl.Process("SYSTEM");
  CHECK(s.repl.ShouldExit());
}

TEST_CASE("REPL: Step prints Ok and stops at end of input", "[repl]") {
  Session s;
  s.screen.ScriptLine(L"PRINT 7");
  CHECK(s.repl.Step());
  const std::vector<std::string> rows = s.screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "Ok");
  CHECK(rows[1] == " 7");
  CHECK_FALSE(s.repl.Step()); // scripted input exhausted
}

TEST_CASE("REPL: DELETE removes a range", "[repl]") {
  Session s;
  s.repl.Process("10 PRINT 1");
  s.repl.Process("20 PRINT 2");
  s.repl.Process("30 PRINT 3");
  s.repl.Process("DELETE 10-20");
  CHECK(s.repl.Store().Size() == 1);
  CHECK(s.repl.Store().Lines()[0].line_number == 30);
  CHECK(s.repl.Process("DELETE").code == newbasic::kErrSyntax);
}

TEST_CASE("REPL: Ok marks readiness, not every line", "[repl]") {
  // The marker belongs after a program runs, not after every direct statement: entering lines
  // and commands should not plaster Ok down the screen.
  Session s;
  s.screen.ScriptLine(L"10 PRINT 1");
  s.screen.ScriptLine(L"RUN");
  s.screen.ScriptLine(L"PRINT 5"); // a direct statement, which should not add a marker
  while (s.repl.Step()) {
  }
  int markers = 0;
  for (const std::string& row : s.screen.RenderRows()) {
    if (row.find("Ok") != std::string::npos)
      ++markers;
  }
  // One at the start of the session and one after RUN.
  CHECK(markers == 2);
}

TEST_CASE("REPL: a reserved word used as a name says which word", "[repl]") {
  // PRINT is reserved, so PRINT cannot name an array. The failure should name the
  // collision instead of being a bare syntax error, which is what cost real time once.
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 DIM PRINT(3)");
  const newbasic::RunResult result = repl.Process("RUN"); // compilation happens here
  CHECK(result.status == newbasic::RunResult::Status::Error);
  CHECK(result.message.find("PRINT") != std::string::npos);
  CHECK(result.message.find("reserved") != std::string::npos);
}
