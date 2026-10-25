// baslib tests: run BASIC host fragments merged with baslib/baslib.bas on a
// headless screen. The public baslib API is entirely parameterized:
// SUB calls and FUNCTION expressions, with no request globals.
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "newbasic/frontend.h"
#include "newbasic/line_store.h"
#include "newbasic/renum.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"

#ifndef NEWBASIC_BASLIB_DIR
#error "NEWBASIC_BASLIB_DIR must point at the baslib source directory"
#endif

namespace {

std::string ReadAll(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.good());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

bool RunHost(const std::string& host, newbasic::screen::HeadlessScreen* screen,
             newbasic::RunResult* result, bool renumber = false) {
  std::string source = host;
  source += "\n";
  source += ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas");

  newbasic::LineStore store;
  std::string parse_error;
  if (!newbasic::LineStore::ParseTextBas(source, &store, &parse_error)) {
    INFO("parse failed: " << parse_error);
    return false;
  }
  if (renumber) {
    const newbasic::RenumOutcome outcome = newbasic::RenumberProgram(&store, 600000, 0, 10);
    if (!outcome.ok) {
      INFO("RENUM failed: " << outcome.message);
      return false;
    }
  }

  newbasic::Program program;
  newbasic::CompileError error;
  if (!newbasic::Compile(store, &program, &error)) {
    INFO("compile failed at line " << error.line << ", code " << error.code << ": "
                                   << error.message);
    return false;
  }
  newbasic::Vm vm(*screen);
  vm.Load(std::move(program));
  *result = vm.Run();
  return true;
}

std::string Row(const newbasic::screen::HeadlessScreen& screen, int row) {
  const std::vector<std::string> rows = screen.RenderRows();
  return row >= 0 && row < static_cast<int>(rows.size()) ? rows[static_cast<std::size_t>(row)]
                                                         : std::string();
}

} // namespace

TEST_CASE("baslib: init, constants, and public procedures", "[baslib]") {
  const std::string host = "10 SCREEN 720\n"
                           "20 CALL blInit(640, 400)\n"
                           "30 PRINT blStatus%\n"
                           "40 PRINT blVersion$; blMaxControls%\n"
                           "50 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0");
  CHECK(Row(screen, 1) == "0.3 255");
}

TEST_CASE("baslib: create, layout, paint, hit-test, and click event", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 P% = blCreate%(1, 2, 0, 10, 10, 300, 200, \"\", 0, 0, 0)\n"
      "40 CALL blSetLayout(P%, 1, 4, 4)\n"
      "50 B% = blCreate%(P%, 3, 1, 0, 0, 100, 24, \"OK\", 100, 0, 0)\n"
      "60 CALL blPaintAll\n"
      "70 PRINT blStatus%\n"
      "80 PRINT blFindAt%(20, 20)\n"
      "90 PRINT blTestClickAt%(20, 20, 1)\n"
      "100 T% = blNextEvent%()\n"
      "110 PRINT T%; blEventCtl%(); blEventAction%()\n"
      "120 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0");
  CHECK(Row(screen, 1) == " 3");
  CHECK(Row(screen, 2) == " 3");
  CHECK(Row(screen, 3) == " 1  3  100");
  CHECK(screen.Graphics().Pixel(15, 15) == 7);
}

TEST_CASE("baslib: bounds, text, action, visibility, and enabled setters", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 B% = blCreate%(1, 3, 1, 10, 10, 100, 24, \"A\", 0, 0, 0)\n"
      "40 CALL blSetBounds(B%, 20, 20, 120, 28)\n"
      "50 CALL blSetText(B%, \"B\")\n"
      "60 CALL blSetAction(B%, 7)\n"
      "70 CALL blLayoutAll\n"
      "80 PRINT blCtl%(B%,7); blCtl%(B%,8); blGetText$(B%); blCtl%(B%,15)\n"
      "90 CALL blHide(B%) : PRINT blFindAt%(25, 25)\n"
      "100 CALL blShow(B%) : PRINT blFindAt%(25, 25)\n"
      "110 CALL blSetEnabled(B%, 0) : PRINT blFindAt%(25, 25)\n"
      "120 CALL blSetEnabled(B%, 1) : PRINT blFindAt%(25, 25)\n"
      "130 CALL blInvalidate(B%) : CALL blUpdate(B%)\n"
      "140 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 20  20 B 7");
  CHECK(Row(screen, 1) == " 1");
  CHECK(Row(screen, 2) == " 2");
  CHECK(Row(screen, 3) == " 1");
  CHECK(Row(screen, 4) == " 2");
}

