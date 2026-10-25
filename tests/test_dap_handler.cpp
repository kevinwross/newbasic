// M5: DAP request dispatcher driven without stdio.
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <string>
#include <vector>

#include "newbasic/dap/handler.h"
#include "newbasic/debugger.h"
#include "test_helpers.h"

using newbasic::dap::Handler;
using newbasic::dap::Json;

namespace {

struct Adapter {
  explicit Adapter(bool history = true) : debugger(vm), handler(vm, debugger, history) {}

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
  Handler handler;
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
  REQUIRE(outgoing.size() == 2);
  CHECK(outgoing[0].Find("success")->AsBool());
  CHECK(outgoing[0].Find("request_seq")->AsNumber() == 1.0);
  const Json* body = outgoing[0].Find("body");
  REQUIRE(body != nullptr);
  CHECK(body->Find("supportsConfigurationDoneRequest")->AsBool());
  CHECK(body->Find("supportsConditionalBreakpoints")->AsBool());
  CHECK(body->Find("supportsSetVariable")->AsBool());
  CHECK(outgoing[1].Find("type")->AsString() == "event");
  CHECK(outgoing[1].Find("event")->AsString() == "initialized");
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
  // Response, continued event, then a stopped event on the breakpoint line.
  REQUIRE(outgoing.size() == 3);
  CHECK(outgoing[0].Find("success")->AsBool());
  CHECK(outgoing[1].Find("event")->AsString() == "continued");
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

TEST_CASE("DAP: Step Over skips a GOSUB and Step Out returns from it", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=0\\n20 GOSUB 100\\n30 A=2\\n40 END\\n100 A=1\\n110 RETURN\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"breakpoints\":[{\"line\":2}]}}");
  const std::vector<Json> configured =
      adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}");
  REQUIRE(FirstEvent(configured, "stopped") != nullptr);
  CHECK(adapter.debugger.CurrentLine() == 20);

  const std::vector<Json> over =
      adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"next\"}");
  const Json* over_stop = FirstEvent(over, "stopped");
  REQUIRE(over_stop != nullptr);
  CHECK(adapter.debugger.CurrentLine() == 30);
  CHECK(adapter.debugger.Variable("A")->AsNumber() == 1.0);

  Adapter stepped;
  stepped.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=0\\n20 GOSUB 100\\n30 A=2\\n40 END\\n100 A=1\\n110 RETURN\\n\"}}");
  stepped.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"breakpoints\":[{\"line\":2}]}}");
  stepped.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}");
  const std::vector<Json> into =
      stepped.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"stepIn\"}");
  CHECK(FirstEvent(into, "stopped") != nullptr);
  CHECK(stepped.debugger.CurrentLine() == 100);

  const std::vector<Json> out =
      stepped.Request("{\"seq\":5,\"type\":\"request\",\"command\":\"stepOut\"}");
  REQUIRE(!out.empty());
  CHECK(out[0].Find("success")->AsBool());
  CHECK(FirstEvent(out, "stopped") != nullptr);
  CHECK(stepped.debugger.CurrentLine() == 30);
  CHECK(stepped.debugger.Variable("A")->AsNumber() == 1.0);
}

TEST_CASE("DAP: arrays appear in Variables and expand", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 DIM Numbers(2,3), Names$(2)\\n"
                  "20 A=1 : Numbers(0,0)=42 : Names$(1)=\\\"A\\\"\\n30 END\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"breakpoints\":[{\"line\":3}]}}");
  adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}");

  const std::vector<Json> outgoing =
      adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"variables\"}");
  const Json* variables = outgoing[0].Find("body")->Find("variables");
  REQUIRE(variables->Size() == 3); // A!, Numbers!, Names$

  int numbers_reference = 0;
  int names_reference = 0;
  for (const Json& item : variables->Items()) {
    const std::string name = item.Find("name")->AsString();
    if (name == "NUMBERS!") {
      CHECK(item.Find("value")->AsString().find("ARRAY(0..2, 0..3)") != std::string::npos);
      CHECK(item.Find("value")->AsString().find("12 elements") != std::string::npos);
      numbers_reference = static_cast<int>(item.Find("variablesReference")->AsNumber());
      CHECK(numbers_reference > 0);
    }
    if (name == "NAMES$") {
      CHECK(item.Find("value")->AsString().find("ARRAY(0..2)") != std::string::npos);
      CHECK(item.Find("value")->AsString().find("3 elements") != std::string::npos);
      names_reference = static_cast<int>(item.Find("variablesReference")->AsNumber());
      CHECK(names_reference > 0);
    }
  }
  REQUIRE(numbers_reference > 0);
  REQUIRE(names_reference > 0);

  const std::string numbers_request =
      "{\"seq\":5,\"type\":\"request\",\"command\":\"variables\",\"arguments\":"
      "{\"variablesReference\":" +
      std::to_string(numbers_reference) + "}}";
  const std::vector<Json> numbers = adapter.Request(numbers_request);
  const Json* number_items = numbers[0].Find("body")->Find("variables");
  REQUIRE(number_items->Size() == 12);
  CHECK(number_items->Items()[0].Find("name")->AsString() == "NUMBERS!(0,0)");
  CHECK(number_items->Items()[0].Find("value")->AsString() == "42");

  const std::string names_request =
      "{\"seq\":6,\"type\":\"request\",\"command\":\"variables\",\"arguments\":"
      "{\"variablesReference\":" +
      std::to_string(names_reference) + "}}";
  const std::vector<Json> names = adapter.Request(names_request);
  const Json* name_items = names[0].Find("body")->Find("variables");
  REQUIRE(name_items->Size() == 3);
  bool found_name = false;
  for (const Json& item : name_items->Items()) {
    if (item.Find("name")->AsString() == "NAMES$(1)") {
      found_name = true;
      CHECK(item.Find("value")->AsString() == "\"A\"");
    }
  }
  CHECK(found_name);
}

