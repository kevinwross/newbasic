// Tests for the 2026-10-24 language additions: computed GOTO/GOSUB, live mouse
// state, DRAWTEXT metrics, screen/view queries, PUSH/POP VIEW, SLEEP, and
// SELECT CASE.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "newbasic/frontend.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"
#include "test_helpers.h"

using newbasic::screen::HeadlessScreen;

namespace {

void RunOn(HeadlessScreen* screen, const std::string& source, newbasic::Host* host = nullptr) {
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource(source, &program, &error));
  newbasic::Vm vm(*screen, host);
  vm.Load(std::move(program));
  const newbasic::RunResult result = vm.Run();
  INFO("error " << result.code << " in line " << result.line << ": " << result.message);
  REQUIRE(result.status != newbasic::RunResult::Status::Error);
}

class MouseScreen : public HeadlessScreen {
public:
  int x = 40;
  int y = 30;
  int buttons = 5;
  int wheel = 3;

  int MousePositionX() const override { return x; }
  int MousePositionY() const override { return y; }
  int MouseButtons() const override { return buttons; }
  int TakeMouseWheel() override {
    const int value = wheel;
    wheel = 0;
    return value;
  }
};

} // namespace

TEST_CASE("Computed GOSUB: variable and expression targets", "[computed-gosub]") {
  const std::string source = "10 T% = 200\n"
                             "20 GOSUB T%\n"
                             "30 PRINT \"BACK\"\n"
                             "40 END\n"
                             "200 PRINT \"SUB\"\n"
                             "210 RETURN\n";
  const std::vector<std::string> rows = lwtest::RunRows(source);
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "SUB");
  CHECK(rows[1] == "BACK");

  CHECK(lwtest::Row0("10 GOSUB 50 + 150\n"
                     "20 PRINT \"BACK\"\n"
                     "30 END\n"
                     "200 PRINT \"SUB\"\n"
                     "210 RETURN\n") == "SUB");
}

TEST_CASE("Computed GOSUB: IF THEN GOSUB expression", "[computed-gosub]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 T% = 200 : X = 1\n"
                                                        "20 IF X THEN GOSUB T%\n"
                                                        "30 PRINT \"BACK\"\n"
                                                        "40 END\n"
                                                        "200 PRINT \"SUB\"\n"
                                                        "210 RETURN\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "SUB");
  CHECK(rows[1] == "BACK");
}

TEST_CASE("Computed GOSUB: false IF branch does not evaluate the target", "[computed-gosub]") {
  CHECK(lwtest::Row0("10 T% = 999\n"
                     "20 IF 0 THEN GOSUB T%\n"
                     "30 PRINT \"OK\"\n") == "OK");
}

TEST_CASE("Computed GOSUB: undefined and non-numeric targets", "[computed-gosub]") {
  // The literal form still reports the compile-time/load-time undefined line.
  CHECK(lwtest::RunErrorCode("10 GOSUB 999\n") == newbasic::kErrUndefinedLineNumber);
  // The computed form resolves at run time.
  CHECK(lwtest::RunErrorCode("10 T% = 999\n20 GOSUB T%\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(lwtest::RunErrorCode("10 GOSUB \"x\"\n") == newbasic::kErrTypeMismatch);
}

TEST_CASE("Computed GOTO: variable, expression, and IF THEN targets", "[computed-goto]") {
  CHECK(lwtest::Row0("10 T% = 200\n"
                     "20 GOTO T%\n"
                     "30 PRINT \"SKIP\"\n"
                     "40 END\n"
                     "200 PRINT \"TARGET\"\n"
                     "210 END\n") == "TARGET");

  CHECK(lwtest::Row0("10 GOTO 50 + 150\n"
                     "20 PRINT \"SKIP\"\n"
                     "30 END\n"
                     "200 PRINT \"TARGET\"\n"
                     "210 END\n") == "TARGET");

  CHECK(lwtest::Row0("10 T% = 200 : X = 1\n"
                     "20 IF X THEN GOTO T%\n"
                     "30 PRINT \"SKIP\"\n"
                     "40 END\n"
                     "200 PRINT \"TARGET\"\n"
                     "210 END\n") == "TARGET");

  CHECK(lwtest::Row0("10 T% = 999\n"
                     "20 IF 0 THEN GOTO T%\n"
                     "30 PRINT \"OK\"\n") == "OK");
}