TEST_CASE("baslib: action-line dispatch reaches host code", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 B% = blCreate%(1, 3, 1, 10, 10, 100, 24, \"GO\", 100, 0, 0)\n"
      "40 CALL blSetActionLine(100, 1000)\n"
      "50 H% = blTestClickAt%(20, 20, 1)\n"
      "60 PRINT blEventCount%()\n"
      "70 T% = blNextEvent%()\n"
      "80 IF blEventAction%() <> blActionNone% THEN GOSUB blActionLine&(blEventAction%())\n"
      "90 END\n"
      "1000 PRINT \"ACTION\"; blEventCtl%()\n"
      "1010 RETURN\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 1");
  CHECK(Row(screen, 1) == "ACTION 2");
}

TEST_CASE("baslib: destroy, re-init, layout, and host STACK", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 P% = blCreate%(1, 2, 0, 10, 10, 300, 200, \"\", 0, 0, 0)\n"
      "40 CALL blSetLayout(P%, 1, 4, 4)\n"
      "50 B% = blCreate%(P%, 3, 1, 0, 0, 100, 24, \"\", 0, 0, 0)\n"
      "60 CALL blLayoutAll\n"
      "70 PRINT blCtl%(B%,11); blCtl%(B%,12)\n"
      "80 CALL blDestroy(P%) : PRINT blStatus%\n"
      "90 CALL blSetText(P%, \"x\") : PRINT blStatus%\n"
      "100 STACK PUSH 12345\n"
      "110 CALL blInit(0, 0)\n"
      "120 STACK POP X% : PRINT X%\n"
      "130 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 14  14");
  CHECK(Row(screen, 1) == " 0");
  CHECK(Row(screen, 2) == " 3");
  CHECK(Row(screen, 3) == " 12345");
}

TEST_CASE("baslib: root uses the current canvas size", "[baslib]") {
  const std::string host = "10 SCREEN 13\n"
                           "20 CALL blInit(0, 0)\n"
                           "30 PRINT blCtl%(1,9); blCtl%(1,10)\n"
                           "40 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 320  200");
}

TEST_CASE("baslib: nested layout uses per-call locals", "[baslib]") {
  // A horizontal panel inside a vertical panel used to let the inner layout
  // clobber the outer cursor, so the sibling after it was misplaced. The
  // SUB/LOCAL rewrite must keep each layout activation independent.
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 M% = blCreate%(1, 2, 0, 10, 10, 400, 300, \"\", 0, 0, 0)\n"
      "40 CALL blSetLayout(M%, 1, 4, 4)\n"
      "50 R% = blCreate%(M%, 2, 0, 0, 0, 100, 100, \"\", 0, 0, 0)\n"
      "60 CALL blSetLayout(R%, 2, 2, 2)\n"
      "70 B% = blCreate%(R%, 3, 1, 0, 0, 60, 20, \"\", 0, 0, 0)\n"
      "80 S% = blCreate%(M%, 3, 2, 0, 0, 60, 20, \"\", 0, 0, 0)\n"
      "90 CALL blLayoutAll\n"
      "100 PRINT blCtl%(R%,11); blCtl%(R%,12); blCtl%(R%,9); blCtl%(R%,10)\n"
      "110 PRINT blCtl%(B%,11); blCtl%(B%,12); blCtl%(B%,9); blCtl%(B%,10)\n"
      "120 PRINT blCtl%(S%,11); blCtl%(S%,12); blCtl%(S%,9); blCtl%(S%,10)\n"
      "130 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 14  14  392  100");
  CHECK(Row(screen, 1) == " 16  16  60  96");
  CHECK(Row(screen, 2) == " 14  118  392  20");
}

TEST_CASE("baslib: 255-control ceiling", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 FOR I% = 2 TO 255\n"
      "40 H% = blCreate%(1, 3, 1, 0, 0, 1, 1, \"\", 0, 0, 0)\n"
      "50 NEXT I%\n"
      "60 PRINT blStatus%; H%\n"
      "70 X% = blCreate%(1, 3, 1, 0, 0, 1, 1, \"\", 0, 0, 0)\n"
      "80 PRINT blStatus%; X%\n"
      "90 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0  255");
  CHECK(Row(screen, 1) == " 2  0");
}

