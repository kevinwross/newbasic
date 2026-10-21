#pragma once

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "newbasic/dap/framing.h"
#include "newbasic/dap/handler.h"
#include "newbasic/dap/json.h"

namespace newbasic::dap {

// Connects the DAP transport to the dispatcher: reads framed requests from an
// input stream and writes framed responses and events to an output stream.
// Kept separate from main() so the adapter can be driven by string streams in
// tests (no pipes, no subprocess).
class Session {
public:
  Session(Vm& vm, debug::Debugger& debugger) : handler_(vm, debugger) {}

  // Runs until end of input, returning the number of requests handled.
  std::size_t Run(std::istream& in, std::ostream& out) {
    std::size_t handled = 0;
    for (;;) {
      const Frame frame = ReadFrame(in);
      if (!frame.ok) {
        if (frame.error == "eof")
          return handled;
        // The framing is broken, so the stream position is unknown: report
        // and stop rather than emit garbage forever.
        WriteFrame(out, ErrorResponse("", "framing error: " + frame.error).Dump());
        return handled;
      }
      Json request;
      std::string error;
      if (!Json::Parse(frame.body, &request, &error)) {
        // Framing is intact, so the session can continue after reporting.
        WriteFrame(out, ErrorResponse("", "malformed request: " + error).Dump());
        continue;
      }
      std::vector<Json> outgoing;
      handler_.Handle(request, &outgoing);
      for (const Json& message : outgoing) {
        WriteFrame(out, message.Dump());
      }
      ++handled;
    }
  }

private:
  // A response to something unparseable: there is no request seq or command to
  // echo, so both are reported empty rather than invented.
  static Json ErrorResponse(const std::string& command, const std::string& message) {
    Json response = Json::Object();
    response.Set("seq", Json::Number(0));
    response.Set("type", Json::Str("response"));
    response.Set("request_seq", Json::Number(0));
    response.Set("success", Json::Bool(false));
    response.Set("command", Json::Str(command));
    response.Set("message", Json::Str(message));
    return response;
  }

  Handler handler_;
  double seq_ = 1;
};

} // namespace newbasic::dap
