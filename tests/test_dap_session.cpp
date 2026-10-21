// M5: DAP session loop over streams (framing + JSON + dispatcher).
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "lwbasic/dap/framing.h"
#include "lwbasic/dap/session.h"
#include "lwbasic/debugger.h"
#include "test_helpers.h"

using lwbasic::dap::Frame;
using lwbasic::dap::Json;
using lwbasic::dap::ReadFrame;
using lwbasic::dap::Session;
using lwbasic::dap::WriteFrame;

namespace {

struct Adapter {
  Adapter() : debugger(vm), session(vm, debugger) {}

  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Vm vm{screen, &host};
  lwbasic::debug::Debugger debugger;
  Session session;
};

Json BodyOf(const Frame& frame) {
  Json json;
  std::string error;
  if (!Json::Parse(frame.body, &json, &error)) {
    FAIL("response body is not JSON: " << error << " [" << frame.body << "]");
  }
  return json;
}

} // namespace

TEST_CASE("DAP session: a full handshake over framed streams", "[dap]") {
  Adapter adapter;
  std::stringstream in;
  WriteFrame(in, "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}");
  WriteFrame(in, "{\"seq\":2,\"type\":\"request\",\"command\":\"launch\",\"arguments\":"
                 "{\"programText\":\"10 A=1\\n20 A=A+1\\n30 END\\n\"}}");
  WriteFrame(in, "{\"seq\":3,\"type\":\"request\",\"command\":\"setBreakpoints\",\"arguments\":"
                 "{\"breakpoints\":[{\"line\":20}]}}");
  WriteFrame(in, "{\"seq\":4,\"type\":\"request\",\"command\":\"configurationDone\"}");
  WriteFrame(in, "{\"seq\":5,\"type\":\"request\",\"command\":\"disconnect\"}");

  std::stringstream out;
  CHECK(adapter.session.Run(in, out) == 5);

  // First response: initialize, with its request_seq echoed.
  const Json initialize = BodyOf(ReadFrame(out));
  CHECK(initialize.Find("success")->AsBool());
  CHECK(initialize.Find("command")->AsString() == "initialize");
  CHECK(initialize.Find("request_seq")->AsNumber() == 1.0);

  const Json launch = BodyOf(ReadFrame(out));
  CHECK(launch.Find("command")->AsString() == "launch");
  const Json breakpoints = BodyOf(ReadFrame(out));
  CHECK(breakpoints.Find("command")->AsString() == "setBreakpoints");

  // configurationDone produces its response and then a stopped event, both
  // framed on the same stream and in that order.
  const Json config = BodyOf(ReadFrame(out));
  CHECK(config.Find("command")->AsString() == "configurationDone");
  const Json stopped = BodyOf(ReadFrame(out));
  CHECK(stopped.Find("type")->AsString() == "event");
  CHECK(stopped.Find("event")->AsString() == "stopped");
  CHECK(stopped.Find("body")->Find("reason")->AsString() == "breakpoint");

  const Json disconnect = BodyOf(ReadFrame(out));
  CHECK(disconnect.Find("command")->AsString() == "disconnect");
  CHECK(disconnect.Find("success")->AsBool());

  const Frame end = ReadFrame(out);
  CHECK_FALSE(end.ok);
  CHECK(end.error == "eof");
}

TEST_CASE("DAP session: malformed JSON is reported and the loop continues", "[dap]") {
  Adapter adapter;
  std::stringstream in;
  WriteFrame(in, "this is not json");
  WriteFrame(in, "{\"seq\":2,\"type\":\"request\",\"command\":\"threads\"}");

  std::stringstream out;
  CHECK(adapter.session.Run(in, out) == 1); // the valid request

  const Json failure = BodyOf(ReadFrame(out));
  CHECK_FALSE(failure.Find("success")->AsBool());
  CHECK(failure.Find("message")->AsString().find("malformed request") != std::string::npos);
  CHECK(failure.Find("request_seq")->AsNumber() == 0.0); // nothing to echo

  const Json threads = BodyOf(ReadFrame(out));
  CHECK(threads.Find("success")->AsBool());
  CHECK(threads.Find("command")->AsString() == "threads");
}

TEST_CASE("DAP session: broken framing stops the loop after reporting", "[dap]") {
  Adapter adapter;
  std::stringstream in("Content-Type: application/vsdp\r\n\r\nbody without a length");

  std::stringstream out;
  CHECK(adapter.session.Run(in, out) == 0);
  const Json failure = BodyOf(ReadFrame(out));
  CHECK_FALSE(failure.Find("success")->AsBool());
  CHECK(failure.Find("message")->AsString().find("framing error") != std::string::npos);
}

TEST_CASE("DAP session: end of input ends the session quietly", "[dap]") {
  Adapter adapter;
  std::stringstream in;
  std::stringstream out;
  CHECK(adapter.session.Run(in, out) == 0);
  CHECK(out.str().empty());
}

TEST_CASE("DAP session: a truncated body is a framing error, not a hang", "[dap]") {
  Adapter adapter;
  std::stringstream in("Content-Length: 100\r\n\r\n{\"seq\":1}");
  std::stringstream out;
  CHECK(adapter.session.Run(in, out) == 0);
  const Json failure = BodyOf(ReadFrame(out));
  CHECK(failure.Find("message")->AsString().find("truncated body") != std::string::npos);
}