TEST_CASE("baslib: value controls use parameterized getters and setters", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 S% = blCreate%(1, 3, 2, 10, 10, 200, 20, \"\", 0, 0, 0)\n"
      "40 CALL blSetValue(S%, 42)\n"
      "50 PRINT blGetValue#(S%)\n"
      "60 CALL blPaintAll : PRINT blStatus%\n"
      "70 G% = blCreate%(1, 3, 3, 10, 40, 200, 20, \"\", 0, 0, 0)\n"
      "80 D% = blCreate%(1, 3, 4, 10, 70, 40, 40, \"\", 0, 0, 0)\n"
      "90 CALL blPaintAll : PRINT blStatus%\n"
      "100 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 42");
  CHECK(Row(screen, 1) == " 0");
  CHECK(Row(screen, 2) == " 0");
}

TEST_CASE("baslib: slider drag updates its value", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 S% = blCreate%(1, 3, 2, 10, 10, 100, 20, \"\", 0, 0, 0)\n"
      "40 CALL blLayoutAll\n"
      "50 CALL blTestMouseMove(20, 20)\n"
      "60 CALL blTestMouseButton(1, 1, 20, 20)\n"
      "70 CALL blTestMouseMove(60, 20)\n"
      "80 CALL blTestMouseButton(1, 0, 60, 20)\n"
      "90 PRINT blUser#(S%,2)\n"
      "100 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 50");
}

TEST_CASE("baslib: hover, press, release, and wheel events", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 B% = blCreate%(1, 3, 1, 10, 10, 100, 24, \"\", 0, 0, 0)\n"
      "40 CALL blTestMouseMove(20, 20)\n"
      "50 PRINT blNextEvent%()\n"
      "60 PRINT blNextEvent%()\n"
      "70 CALL blTestMouseButton(1, 1, 20, 20)\n"
      "80 PRINT blNextEvent%()\n"
      "90 CALL blTestMouseButton(1, 0, 20, 20)\n"
      "100 PRINT blNextEvent%()\n"
      "110 CALL blTestWheel(20, 20, 3)\n"
      "120 PRINT blNextEvent%(); blEventWheel%()\n"
      "130 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 4");
  CHECK(Row(screen, 1) == " 3");
  CHECK(Row(screen, 2) == " 6");
  CHECK(Row(screen, 3) == " 7");
  CHECK(Row(screen, 4) == " 8  3");
}

TEST_CASE("baslib: ERL reports a 500000+ library line", "[baslib]") {
  const std::string host =
      "10 ON ERROR GOTO 900\n"
      "20 SCREEN 720\n"
      "30 CALL blInit(640, 400)\n"
      "40 GOSUB blTestErl\n"
      "50 END\n"
      "900 PRINT \"ERR\"; ERR; \"ERL\"; ERL\n"
      "910 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == "ERR 5 ERL 512020");
}

TEST_CASE("baslib: dynamic class registration", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 CALL blRegisterClass(5, 502999, \"Extra\")\n"
      "40 PRINT blStatus%; blMethod&(5,1)\n"
      "50 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0  502999");
}

TEST_CASE("baslib: blUpdateAll repaints only dirty subtrees", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 CALL blInit(640, 400)\n"
      "30 P1% = blCreate%(1, 2, 0, 10, 10, 300, 200, \"\", 0, 0, 0)\n"
      "40 CALL blSetLayout(P1%, 1, 4, 4)\n"
      "50 B1% = blCreate%(P1%, 3, 1, 0, 0, 100, 24, \"A\", 0, 0, 0)\n"
      "60 P2% = blCreate%(1, 2, 0, 400, 10, 100, 100, \"\", 0, 0, 0)\n"
      "70 CALL blPaintAll\n"
      "80 PSET (12,12), 3\n"
      "90 PSET (402,12), 3\n"
      "100 CALL blUpdateAll\n"
      "110 PRINT POINT(12,12); POINT(402,12)\n"
      "120 CALL blSetText(B1%, \"B\")\n"
      "130 CALL blUpdateAll\n"
      "140 PRINT POINT(12,12); POINT(402,12)\n"
      "150 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 3  3");
  CHECK(Row(screen, 1) == " 7  3");
}

TEST_CASE("baslib: paint preserves the host viewport", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 VIEW (100,100)-(500,300)\n"
      "30 CALL blInit(640, 400)\n"
      "40 P% = blCreate%(1, 2, 0, 10, 10, 100, 50, \"\", 0, 0, 0)\n"
      "50 CALL blPaintAll\n"
      "60 PRINT VIEWX1; VIEWY1; VIEWX2; VIEWY2\n"
      "70 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 100  100  500  300");
}

