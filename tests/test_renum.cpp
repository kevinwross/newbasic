// RENUM and MERGE (plan.md 7b): renumbering must rewrite line references, skip
// strings and comments, refuse reordering, and report undefined references.
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

namespace {

std::vector<std::string> ListProgram(newbasic::Repl* repl,
                                     newbasic::screen::HeadlessScreen* screen) {
  screen->Clear();
  repl->Process("LIST");
  return screen->RenderRows();
}

std::string AllRows(const newbasic::screen::HeadlessScreen& screen) {
  std::string all;
  for (const std::string& row : screen.RenderRows())
    all += row + "\n";
  return all;
}

// HeadlessScreen deliberately does not echo ReadLine input; the real console
// does. This variant mirrors the console so prompt-and-input layout is testable.
class EchoingScreen : public newbasic::screen::HeadlessScreen {
public:
  std::wstring ReadLine() override {
    std::wstring line = newbasic::screen::HeadlessScreen::ReadLine();
    WriteAtCursor(line);
    NewLine();
    return line;
  }
};

struct TempFile {
  explicit TempFile(const char* base) : path_(lwtest::TempName(base)) {}
  ~TempFile() { std::remove(path_.c_str()); }
  const std::string& Path() const { return path_; }
  void Write(const std::string& text) const {
    std::ofstream out(path_, std::ios::binary);
    out << text;
  }

private:
  std::string path_;
};

} // namespace

TEST_CASE("RENUM: renumbers lines and rewrites GOTO", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 GOTO 30");
  repl.Process("20 PRINT \"X\"");
  repl.Process("30 PRINT \"Y\"");
  repl.Process("RENUM 100,,100");
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == "100 GOTO 300"); // the reference was rewritten
  CHECK(rows[1] == "200 PRINT \"X\"");
  CHECK(rows[2] == "300 PRINT \"Y\"");
}

TEST_CASE("RENUM: only lines from the old number onward move", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"A\"");
  repl.Process("20 GOTO 30");
  repl.Process("30 PRINT \"B\"");
  repl.Process("RENUM 100,20");
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == "10 PRINT \"A\""); // before the old number: untouched
  CHECK(rows[1] == "100 GOTO 110");
  CHECK(rows[2] == "110 PRINT \"B\"");
}

TEST_CASE("RENUM: references in strings and comments are left alone", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"GOTO 30\": GOTO 30");
  repl.Process("20 END");
  repl.Process("30 END");
  repl.Process("RENUM 100,,100");
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(!rows.empty());
  // The quoted text keeps its number; the real reference after the colon is
  // rewritten.
  CHECK(rows[0] == "100 PRINT \"GOTO 30\": GOTO 300");
}

TEST_CASE("RENUM: reordering the program is Illegal function call", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"A\"");
  repl.Process("20 PRINT \"B\"");
  repl.Process("30 PRINT \"C\"");
  // Renumbering from line 30 to 15 would put it before line 20.
  const newbasic::RunResult result = repl.Process("RENUM 15,30");
  CHECK(result.status == newbasic::RunResult::Status::Error);
  CHECK(result.code == newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("RENUM: an undefined reference is reported and left unchanged", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 GOTO 999");
  repl.Process("20 END");
  const newbasic::RunResult result = repl.Process("RENUM 100,,100");
  CHECK(result.status == newbasic::RunResult::Status::Error);
  CHECK(result.code == newbasic::kErrUndefinedLineNumber);
  CHECK(result.message.find("999") != std::string::npos);
  // The bad reference is not silently changed.
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "100 GOTO 999");
}