TEST_CASE("Computed GOTO: undefined and non-numeric targets", "[computed-goto]") {
  CHECK(lwtest::RunErrorCode("10 GOTO 999\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(lwtest::RunErrorCode("10 T% = 999\n20 GOTO T%\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(lwtest::RunErrorCode("10 GOTO \"x\"\n") == newbasic::kErrTypeMismatch);
}

TEST_CASE("SELECT CASE: value lists, ELSE, IS, and strings", "[select-case]") {
  CHECK(lwtest::Row0("10 X = 2\n"
                     "20 SELECT CASE X\n"
                     "30 CASE 1\n"
                     "40 PRINT \"ONE\"\n"
                     "50 CASE 2, 3\n"
                     "60 PRINT \"TWO\"\n"
                     "70 CASE ELSE\n"
                     "80 PRINT \"OTHER\"\n"
                     "90 END SELECT\n") == "TWO");

  CHECK(lwtest::Row0("10 X = 7\n"
                     "20 SELECT CASE X\n"
                     "30 CASE IS > 5\n"
                     "40 PRINT \"BIG\"\n"
                     "50 CASE IS <= 5\n"
                     "60 PRINT \"SMALL\"\n"
                     "70 END SELECT\n") == "BIG");

  CHECK(lwtest::Row0("10 X = 2\n"
                     "20 SELECT CASE X\n"
                     "30 CASE IS > 5\n"
                     "40 PRINT \"BIG\"\n"
                     "50 CASE IS <= 5\n"
                     "60 PRINT \"SMALL\"\n"
                     "70 END SELECT\n") == "SMALL");

  CHECK(lwtest::Row0("10 X = 9\n"
                     "20 SELECT CASE X\n"
                     "30 CASE 1\n"
                     "40 PRINT \"ONE\"\n"
                     "50 CASE ELSE\n"
                     "60 PRINT \"ELSE\"\n"
                     "70 END SELECT\n") == "ELSE");

  CHECK(lwtest::Row0("10 X = 2\n"
                     "20 SELECT CASE X\n"
                     "30 CASE IS = 2\n"
                     "40 PRINT \"EQ\"\n"
                     "50 CASE ELSE\n"
                     "60 PRINT \"OTHER\"\n"
                     "70 END SELECT\n") == "EQ");

  CHECK(lwtest::Row0("10 S$ = \"B\"\n"
                     "20 SELECT CASE S$\n"
                     "30 CASE \"A\"\n"
                     "40 PRINT \"A\"\n"
                     "50 CASE \"B\"\n"
                     "60 PRINT \"B\"\n"
                     "70 END SELECT\n") == "B");
}

TEST_CASE("SELECT CASE: no match, nesting, and repeated execution", "[select-case]") {
  CHECK(lwtest::Row0("10 X = 9\n"
                     "20 SELECT CASE X\n"
                     "30 CASE 1\n"
                     "40 PRINT \"ONE\"\n"
                     "50 END SELECT\n"
                     "60 PRINT \"AFTER\"\n") == "AFTER");

  CHECK(lwtest::Row0("10 X = 1 : Y = 2\n"
                     "20 SELECT CASE X\n"
                     "30 CASE 1\n"
                     "40 SELECT CASE Y\n"
                     "50 CASE 2\n"
                     "60 PRINT \"NEST\"\n"
                     "70 END SELECT\n"
                     "80 END SELECT\n") == "NEST");

  // A second SELECT after the first proves the selector stack was balanced.
  CHECK(lwtest::RunErrorCode("10 X = 1\n"
                             "20 SELECT CASE X\n"
                             "30 CASE 1\n"
                             "40 END SELECT\n"
                             "50 SELECT CASE X\n"
                             "60 CASE 1\n"
                             "70 PRINT \"OK\"\n"
                             "80 END SELECT\n") == 0);
}

TEST_CASE("SELECT CASE: same-line form", "[select-case]") {
  CHECK(lwtest::Row0("10 X = 2 : SELECT CASE X : CASE 1 : PRINT \"ONE\" : CASE 2 : PRINT \"TWO\" : "
                     "END SELECT\n") == "TWO");
}

TEST_CASE("SELECT CASE: structural errors", "[select-case]") {
  CHECK(lwtest::CompileCode("10 SELECT CASE 1\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 CASE 1\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 END SELECT\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 SELECT CASE 1\n20 CASE 1\n30 END SELECT\n") == 0);
}

TEST_CASE("SELECT CASE: GOTO out is rejected", "[select-case]") {
  CHECK(lwtest::RunErrorCode("10 X = 1\n"
                             "20 SELECT CASE X\n"
                             "30 CASE 1\n"
                             "40 GOTO 100\n"
                             "50 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  CHECK(lwtest::RunErrorCode("10 T% = 100\n"
                             "20 SELECT CASE 1\n"
                             "30 CASE 1\n"
                             "40 GOTO T%\n"
                             "50 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  CHECK(lwtest::RunErrorCode("10 SELECT CASE 9\n"
                             "20 CASE 1\n"
                             "30 PRINT \"ONE\"\n"
                             "40 CASE ELSE\n"
                             "50 GOTO 100\n"
                             "60 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  CHECK(lwtest::RunErrorCode("10 SELECT CASE 1\n"
                             "20 CASE 1\n"
                             "30 IF 1 THEN GOTO 100\n"
                             "40 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  CHECK(lwtest::RunErrorCode("10 SELECT CASE 1\n"
                             "20 CASE 1\n"
                             "30 IF 1 THEN 100\n"
                             "40 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  CHECK(lwtest::RunErrorCode("10 SELECT CASE 1\n"
                             "20 CASE 1\n"
                             "30 ON 1 GOTO 100\n"
                             "40 END SELECT\n"
                             "100 PRINT \"OUT\"\n") == newbasic::kErrIllegalFunctionCall);

  // A subroutine must not RETURN out of a SELECT it opened.
  CHECK(lwtest::RunErrorCode("10 GOSUB 100\n"
                             "20 PRINT \"AFTER\"\n"
                             "30 END\n"
                             "100 SELECT CASE 1\n"
                             "110 CASE 1\n"
                             "120 RETURN\n"
                             "130 END SELECT\n") == newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("SELECT CASE: GOTO within and GOSUB out/back are allowed", "[select-case]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 X = 1\n"
                                                        "20 SELECT CASE X\n"
                                                        "30 CASE 1\n"
                                                        "40 GOSUB 100\n"
                                                        "50 PRINT \"IN\"\n"
                                                        "60 END SELECT\n"
                                                        "70 END\n"
                                                        "100 PRINT \"SUB\"\n"
                                                        "110 RETURN\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "SUB");
  CHECK(rows[1] == "IN");

  CHECK(lwtest::Row0("10 X = 1\n"
                     "20 SELECT CASE X\n"
                     "30 CASE 1\n"
                     "40 GOTO 60\n"
                     "50 PRINT \"SKIP\"\n"
                     "60 END SELECT\n"
                     "70 GOTO 200\n"
                     "200 PRINT \"AFTER\"\n") == "AFTER");
}

TEST_CASE("SLEEP: forwards milliseconds to the host", "[sleep]") {
  HeadlessScreen screen;
  newbasic::ScriptedHost host;
  RunOn(&screen, "10 SLEEP 25\n20 SLEEP 0\n30 END\n", &host);
  CHECK(host.slept_ms == 25);
  CHECK(lwtest::RunErrorCode("10 SLEEP -1\n") == newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("TEXTWIDTH and TEXTHEIGHT: CP437 metrics", "[text-metrics]") {
  CHECK(lwtest::Row0("10 IF TEXTWIDTH(\"A\", 2) = 16 THEN PRINT \"W\";\n"
                     "20 IF TEXTHEIGHT(\"A\" + CHR$(13) + \"BB\", 1) = 32 THEN PRINT \"H\";\n"
                     "30 IF TEXTWIDTH(\"A\" + CHR$(13) + \"BB\", 1) = 16 THEN PRINT \"X\";\n"
                     "40 PRINT\n") == "WHX");
  CHECK(lwtest::Row0("10 IF TEXTWIDTH(\"\") = 0 THEN PRINT \"E\";\n"
                     "20 IF TEXTHEIGHT(\"\") = 0 THEN PRINT \"Z\";\n"
                     "30 PRINT\n") == "EZ");
  CHECK(lwtest::RunErrorCode("10 X = TEXTWIDTH(\"A\", 9)\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::CompileCode("10 X = TEXTWIDTH()\n") == newbasic::kErrSyntax);
}

TEST_CASE("SCREENW, SCREENH, and VIEW queries", "[screen-query]") {
  CHECK(lwtest::Row0("10 SCREEN 720\n"
                     "20 IF SCREENW = 1280 THEN PRINT \"W\";\n"
                     "30 IF SCREENH = 720 THEN PRINT \"H\";\n"
                     "40 VIEW (10, 20)-(109, 219)\n"
                     "50 IF VIEWX1 = 10 THEN PRINT \"1\";\n"
                     "60 IF VIEWY1 = 20 THEN PRINT \"2\";\n"
                     "70 IF VIEWX2 = 109 THEN PRINT \"3\";\n"
                     "80 IF VIEWY2 = 219 THEN PRINT \"4\";\n"
                     "90 PRINT\n") == "WH1234");

  CHECK(lwtest::Row0("10 SCREEN 720\n"
                     "20 VIEW (10, 10)-(20, 20)\n"
                     "30 PUSH VIEW\n"
                     "40 VIEW (50, 50)-(60, 60)\n"
                     "50 POP VIEW\n"
                     "60 IF VIEWX1 = 10 AND VIEWY1 = 10 AND VIEWX2 = 20 AND VIEWY2 = 20 THEN PRINT "
                     "\"OK\"\n") == "OK");
  CHECK(lwtest::RunErrorCode("10 POP VIEW\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::Row0("10 IF VIEWX2 = -1 AND VIEWY2 = -1 THEN PRINT \"TEXT\"\n") == "TEXT");
}

TEST_CASE("Live mouse state and wheel", "[mouse-live]") {
  MouseScreen screen;
  RunOn(&screen, "10 PRINT MOUSEPOSX; MOUSEPOSY; MOUSEBUTTONS; MOUSEWHEEL\n");
  CHECK(screen.RenderRows()[0] == " 40  30  5  3");

  MouseScreen wheel_screen;
  RunOn(&wheel_screen, "10 PRINT MOUSEWHEEL; MOUSEWHEEL\n");
  CHECK(wheel_screen.RenderRows()[0] == " 3  0");

  HeadlessScreen no_device;
  RunOn(&no_device, "10 PRINT MOUSEPOSX; MOUSEPOSY; MOUSEBUTTONS\n");
  CHECK(no_device.RenderRows()[0] == " 0  0  0");
}

TEST_CASE("Lexer: the new names are reserved words", "[new-features][lexer]") {
  const char* const names[] = {"SELECT",     "CASE",       "SLEEP",     "PUSH",      "POP",
                               "LABEL",      "CONST",      "STACK",     "TEXTWIDTH", "TEXTHEIGHT",
                               "MOUSEPOSX",  "MOUSEPOSY",  "MOUSEBUTTONS", "MOUSEWHEEL",
                               "SCREENW",    "SCREENH",    "VIEWX1",    "VIEWY1",    "VIEWX2",
                               "VIEWY2"};
  for (const char* name : names) {
    INFO("name: " << name);
    CHECK(newbasic::Lexer::IsKeyword(name));
  }
}
