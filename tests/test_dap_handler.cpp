// M5: DAP request dispatcher driven without stdio.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/dap/handler.h"
#include "newbasic/debugger.h"
#include "test_helpers.h"

using newbasic::dap::Handler;
using newbasic::dap::Json;

namespace {

struct Adapter {
  Adapter() : debugger(vm) {}

  // Feeds one request and returns everything the adapter would send.
  std::vector<Json> Request(const std::string& text) {
    Json request;
    std::string error;
    if (!Json::Parse(text, &request, &error)) {
      FAIL("bad request JSON: " << error);
    }
    std::vector<Json> outgoing;
    handler.Handle(request, &outgoing);
    return outgoing;
  }

  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Vm vm{screen, &host};
  newbasic::debug::Debugger debugger;
  Handler handler{vm, debugger};
};

const char* const kProgram = "10 A=1\n"
                             "20 A=A+1\n"
                             "30 B$=\"hi\"\n"
                             "40 A=A+1\n"
                             "50 END\n";

std::string ResponseWithCommand(const std::vector<Json>& outgoing, const std::string& command,
                                bool* found) {
  *found = false;
  for (const Json& message : outgoing) {
    const Json* type = message.Find("type");
    const Json* name = message.Find("command");
    if (type != nullptr && type->AsString() == "response" && name != nullptr &&
        name->AsString() == command) {
      *found = true;
    }
  }
  return std::string();
}

const Json* FirstEvent(const std::vector<Json>& outgoing, const std::string& name) {
  for (const Json& message : outgoing) {
    const Json* type = message.Find("type");
    const Json* event = message.Find("event");
    if (type != nullptr && type->AsString() == "event" && event != nullptr &&
        event->AsString() == name) {
      return &message;
    }
  }
  return nullptr;
}

} // namespace

TEST_CASE("DAP: initialize advertises the supported capabilities", "[dap]") {
  Adapter adapter;
  const std::vector<Json> outgoing =
      adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}");
  REQUIRE(outgoing.size() == 1);
  CHECK(outgoing[0].Find("success")->AsBool());
  CHECK(outgoing[0].Find("request_seq")->AsNumber() == 1.0);
  const Json* body = outgoing[0].Find("body");
  REQUIRE(body != nullptr);
  CHECK(body->Find("supportsConfigurationDoneRequest")->AsBool());
  CHECK(body->Find("supportsConditionalBreakpoints")->AsBool());
}

TEST_CASE("DAP: launch loads a program and runs to a breakpoint", "[dap]") {
  Adapter adapter;
  const std::string launch =
      std::string("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",") +
      "\"arguments\":{\"programText\":\"10 A=1\\n20 A=A+1\\n30 END\\n\"}}";
  std::vector<Json> outgoing = adapter.Request(launch);
  CHECK(outgoing[0].Find("success")->AsBool());

  bool found = false;
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"source\":{\"path\":\"x.bas\"},"
                  "\"breakpoints\":[{\"line\":2}]}}");
  ResponseWithCommand(outgoing, "setBreakpoints", &found);
  CHECK(adapter.debugger.HasBreakpoint(20));

  outgoing = adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"threads\"}");
  REQUIRE(outgoing.size() == 1);
  CHECK(outgoing[0].Find("body")->Find("threads")->Items()[0].Find("id")->AsNumber() == 1.0);

  outgoing = adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"configurationDone\"}");
  // Response then a stopped event on the breakpoint line.
  REQUIRE(outgoing.size() == 2);
  CHECK(outgoing[0].Find("success")->AsBool());
  const Json* stopped = FirstEvent(outgoing, "stopped");
  REQUIRE(stopped != nullptr);
  CHECK(stopped->Find("body")->Find("reason")->AsString() == "breakpoint");
  CHECK(stopped->Find("body")->Find("line")->AsNumber() == 2.0);

  // Inspection at the stop. The stack frame reports the editor line (2) while
  // the debugger internally stopped on BASIC line 20.
  outgoing = adapter.Request("{\"seq\":5,\"type\":\"request\",\"command\":\"stackTrace\"}");
  const Json& frame = outgoing[0].Find("body")->Find("stackFrames")->Items()[0];
  REQUIRE(frame.Find("line") != nullptr);
  CHECK(frame.Find("line")->AsNumber() == 2.0);
  outgoing = adapter.Request("{\"seq\":6,\"type\":\"request\",\"command\":\"variables\"}");
  const Json* variables = outgoing[0].Find("body")->Find("variables");
  REQUIRE(variables->Size() == 1);
  CHECK(variables->Items()[0].Find("name")->AsString() == "A!");
  CHECK(variables->Items()[0].Find("value")->AsString() == "1");

  // Stepping, then continuing to the end reports terminated.
  outgoing = adapter.Request("{\"seq\":7,\"type\":\"request\",\"command\":\"next\"}");
  const Json* step = FirstEvent(outgoing, "stopped");
  REQUIRE(step != nullptr);
  CHECK(step->Find("body")->Find("reason")->AsString() == "step");
  outgoing = adapter.Request("{\"seq\":8,\"type\":\"request\",\"command\":\"continue\"}");
  CHECK(FirstEvent(outgoing, "terminated") != nullptr);
}

