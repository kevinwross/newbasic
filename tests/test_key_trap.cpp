// M11: event trapping beyond the timer - ON KEY(n) GOSUB and KEY(n) ON/OFF/STOP.
// The input model delivers characters rather than scan codes, so any pending key
// fires the lowest armed trap; the character is then handed to INKEY$ so a handler
// can read it.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "lwbasic/errors.h"
#include "lwbasic/repl.h"
#include "lwbasic/screen/headless_screen.h"
#include "test_helpers.h"

namespace {

std::string AllRows(const lwbasic::screen::HeadlessScreen& screen) {
  std::string all;
  for (const std::string& row : screen.RenderRows())
    all += row + "\n";
  return all;
}

// A short busy loop gives the event poll chances to run between instructions.
// (No trailing newline: Process takes a single line.)
const char kLoop[] = "30 FOR I=1 TO 3000: NEXT";

} // namespace

TEST_CASE("M11: ON KEY(n) fires when a key is pending", "[m11]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 ON KEY(1) GOSUB 100");
  repl.Process("20 KEY(1) ON");
  repl.Process(kLoop);
  repl.Process("40 PRINT \"DONE\"");
  repl.Process("50 END");
  repl.Process("100 PRINT \"TRAP\": RETURN");
  screen.ScriptKey(L'X');
  screen.Clear();
  const lwbasic::RunResult result = repl.Process("RUN");
  const std::string all = AllRows(screen);
  INFO("status=" << static_cast<int>(result.status) << " code=" << result.code
                 << " line=" << result.line << " screen: [" << all << "]");
  CHECK(all.find("TRAP") != std::string::npos);
  CHECK(all.find("DONE") != std::string::npos);
}

TEST_CASE("M11: KEY(n) OFF stops the trap firing", "[m11]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 ON KEY(1) GOSUB 100");
  repl.Process("20 KEY(1) OFF");
  repl.Process(kLoop);
  repl.Process("40 PRINT \"DONE\"");
  repl.Process("50 END");
  repl.Process("100 PRINT \"TRAP\": RETURN");
  screen.ScriptKey(L'X');
  screen.Clear();
  repl.Process("RUN");
  const std::string all = AllRows(screen);
  CHECK(all.find("DONE") != std::string::npos);
  CHECK(all.find("TRAP") == std::string::npos);
}

TEST_CASE("M11: the trap hands the key to INKEY$ in the handler", "[m11]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 ON KEY(1) GOSUB 100");
  repl.Process("20 KEY(1) ON");
  repl.Process(kLoop);
  repl.Process("40 PRINT \"DONE\"");
  repl.Process("50 END");
  repl.Process("100 K$=INKEY$: PRINT \"KEY=\";K$: RETURN");
  screen.ScriptKey(L'Z');
  screen.Clear();
  repl.Process("RUN");
  const std::string all = AllRows(screen);
  // The character consumed to fire the trap is still readable.
  CHECK(all.find("KEY=Z") != std::string::npos);
}

TEST_CASE("M11: KEY(n) ON without a handler is Illegal function call", "[m11]") {
  CHECK(lwtest::RunErrorCode("10 KEY(1) ON\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 KEY(11) ON\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 ON KEY(11) GOSUB 100\n100 RETURN\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("M11: hardware-only traps report Advanced Feature with a reason", "[m11]") {
  CHECK(lwtest::RunErrorCode("10 ON COM(1) GOSUB 100\n100 RETURN\n") ==
        lwbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 ON STRIG(1) GOSUB 100\n100 RETURN\n") ==
        lwbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 ON PLAY(1) GOSUB 100\n100 RETURN\n") ==
        lwbasic::kErrAdvancedFeature);
  CHECK(lwtest::RunErrorCode("10 ON PEN GOSUB 100\n100 RETURN\n") == lwbasic::kErrAdvancedFeature);
}
