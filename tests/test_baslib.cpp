// baslib tests: run BASIC host fragments merged with baslib/baslib.bas on a
// headless screen. These cover the executable parts of baslib-imp.md without
// depending on a window or a real pointing device.
#include <catch2/catch_test_macros.hpp>

#include <fstream>
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

TEST_CASE("baslib: init, constants, and labels", "[baslib]") {
  const std::string host = "10 SCREEN 720\n"
                           "20 blReqW% = 640 : blReqH% = 400\n"
                           "30 GOSUB blApiInit\n"
                           "40 PRINT blStatus%\n"
                           "50 PRINT blVersion$; blMaxControls%\n"
                           "60 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0");
  CHECK(Row(screen, 1) == "0.1 255");
}

TEST_CASE("baslib: create, layout, paint, hit-test, and click event", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 blReqW% = 640 : blReqH% = 400\n"
      "30 GOSUB blApiInit\n"
      "40 blReqClass% = 2 : blReqParent% = 1 : blReqX% = 10 : blReqY% = 10\n"
      "45 blReqW% = 300 : blReqH% = 200 : blReqMode% = 1 : blReqPad% = 4 : blReqGap% = 4\n"
      "50 GOSUB blApiCreate : blPanel% = blRetHandle%\n"
      "60 blReqClass% = 3 : blReqParent% = blPanel% : blReqKind% = 1\n"
      "65 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24\n"
      "70 blReqText$ = \"OK\" : blReqAction% = 100 : GOSUB blApiCreate : blButton% = blRetHandle%\n"
      "80 GOSUB blApiPaintAll\n"
      "90 PRINT blStatus%\n"
      "100 blReqX% = 20 : blReqY% = 20 : GOSUB blApiFindAt : PRINT blRetHandle%\n"
      "110 blReqButton% = 1 : GOSUB blApiTestClickAt : PRINT blRetHandle%\n"
      "120 GOSUB blApiNextEvent : PRINT blEventType%; blEventCtl%; blEventAction%\n"
      "130 END\n";
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

TEST_CASE("baslib: action-line dispatch reaches host code", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 blReqW% = 640 : blReqH% = 400 : GOSUB blApiInit\n"
      "30 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 1\n"
      "35 blReqX% = 10 : blReqY% = 10 : blReqW% = 100 : blReqH% = 24 : blReqAction% = 100\n"
      "40 GOSUB blApiCreate\n"
      "50 blReqActionId% = 100 : blReqLine& = 1000 : GOSUB blApiSetActionLine\n"
      "60 blReqX% = 20 : blReqY% = 20 : blReqButton% = 1 : GOSUB blApiTestClickAt\n"
      "70 GOSUB blApiNextEvent\n"
      "80 IF blEventAction% <> blActionNone% THEN GOSUB blActionLine&(blEventAction%)\n"
      "90 END\n"
      "1000 PRINT \"ACTION\"; blEventCtl%\n"
      "1010 RETURN\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == "ACTION 2");
}

TEST_CASE("baslib: destroy, re-init, layout, and host sentinel", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 blReqW% = 640 : blReqH% = 400 : GOSUB blApiInit\n"
      "30 blReqClass% = 2 : blReqParent% = 1 : blReqX% = 10 : blReqY% = 10\n"
      "35 blReqW% = 300 : blReqH% = 200 : blReqMode% = 1 : blReqPad% = 4 : blReqGap% = 4\n"
      "40 GOSUB blApiCreate : blPanel% = blRetHandle%\n"
      "50 blReqClass% = 3 : blReqParent% = blPanel% : blReqKind% = 1\n"
      "55 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24\n"
      "58 GOSUB blApiCreate : GOSUB blApiLayoutAll\n"
      "60 PRINT blCtl%(blRetHandle%,11); blCtl%(blRetHandle%,12)\n"
      "70 blReqHandle% = blPanel% : GOSUB blApiDestroy : PRINT blStatus%\n"
      "80 blReqHandle% = blPanel% : GOSUB blApiSetText : PRINT blStatus%\n"
      "90 STACK PUSH 12345\n"
      "100 GOSUB blApiInit\n"
      "110 STACK POP X% : PRINT X%\n"
      "120 END\n";
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
                           "20 blReqW% = 0 : blReqH% = 0 : GOSUB blApiInit\n"
                           "30 PRINT blCtl%(1,9); blCtl%(1,10)\n"
                           "40 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 320  200");
}

TEST_CASE("baslib: frame depth and balance", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 GOSUB blApiInit\n"
      "30 FOR I% = 1 TO 32\n"
      "40 GOSUB blFramePush\n"
      "50 NEXT I%\n"
      "60 PRINT blFp%; blStatus%\n"
      "70 GOSUB blFramePush : PRINT blFp%; blStatus%\n"
      "80 FOR I% = 1 TO 32\n"
      "90 GOSUB blFrameDrop : STACK POP X#, S$\n"
      "100 NEXT I%\n"
      "110 PRINT blFp%\n"
      "120 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 32  0");
  CHECK(Row(screen, 1) == " 32  6");
  CHECK(Row(screen, 2) == " 0");
}