TEST_CASE("DAP: conditional breakpoints", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 FOR I=1 TO 5\\n20 A=A+1\\n30 NEXT I\\n40 END\\n\"}}");
  const std::vector<Json> set = adapter.Request(
      "{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\",\"arguments\":{"
      "\"breakpoints\":[{\"line\":2,\"condition\":\"I=4\"}]}}");
  CHECK(set[0].Find("body")->Find("breakpoints")->Items()[0].Find("verified")->AsBool());
  CHECK(adapter.debugger.Condition(20) == "I=4");

  const std::vector<Json> run =
      adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}");
  const Json* stopped = FirstEvent(run, "stopped");
  REQUIRE(stopped != nullptr);
  CHECK(stopped->Find("body")->Find("line")->AsNumber() == 2.0);
  CHECK(adapter.debugger.Variable("I")->AsNumber() == 4.0);
}

TEST_CASE("DAP: evaluate reads a variable and rejects an unknown one", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=7\\n20 B$=\\\"hi\\\"\\n30 END\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"configurationDone\"}");
  const std::vector<Json> number =
      adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{"
                      "\"expression\":\"A\"}}");
  CHECK(number[0].Find("success")->AsBool());
  CHECK(number[0].Find("body")->Find("result")->AsString() == "7");
  const std::vector<Json> text =
      adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{"
                      "\"expression\":\"B$\"}}");
  CHECK(text[0].Find("body")->Find("result")->AsString() == "\"hi\"");
  const std::vector<Json> missing =
      adapter.Request("{\"seq\":5,\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{"
                      "\"expression\":\"NOPE\"}}");
  CHECK_FALSE(missing[0].Find("success")->AsBool());
  CHECK(missing[0].Find("message")->AsString().find("NOPE") != std::string::npos);
}

TEST_CASE("DAP: an error stop is reported as an exception", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 PRINT 1/0\\n20 END\\n\"}}");
  const std::vector<Json> outgoing =
      adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"configurationDone\"}");
  const Json* stopped = FirstEvent(outgoing, "stopped");
  REQUIRE(stopped != nullptr);
  CHECK(stopped->Find("body")->Find("reason")->AsString() == "exception");
  CHECK(stopped->Find("body")->Find("text")->AsString().find("Division by zero") !=
        std::string::npos);
}

TEST_CASE("DAP: unsupported requests fail cleanly", "[dap]") {
  Adapter adapter;
  const std::vector<Json> outgoing =
      adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"readMemory\"}");
  REQUIRE(outgoing.size() == 1);
  CHECK_FALSE(outgoing[0].Find("success")->AsBool());
  CHECK(outgoing[0].Find("message")->AsString().find("readMemory") != std::string::npos);
  CHECK(outgoing[0].Find("command")->AsString() == "readMemory");
}

TEST_CASE("DAP: stepBack and reverseContinue walk recorded history", "[dap][travel]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{"
                  "\"programText\":\"10 A=1\\n20 A=A+1\\n30 A=A+1\\n40 END\\n\"}}");
  // Three line steps: at 20 A=1, at 30 A=2, at 40 A=3.
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"next\"}");
  adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"next\"}");
  adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"next\"}");
  CHECK(adapter.debugger.CurrentLine() == 40);
  CHECK(adapter.debugger.Variable("A")->AsNumber() == 3.0);

  // stepBack: the response comes first, then a stopped event at the rewound line.
  const std::vector<Json> back =
      adapter.Request("{\"seq\":5,\"type\":\"request\",\"command\":\"stepBack\"}");
  CHECK(back[0].Find("success")->AsBool());
  CHECK(FirstEvent(back, "stopped") != nullptr);
  CHECK(adapter.debugger.CurrentLine() == 30);
  CHECK(adapter.debugger.Variable("A")->AsNumber() == 2.0);

  // reverseContinue rewinds to the previous line change.
  const std::vector<Json> reverse =
      adapter.Request("{\"seq\":6,\"type\":\"request\",\"command\":\"reverseContinue\"}");
  CHECK(reverse[0].Find("success")->AsBool());
  CHECK(adapter.debugger.CurrentLine() == 20);
  CHECK(adapter.debugger.Variable("A")->AsNumber() == 1.0);

  // Rewinding past the start reports a stop rather than failing the request.
  while (adapter.debugger.HistorySize() > 0) {
    adapter.Request("{\"seq\":7,\"type\":\"request\",\"command\":\"stepBack\"}");
  }
  const std::vector<Json> exhausted =
      adapter.Request("{\"seq\":8,\"type\":\"request\",\"command\":\"stepBack\"}");
  CHECK(exhausted[0].Find("success")->AsBool());
}

TEST_CASE("DAP: launching a new program clears recorded history", "[dap][travel]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{"
                  "\"programText\":\"10 A=1\\n20 END\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"next\"}");
  adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"next\"}");
  CHECK(adapter.debugger.HistorySize() > 0);

  adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{"
                  "\"programText\":\"10 B=2\\n20 END\\n\"}}");
  // Snapshots from the previous program would describe a different program.
  CHECK(adapter.debugger.HistorySize() == 0);
}