TEST_CASE("baslib: demo.bas compiles", "[baslib]") {
  std::string source = ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/demo.bas");
  source += "\n";
  source += ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas");
  newbasic::LineStore store;
  std::string parse_error;
  REQUIRE(newbasic::LineStore::ParseTextBas(source, &store, &parse_error));
  newbasic::Program program;
  newbasic::CompileError error;
  CHECK(newbasic::Compile(store, &program, &error));
  if (error.code != 0)
    INFO("demo compile error at " << error.line << ": " << error.message);
}

TEST_CASE("baslib: demo-controls.bas compiles", "[baslib]") {
  std::string source = ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/demo-controls.bas");
  source += "\n";
  source += ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas");
  newbasic::LineStore store;
  std::string parse_error;
  REQUIRE(newbasic::LineStore::ParseTextBas(source, &store, &parse_error));
  newbasic::Program program;
  newbasic::CompileError error;
  CHECK(newbasic::Compile(store, &program, &error));
  if (error.code != 0)
    INFO("demo-controls compile error at " << error.line << ": " << error.message);
}

TEST_CASE("baslib: samplehost.bas compiles", "[baslib]") {
  std::string source = ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/samplehost.bas");
  source += "\n";
  source += ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas");
  newbasic::LineStore store;
  std::string parse_error;
  REQUIRE(newbasic::LineStore::ParseTextBas(source, &store, &parse_error));
  newbasic::Program program;
  newbasic::CompileError error;
  CHECK(newbasic::Compile(store, &program, &error));
  if (error.code != 0)
    INFO("samplehost compile error at " << error.line << ": " << error.message);
}

TEST_CASE("baslib: public procedure names survive RENUM", "[baslib]") {
  const std::string host = "10 SCREEN 720\n"
                           "20 CALL blInit(640, 400)\n"
                           "30 PRINT blStatus%\n"
                           "40 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result, true));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0");
}

TEST_CASE("baslib: namespace, suffix, and no request-global audit", "[baslib]") {
  std::istringstream input(ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas"));
  std::string line;
  std::set<std::string> labels;
  std::set<std::string> procedures;
  std::vector<std::string> lines;
  const std::regex label_re(R"(^(\d+)\s+LABEL\s+([A-Za-z_][A-Za-z0-9_]*))");
  const std::regex proc_re(
      R"(^(\d+)\s+(?:SUB|FUNCTION)\s+([A-Za-z_][A-Za-z0-9_]*[%#$&]?)(?:\s*\(.*\))?\s*$)");
  while (std::getline(input, line)) {
    lines.push_back(line);
    std::smatch match;
    if (std::regex_match(line, match, label_re)) {
      labels.insert(match[2].str());
    } else if (std::regex_match(line, match, proc_re)) {
      procedures.insert(match[2].str());
    }
  }

  CHECK(labels.count("blApiCreate") == 0);
  CHECK(procedures.count("blCreate%") == 1);
  CHECK(procedures.count("blSetValue") == 1);
  CHECK(procedures.count("blGetValue#") == 1);

  const std::regex token_re(R"(\b(bl[A-Za-z0-9_]+)([%#$&]?))");
  for (const std::string& source_line : lines) {
    const std::size_t rem = source_line.find("REM");
    const std::string code = rem == std::string::npos ? source_line : source_line.substr(0, rem);
    for (std::sregex_iterator it(code.begin(), code.end(), token_re), end; it != end; ++it) {
      const std::string name = (*it)[1].str();
      const std::string suffix = (*it)[2].str();
      if (name.rfind("blReq", 0) == 0 || name.rfind("blRet", 0) == 0) {
        INFO("request/result global remains: " << name << " in: " << source_line);
        CHECK(false);
      }
      if (suffix.empty() && labels.count(name) == 0 && procedures.count(name) == 0) {
        INFO("unsuffixed baslib token: " << name << " in: " << source_line);
        CHECK(false);
      }
    }
  }

  const std::regex upper_re(R"(\bBl)");
  for (const std::string& source_line : lines) {
    if (std::regex_search(source_line, upper_re)) {
      INFO("uppercase Bl identifier in: " << source_line);
      CHECK(false);
    }
  }
}

TEST_CASE("baslib: source stays inside its line block", "[baslib]") {
  std::istringstream input(ReadAll(std::string(NEWBASIC_BASLIB_DIR) + "/baslib.bas"));
  std::string line;
  int count = 0;
  while (std::getline(input, line)) {
    if (line.empty())
      continue;
    const std::size_t space = line.find(' ');
    REQUIRE(space != std::string::npos);
    const int number = std::stoi(line.substr(0, space));
    CHECK(number >= 500000);
    CHECK(number <= 519999);
    ++count;
  }
  CHECK(count > 0);
}