TEST_CASE("DAP: a new launch clears breakpoints and conditions", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=1\\n20 A=2\\n30 END\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"breakpoints\":[{\"line\":2,\"condition\":\"A=1\"}]}}");
  CHECK(adapter.debugger.HasBreakpoint(20));
  CHECK(adapter.debugger.HasCondition(20));

  adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 B=1\\n20 B=2\\n30 END\\n\"}}");
  CHECK_FALSE(adapter.debugger.HasBreakpoint(20));
  CHECK_FALSE(adapter.debugger.HasCondition(20));
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

TEST_CASE("DAP: setVariable edits Locals scalars", "[dap]") {
  Adapter adapter;
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=1\\n20 B$=\\\"HI\\\"\\n30 END\\n\"}}");
  adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                  "\"arguments\":{\"breakpoints\":[{\"line\":3}]}}");
  adapter.Request("{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}");

  const std::vector<Json> number =
      adapter.Request("{\"seq\":4,\"type\":\"request\",\"command\":\"setVariable\","
                      "\"arguments\":{\"variablesReference\":1,\"name\":\"A\",\"value\":\"42\"}}");
  REQUIRE(number[0].Find("success")->AsBool());
  CHECK(number[0].Find("body")->Find("value")->AsString() == "42");
  REQUIRE(adapter.debugger.Variable("A").has_value());
  CHECK(adapter.debugger.Variable("A")->AsNumber() == 42.0);

  const std::vector<Json> text =
      adapter.Request("{\"seq\":5,\"type\":\"request\",\"command\":\"setVariable\","
                      "\"arguments\":{\"variablesReference\":1,\"name\":\"B$\","
                      "\"value\":\"\\\"BYE\\\"\"}}");
  REQUIRE(text[0].Find("success")->AsBool());
  CHECK(text[0].Find("body")->Find("value")->AsString() == "\"BYE\"");
  REQUIRE(adapter.debugger.Variable("B$").has_value());
  CHECK(adapter.debugger.Variable("B$")->AsString() == "BYE");

  const std::vector<Json> unknown =
      adapter.Request("{\"seq\":6,\"type\":\"request\",\"command\":\"setVariable\","
                      "\"arguments\":{\"variablesReference\":1,\"name\":\"ZZ\",\"value\":\"1\"}}");
  CHECK_FALSE(unknown[0].Find("success")->AsBool());
}

TEST_CASE("DAP: Step Back follows the history setting", "[dap]") {
  Adapter adapter(false);
  const std::vector<Json> initialize =
      adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}");
  REQUIRE(initialize.size() == 2);
  const Json* body = initialize[0].Find("body");
  REQUIRE(body != nullptr);
  CHECK_FALSE(body->Find("supportsStepBack")->AsBool());

  const std::vector<Json> back =
      adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"stepBack\"}");
  REQUIRE(back.size() == 1);
  CHECK_FALSE(back[0].Find("success")->AsBool());
  CHECK(back[0].Find("message")->AsString().find("--history") != std::string::npos);
}

TEST_CASE("DAP: a pause request stops the running program", "[dap]") {
  Adapter adapter;
  std::atomic<bool> pause{true};
  adapter.handler.SetPauseFlag(&pause);
  adapter.Request("{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                  "{\"programText\":\"10 A=A+1\\n20 END\\n\"}}");
  const std::vector<Json> run =
      adapter.Request("{\"seq\":2,\"type\":\"request\",\"command\":\"configurationDone\"}");
  const Json* stopped = FirstEvent(run, "stopped");
  REQUIRE(stopped != nullptr);
  CHECK(stopped->Find("body")->Find("reason")->AsString() == "pause");
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
