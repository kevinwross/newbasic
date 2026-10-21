// M3: timer event statements (ON TIMER / TIMER ON|OFF|STOP).
//
// Compilation, operands and error paths are covered here. Event *firing* is
// not enabled yet: registering a handler works, but handlers do not run, so
// no tick assertions appear below (see the NOTE in Vm::Run and plan.md, M3).
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

TEST_CASE("Timer events: syntax", "[events]") {
  CHECK(lwtest::CompileCode("10 ON TIMER(1) GOSUB 100\n100 RETURN\n") == 0);
  CHECK(lwtest::CompileCode("10 ON TIMER(1) PRINT \"x\"\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 ON TIMER(1) GOSUB\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 TIMER ON\n20 TIMER OFF\n30 TIMER STOP\n") == 0);
  CHECK(lwtest::CompileCode("10 TIMER\n") == newbasic::kErrSyntax);
  CHECK(lwtest::CompileCode("10 TIMER MAYBE\n") == newbasic::kErrSyntax);
}

TEST_CASE("Timer events: programmes run to completion", "[events]") {
  CHECK(lwtest::RunErrorCode("10 ON TIMER(1) GOSUB 100\n"
                             "20 TIMER ON\n"
                             "30 PRINT \"OK\"\n"
                             "40 TIMER OFF\n"
                             "50 END\n"
                             "100 RETURN\n") == 0);
  // The TIMER function still reads the clock in expressions.
  CHECK(lwtest::Row0("10 T=TIMER: PRINT \"DONE\"\n") == "DONE");
}

TEST_CASE("Timer events: the poll's inputs are armed in the right order", "[events]") {
  // This was a measurement test that only printed the poll state while the
  // defect was open; it now *asserts* the same things, so it guards against a
  // regression in registration, enablement or the stepping seam. The print
  // version is what made the original bug findable - Vm::TimerDebug() stays
  // available for the next one.
  newbasic::Program program;
  newbasic::CompileError error;
  const std::string source = "10 ON TIMER(1) GOSUB 100\n"
                             "20 TIMER ON\n"
                             "30 FOR I=1 TO 3: NEXT I\n"
                             "40 END\n"
                             "100 PRINT \"TICK\"\n"
                             "110 RETURN\n";
  REQUIRE(lwtest::CompileSource(source, &program, &error));
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  host.timer_step = 10.0;
  newbasic::Vm vm(screen, &host);
  vm.Load(program);

  // Nothing armed before the statements run.
  CHECK(vm.TimerDebug().find("enabled=0") != std::string::npos);
  CHECK(vm.TimerDebug().find("interval=0.000000") != std::string::npos);

  // Two instructions make up `ON TIMER(1) GOSUB 100`: push the interval, then
  // register it. That arms the timer but does not enable it.
  vm.Step();
  vm.Step();
  CHECK(vm.TimerDebug().find("interval=1.000000") != std::string::npos);
  CHECK(vm.TimerDebug().find("enabled=0") != std::string::npos);

  // The next instruction is `TIMER ON`.
  vm.Step();
  CHECK(vm.TimerDebug().find("enabled=1") != std::string::npos);

  // Stepping never polls - the poll lives in Vm::Run - and once the program has
  // halted, Step() must refuse to continue. Before that guard existed, stepping
  // past END executed the handler body that physically follows the main
  // program, printing TICK while timer_last_ was still untouched; that looked
  // deceptively like "ON TIMER runs its handler at registration".
  for (int i = 0; i < 10; ++i) {
    vm.Step();
  }
  CHECK(screen.RenderRows()[0].empty());
  CHECK(vm.TimerDebug().find("last=0.000000") != std::string::npos);

  // Further stepping stays halted rather than running off the end.
  CHECK_FALSE(vm.Step());
  CHECK_FALSE(vm.Step());

  // A debugger rewind re-arms execution (SetPc clears the halted state).
  vm.SetPc(0);
  CHECK(vm.Step());
}

TEST_CASE("Timer events: a ready clock fires the handler (Run path)", "[events][firing]") {
  // The decisive test: the program is driven through Vm::Run (via the helper),
  // the clock advances 10 seconds per read against a 1 second interval, so the
  // poll must fire. This is the assertion that has been failing.
  newbasic::ScriptedHost host;
  host.timer_step = 10.0;
  const std::vector<std::string> rows = lwtest::RunRowsWithInput("10 ON TIMER(1) GOSUB 100\n"
                                                                 "20 TIMER ON\n"
                                                                 "30 FOR I=1 TO 20: NEXT I\n"
                                                                 "40 END\n"
                                                                 "100 PRINT \"TICK\"\n"
                                                                 "110 RETURN\n",
                                                                 {}, &host);
  int ticks = 0;
  for (const std::string& row : rows) {
    if (row == "TICK")
      ++ticks;
  }
  INFO("ticks=" << ticks << " rows0=[" << rows[0] << "] rows1=[" << rows[1] << "]");
  CHECK(ticks > 0);
  // Bounded by main-program progress: an event needs a real instruction in
  // between, so the count cannot run away even though the clock is ready on
  // every poll. The bound is generous because events are per instruction, not
  // per loop iteration.
  CHECK(ticks < 200);
}
