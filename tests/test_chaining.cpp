// M10: in-program SAVE/LOAD and CHAIN/COMMON. These touch real files, so names are
// unique per run and cleaned up by RAII.
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "lwbasic/errors.h"
#include "lwbasic/repl.h"
#include "lwbasic/screen/headless_screen.h"
#include "test_helpers.h"

namespace {

std::string TempPath() {
  static int counter = 0;
  return "lwbasic_chain_" + std::to_string(++counter) + ".bas";
}

struct TempFile {
  explicit TempFile(std::string path) : path_(std::move(path)) {}
  ~TempFile() { std::remove(path_.c_str()); }
  const std::string& Path() const { return path_; }

  void Write(const std::string& text) const {
    std::ofstream out(path_, std::ios::binary);
    out << text;
  }

  std::string Read() const {
    std::ifstream in(path_, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
  }

private:
  std::string path_;
};

std::string AllRows(const lwbasic::screen::HeadlessScreen& screen) {
  std::string all;
  for (const std::string& row : screen.RenderRows())
    all += row + "\n";
  return all;
}

} // namespace

TEST_CASE("M10: in-program SAVE writes the program text", "[m10]") {
  TempFile file(TempPath());
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SAVE \"" + file.Path() + "\"");
  repl.Process("20 END");
  repl.Process("RUN");
  const std::string saved = file.Read();
  // The file holds the stored program as text (a tab follows each line number), so
  // it still contains both statements.
  INFO("saved: [" << saved << "]");
  CHECK(saved.find("SAVE") != std::string::npos);
  CHECK(saved.find("END") != std::string::npos);
}

TEST_CASE("M10: in-program LOAD replaces the program and clears variables", "[m10]") {
  TempFile other(TempPath());
  other.Write("10 PRINT A\n");
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  // A is set, then LOAD replaces the program; the loaded program prints A, which
  // must be 0 because LOAD starts clean.
  repl.Process("10 A=5");
  repl.Process("20 LOAD \"" + other.Path() + "\"");
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == " 0");
}

TEST_CASE("M10: CHAIN keeps only the COMMON variables", "[m10]") {
  TempFile other(TempPath());
  other.Write("10 PRINT A;B\n");
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 A=5: B=6");
  repl.Process("20 COMMON A");
  repl.Process("30 CHAIN \"" + other.Path() + "\"");
  screen.Clear();
  repl.Process("RUN");
  // A was COMMON so it survives; B was not, so it reads as 0.
  CHECK(screen.RenderRows()[0] == " 5  0");
}

TEST_CASE("M10: CHAIN with a line number starts there", "[m10]") {
  TempFile other(TempPath());
  other.Write("10 PRINT \"FIRST\"\n20 PRINT \"SECOND\"\n");
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 CHAIN \"" + other.Path() + "\", 20");
  screen.Clear();
  repl.Process("RUN");
  const std::string all = AllRows(screen);
  CHECK(all.find("FIRST") == std::string::npos); // skipped
  CHECK(all.find("SECOND") != std::string::npos);
}

TEST_CASE("M10: CHAIN to a missing file is File not found", "[m10]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 CHAIN \"no_such_program.bas\"");
  // Process returns the run result; a direct call bypasses the REPL print path, so
  // the error is read here rather than from the screen.
  const lwbasic::RunResult result = repl.Process("RUN");
  CHECK(result.status == lwbasic::RunResult::Status::Error);
  CHECK(result.code == lwbasic::kErrFileNotFound);
}

TEST_CASE("M10: MERGE inside a program is Illegal direct", "[m10]") {
  CHECK(lwtest::RunErrorCode("10 MERGE \"x\"\n") == lwbasic::kErrIllegalDirect);
}

TEST_CASE("COMMON of an array reports Advanced Feature rather than dropping it", "[m10]") {
  // COMMON D() names the whole array, which a CHAIN does not carry yet. It must say so: a silent
  // drop would leave the chained program short of data with no clue why.
  CHECK(lwtest::RunErrorCode("10 COMMON D()\n20 END\n") == lwbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 COMMON A, D()\n20 END\n") == lwbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 COMMON D(5)\n20 END\n") == lwbasic::kErrAdvancedFeature);
  // A scalar COMMON is still fine: only the array form is refused.
  CHECK(lwtest::RunErrorCode("10 COMMON A, B$\n20 END\n") == 0);
}
