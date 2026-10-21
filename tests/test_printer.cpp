// Printer output (plan.md 7b): LLIST and LPRINT go through Host::WriteToPrinter, so
// tests can assert what a program would have printed without any hardware.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

TEST_CASE("Printer: LPRINT sends its line to the printer", "[printer]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 LPRINT \"HELLO\"");
  repl.Process("20 END");
  repl.Process("RUN");
  CHECK(host.printer.find("HELLO") != std::string::npos);
  // Nothing went to the screen: LPRINT is printer-bound.
  std::string on_screen;
  for (const std::string& row : screen.RenderRows())
    on_screen += row;
  CHECK(on_screen.find("HELLO") == std::string::npos);
}

TEST_CASE("Printer: LPRINT keeps PRINT's item formatting", "[printer]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 LPRINT 42;\"!\";");
  repl.Process("20 LPRINT USING \"###.##\"; 1.5");
  repl.Process("30 END");
  repl.Process("RUN");
  // A number gets its leading space and the trailing space PRINT puts after
  // numbers, and USING works on the printer too.
  CHECK(host.printer.find(" 42 !") != std::string::npos);
  CHECK(host.printer.find("1.50") != std::string::npos);
}

TEST_CASE("Printer: LLIST sends the stored program", "[printer]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"A\"");
  repl.Process("20 END");
  repl.Process("LLIST");
  CHECK(host.printer.find("10 PRINT \"A\"") != std::string::npos);
  CHECK(host.printer.find("20 END") != std::string::npos);
  // The screen stays clear: LLIST goes to the printer, unlike LIST.
  std::string on_screen;
  for (const std::string& row : screen.RenderRows())
    on_screen += row;
  CHECK(on_screen.find("20 END") == std::string::npos);
}

TEST_CASE("Printer: several LPRINTs, then a listing", "[printer]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT 1");
  repl.Process("20 PRINT 2");
  repl.Process("LPRINT \"ONE\"");
  repl.Process("LPRINT \"TWO\"");
  repl.Process("LLIST");
  // Both printer lines arrived, in order.
  const std::size_t one = host.printer.find("ONE");
  const std::size_t two = host.printer.find("TWO");
  CHECK(one != std::string::npos);
  CHECK(two != std::string::npos);
  CHECK(one < two);
  // LLIST writes the stored program to the same sink.
  // NOTE: a range argument (LLIST 20-30) is ignored; recorded in plan.md section A.
  CHECK(host.printer.find("10 PRINT 1") != std::string::npos);
  CHECK(host.printer.find("20 PRINT 2") != std::string::npos);
}