TEST_CASE("baslib: blSend uses the frame protocol", "[baslib]") {
  const std::string host = "10 SCREEN 720\n"
                           "20 GOSUB blApiInit\n"
                           "30 blCallCtl% = 1 : blCallVerb% = 1\n"
                           "40 GOSUB blSend\n"
                           "50 PRINT blStatus%; blFp%\n"
                           "60 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0  0");
}

TEST_CASE("baslib: 255-control ceiling", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 GOSUB blApiInit\n"
      "30 FOR I% = 2 TO 255\n"
      "40 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 1\n"
      "45 blReqX% = 0 : blReqY% = 0 : blReqW% = 1 : blReqH% = 1\n"
      "50 GOSUB blApiCreate\n"
      "60 NEXT I%\n"
      "70 PRINT blStatus%; blRetHandle%\n"
      "80 GOSUB blApiCreate : PRINT blStatus%; blRetHandle%\n"
      "90 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0  255");
  CHECK(Row(screen, 1) == " 2  0");
}

TEST_CASE("baslib: v0.2 value controls", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 blReqW% = 640 : blReqH% = 400 : GOSUB blApiInit\n"
      "30 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 2\n"
      "40 blReqX% = 10 : blReqY% = 10 : blReqW% = 200 : blReqH% = 20\n"
      "50 GOSUB blApiCreate : blSlider% = blRetHandle%\n"
      "60 blReqHandle% = blSlider% : blReqValue# = 42 : GOSUB blApiSetValue\n"
      "70 GOSUB blApiGetValue : PRINT blRetValue#\n"
      "80 GOSUB blApiPaintAll : PRINT blStatus%\n"
      "90 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 3\n"
      "95 blReqX% = 10 : blReqY% = 40 : blReqW% = 200 : blReqH% = 20 : GOSUB blApiCreate\n"
      "100 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 4\n"
      "105 blReqX% = 10 : blReqY% = 70 : blReqW% = 40 : blReqH% = 40 : GOSUB blApiCreate\n"
      "110 GOSUB blApiPaintAll : PRINT blStatus%\n"
      "120 END\n";
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
      "20 blReqW% = 640 : blReqH% = 400 : GOSUB blApiInit\n"
      "30 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 2\n"
      "35 blReqX% = 10 : blReqY% = 10 : blReqW% = 100 : blReqH% = 20\n"
      "40 GOSUB blApiCreate : blSlider% = blRetHandle% : GOSUB blApiLayoutAll\n"
      "50 blReqX% = 20 : blReqY% = 20 : GOSUB blApiTestMouseMove\n"
      "60 blReqX% = 20 : blReqY% = 20 : blReqButton% = 1 : blReqDown% = 1 : GOSUB blApiTestMouseButton\n"
      "70 blReqX% = 60 : blReqY% = 20 : GOSUB blApiTestMouseMove\n"
      "80 blReqX% = 60 : blReqY% = 20 : blReqButton% = 1 : blReqDown% = 0 : GOSUB blApiTestMouseButton\n"
      "90 PRINT blUser#(blSlider%,2)\n"
      "100 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 50");
}

TEST_CASE("baslib: dynamic class registration", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 GOSUB blApiInit\n"
      "30 blReqClass% = 5 : blReqLine& = 500990 : blReqText$ = \"Extra\"\n"
      "40 GOSUB blApiRegisterClass\n"
      "50 PRINT blStatus%; blMethod&(5,1)\n"
      "60 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0  500990");
}

TEST_CASE("baslib: paint preserves the host viewport", "[baslib]") {
  const std::string host =
      "10 SCREEN 720\n"
      "20 VIEW (100,100)-(500,300)\n"
      "30 blReqW% = 640 : blReqH% = 400 : GOSUB blApiInit\n"
      "40 blReqClass% = 2 : blReqParent% = 1 : blReqX% = 10 : blReqY% = 10\n"
      "45 blReqW% = 100 : blReqH% = 50 : GOSUB blApiCreate\n"
      "50 GOSUB blApiPaintAll\n"
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

TEST_CASE("baslib: labels survive RENUM", "[baslib]") {
  const std::string host = "10 SCREEN 720\n"
                           "20 GOSUB blApiInit\n"
                           "30 PRINT blStatus%\n"
                           "40 END\n";
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result;
  REQUIRE(RunHost(host, &screen, &result, true));
  CHECK(result.status == newbasic::RunResult::Status::Done);
  CHECK(Row(screen, 0) == " 0");
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
