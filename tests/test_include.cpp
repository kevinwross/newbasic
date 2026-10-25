// INCLUDE: compile-time source inclusion of numbered .BAS files.
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/frontend.h"
#include "newbasic/host.h"
#include "newbasic/line_store.h"
#include "newbasic/program_files.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"

namespace {

namespace fs = std::filesystem;

struct TempDir {
  TempDir() {
    static int counter = 0;
    std::error_code error;
    path = std::filesystem::current_path(error);
    if (error)
      path = ".";
    path /= "newbasic_include_" + std::to_string(++counter);
    std::filesystem::remove_all(path, error);
    std::filesystem::create_directories(path, error);
    if (error)
      FAIL("cannot create INCLUDE test directory: " << path.string());
  }
  ~TempDir() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
  fs::path path;
};

void WriteFile(const fs::path& path, const std::string& text) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  REQUIRE(out.good());
  out << text;
  REQUIRE(out.good());
}

bool Load(const fs::path& path, newbasic::LineStore* store, int* code,
          std::string* error) {
  return newbasic::LoadProgramFromFile(path.string(), store, code, error);
}

std::vector<std::string> Run(const newbasic::LineStore& store) {
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(newbasic::Compile(store, &program, &error));
  newbasic::screen::HeadlessScreen screen;
  newbasic::Vm vm(screen);
  vm.Load(std::move(program));
  const newbasic::RunResult result = vm.Run();
  INFO("INCLUDE program result " << result.code << ": " << result.message);
  REQUIRE(result.status == newbasic::RunResult::Status::Done);
  return screen.RenderRows();
}

} // namespace

TEST_CASE("INCLUDE merges a numbered library and keeps running", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 VALUE%=42\n1010 RETURN\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 INCLUDE \"LIB.BAS\"\n"
            "20 GOSUB 1000\n"
            "30 PRINT VALUE%\n"
            "40 END\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  REQUIRE(Load(dir.path / "MAIN.BAS", &store, &code, &error));
  CHECK(store.FindIndex(10).has_value()); // the include line is a REM placeholder
  CHECK(store.FindIndex(1000).has_value());
  CHECK(store.FindIndex(1010).has_value());

  const std::vector<std::string> rows = Run(store);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == " 42");
}

TEST_CASE("INCLUDE resolves nested files relative to each includer", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "sub" / "LIB.BAS",
            "1000 INCLUDE \"HELPER.BAS\"\n"
            "1010 GOSUB 2000\n"
            "1020 RETURN\n");
  WriteFile(dir.path / "sub" / "HELPER.BAS",
            "2000 VALUE%=7\n"
            "2010 RETURN\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 INCLUDE \"sub\\LIB.BAS\" ' nested\n"
            "20 GOSUB 1000\n"
            "30 PRINT VALUE%\n"
            "40 END\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  REQUIRE(Load(dir.path / "MAIN.BAS", &store, &code, &error));

  const std::vector<std::string> rows = Run(store);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == " 7");
}

TEST_CASE("INCLUDE is include-once per compilation", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 VALUE%=5\n1010 RETURN\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 INCLUDE \"LIB.BAS\"\n"
            "20 INCLUDE \"LIB.BAS\"\n"
            "30 GOSUB 1000\n"
            "40 PRINT VALUE%\n"
            "50 END\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  REQUIRE(Load(dir.path / "MAIN.BAS", &store, &code, &error));

  const std::vector<std::string> rows = Run(store);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == " 5");
}

TEST_CASE("INCLUDE rejects duplicate line numbers", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 PRINT \"LIB\"\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 INCLUDE \"LIB.BAS\"\n"
            "1000 PRINT \"MAIN\"\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  CHECK_FALSE(Load(dir.path / "MAIN.BAS", &store, &code, &error));
  CHECK(code == newbasic::kErrSyntax);
  CHECK(error.find("line number conflict") != std::string::npos);
}

TEST_CASE("INCLUDE reports missing files and cycles", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "MISSING.BAS", "10 INCLUDE \"NOPE.BAS\"\n");
  WriteFile(dir.path / "A.BAS", "10 INCLUDE \"B.BAS\"\n");
  WriteFile(dir.path / "B.BAS", "20 INCLUDE \"A.BAS\"\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  CHECK_FALSE(Load(dir.path / "MISSING.BAS", &store, &code, &error));
  CHECK(code == newbasic::kErrFileNotFound);

  error.clear();
  CHECK_FALSE(Load(dir.path / "A.BAS", &store, &code, &error));
  CHECK(code == newbasic::kErrSyntax);
  INFO("cycle error: " << error);
  CHECK(error.find("cycle") != std::string::npos);
}

TEST_CASE("INCLUDE ignores strings and rejects malformed directives", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "STR.BAS", "10 PRINT \"INCLUDE \"\"NOPE.BAS\"\"\"\n");
  WriteFile(dir.path / "BAD.BAS", "10 INCLUDE FILE$\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  REQUIRE(Load(dir.path / "STR.BAS", &store, &code, &error));
  const std::vector<std::string> rows = Run(store);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "INCLUDE \"NOPE.BAS\"");

  error.clear();
  CHECK_FALSE(Load(dir.path / "BAD.BAS", &store, &code, &error));
  CHECK(code == newbasic::kErrSyntax);
  CHECK(error.find("quoted file name") != std::string::npos);
}

TEST_CASE("INCLUDE works through the REPL LOAD path", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 VALUE%=11\n1010 RETURN\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 INCLUDE \"LIB.BAS\"\n"
            "20 GOSUB 1000\n"
            "30 PRINT VALUE%\n"
            "40 END\n");

  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  CHECK(repl.Process("LOAD \"" + (dir.path / "MAIN.BAS").generic_string() + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(!rows.empty());
  CHECK(rows[0] == " 11");
}

TEST_CASE("INCLUDE works through CHAIN", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 PRINT \"CHAINED\"\n1010 RETURN\n");
  WriteFile(dir.path / "OTHER.BAS",
            "10 INCLUDE \"LIB.BAS\"\n"
            "20 GOSUB 1000\n"
            "30 END\n");
  WriteFile(dir.path / "MAIN.BAS",
            "10 CHAIN \"" + (dir.path / "OTHER.BAS").generic_string() + "\"\n");

  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  CHECK(repl.Process("LOAD \"" + (dir.path / "MAIN.BAS").generic_string() + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "CHAINED");
}

TEST_CASE("INCLUDE works for program text with an explicit base directory", "[include]") {
  TempDir dir;
  WriteFile(dir.path / "LIB.BAS", "1000 VALUE%=9\n1010 RETURN\n");

  newbasic::LineStore store;
  int code = 0;
  std::string error;
  REQUIRE(newbasic::LoadProgramFromText(
      "10 INCLUDE \"LIB.BAS\"\n"
      "20 GOSUB 1000\n"
      "30 PRINT VALUE%\n"
      "40 END\n",
      dir.path.string(), &store, &code, &error));

  const std::vector<std::string> rows = Run(store);
  REQUIRE(!rows.empty());
  CHECK(rows[0] == " 9");
}