TEST_CASE("RENUM: 32-bit line numbers and references", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 GOTO 20");
  repl.Process("20 PRINT \"HIGH\"");
  repl.Process("30 END");
  const newbasic::RunResult result = repl.Process("RENUM 2000000000,,10");
  CHECK(result.status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == "2000000000 GOTO 2000000010");
  CHECK(rows[1] == "2000000010 PRINT \"HIGH\"");
  CHECK(rows[2] == "2000000020 END");
  screen.Clear();
  CHECK(repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(screen.RenderRows()[0] == "HIGH");
}

TEST_CASE("RENUM: the 32-bit line ceiling", "[renum]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 END");
  CHECK(repl.Process("RENUM 2147483647,,10").status == newbasic::RunResult::Status::Done);
  CHECK(repl.Store().Lines()[0].line_number == newbasic::kMaxLineNumber);

  repl.Process("10 END");
  repl.Process("20 END");
  const newbasic::RunResult overflow = repl.Process("RENUM 2147483640,,10");
  CHECK(overflow.status == newbasic::RunResult::Status::Error);
  CHECK(overflow.code == newbasic::kErrIllegalFunctionCall);
  CHECK(overflow.message.find("2147483647") != std::string::npos);

  const newbasic::RunResult too_large = repl.Process("RENUM 2147483648");
  CHECK(too_large.status == newbasic::RunResult::Status::Error);
  CHECK(too_large.code == newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("ERL: reports a 32-bit line number", "[renum][errors]") {
  CHECK(lwtest::Row0("10 ON ERROR GOTO 2000000001\n"
                     "2000000000 ERROR 5\n"
                     "2000000001 PRINT ERL\n"
                     "2100000000 END\n") == " 2000000000");
}

TEST_CASE("MERGE: lines from a file replace matching line numbers", "[renum]") {
  TempFile file("newbasic_merge.bas");
  file.Write("20 PRINT \"MERGED\"\n40 PRINT \"NEW\"\n");
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"KEPT\"");
  repl.Process("20 PRINT \"ORIGINAL\"");
  repl.Process("30 END");
  const newbasic::RunResult result = repl.Process("MERGE \"" + file.Path() + "\"");
  CHECK(result.status != newbasic::RunResult::Status::Error);
  const std::string rows = AllRows(screen);
  ListProgram(&repl, &screen);
  const std::string listed = AllRows(screen);
  CHECK(listed.find("10 PRINT \"KEPT\"") != std::string::npos);
  CHECK(listed.find("20 PRINT \"MERGED\"") != std::string::npos);
  CHECK(listed.find("ORIGINAL") == std::string::npos);
  CHECK(listed.find("40 PRINT \"NEW\"") != std::string::npos);
  (void)rows;
}

TEST_CASE("AUTO: numbered entry until an empty line", "[auto]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  screen.ScriptLine(L"PRINT \"A\"");
  screen.ScriptLine(L"PRINT \"B\"");
  screen.ScriptLine(L""); // ends the sequence
  repl.Process("AUTO");
  CHECK(repl.Store().Size() == 2);
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "10 PRINT \"A\"");
  CHECK(rows[1] == "20 PRINT \"B\"");
}

TEST_CASE("AUTO: the generated number and typed text share a line", "[auto]") {
  EchoingScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  screen.ScriptLine(L"PRINT \"A\"");
  screen.ScriptLine(L""); // ends the sequence
  repl.Process("AUTO 10,10");
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "10 PRINT \"A\"");
  CHECK(rows[1] == "20");
}

TEST_CASE("AUTO: start and increment arguments", "[auto]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  screen.ScriptLine(L"PRINT \"A\"");
  screen.ScriptLine(L"PRINT \"B\"");
  screen.ScriptLine(L"");
  repl.Process("AUTO 100,5");
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "100 PRINT \"A\"");
  CHECK(rows[1] == "105 PRINT \"B\"");
}

TEST_CASE("AUTO: a line carrying its own number ends the sequence", "[auto]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  screen.ScriptLine(L"PRINT \"A\"");
  screen.ScriptLine(L"99 PRINT \"Z\""); // its own number: enters and ends AUTO
  repl.Process("AUTO 50,50");
  CHECK(repl.Store().Size() == 2);
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "50 PRINT \"A\"");
  CHECK(rows[1] == "99 PRINT \"Z\"");
}

TEST_CASE("RENUM: rewrites every kind of line reference", "[renum]") {
  // The rewriter keys off the word before a number, so each reference word is exercised
  // separately: ON ... GOTO and ON ... GOSUB get their numbers from GOTO/GOSUB, and THEN,
  // ELSE, RESTORE, RESUME and RUN each introduce a number of their own.
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 ON X GOTO 100, 200");
  repl.Process("20 ON X GOSUB 100, 200");
  repl.Process("30 IF X THEN 100 ELSE 200");
  repl.Process("40 RESTORE 100");
  repl.Process("50 RESUME 100");
  repl.Process("60 RUN 100");
  repl.Process("100 RETURN");
  repl.Process("200 RETURN");
  repl.Process("RENUM 1000,,1000");
  const std::vector<std::string> rows = ListProgram(&repl, &screen);
  REQUIRE(rows.size() >= 8);
  CHECK(rows[0] == "1000 ON X GOTO 7000, 8000");
  CHECK(rows[1] == "2000 ON X GOSUB 7000, 8000");
  CHECK(rows[2] == "3000 IF X THEN 7000 ELSE 8000");
  CHECK(rows[3] == "4000 RESTORE 7000");
  CHECK(rows[4] == "5000 RESUME 7000");
  CHECK(rows[5] == "6000 RUN 7000");
  CHECK(rows[6] == "7000 RETURN");
  CHECK(rows[7] == "8000 RETURN");
}
