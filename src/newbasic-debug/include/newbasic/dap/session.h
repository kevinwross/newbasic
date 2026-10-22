#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <istream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
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
    pause_requested_.store(false);
    handler_.SetPauseFlag(&pause_requested_);

    // Input runs on a reader thread so a pause can arrive while Continue() is
    // executing. The reader never touches the VM, debugger or output stream: it
    // parses commands only to set the pause flag, then queues the frame for the
    // main thread.
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<Frame> queue;
    bool eof = false;
    // The main thread owns all DAP output, but the reader may answer a simple
    // threads request while the VM is running; the mutex keeps frames whole.
    std::atomic<bool> running{false};
    std::mutex write_mutex;
    auto WriteMessage = [&](const Json& message) {
      std::lock_guard<std::mutex> lock(write_mutex);
      WriteFrame(out, message.Dump());
    };
    std::thread reader([&] {
      for (;;) {
        Frame frame = ReadFrame(in);
        if (!frame.ok && frame.error == "eof") {
          std::lock_guard<std::mutex> lock(queue_mutex);
          eof = true;
          queue_cv.notify_one();
          return;
        }
        if (frame.ok) {
          Json request;
          std::string error;
          if (Json::Parse(frame.body, &request, &error)) {
            const Json* command = request.Find("command");
            if (command != nullptr && command->IsString() &&
                command->AsString() == "pause") {
              pause_requested_.store(true);
            } else if (command != nullptr && command->IsString() &&
                       command->AsString() == "threads" && running.load()) {
              // VS Code asks for threads immediately after the continued event.
              // The main thread is already inside Continue, so answer this
              // stateless request here; otherwise session setup waits for the
              // program to stop and the Pause button never appears.
              const Json* request_seq = request.Find("seq");
              Json response = Json::Object();
              response.Set("seq", Json::Number(handler_.NextSeq()));
              response.Set("type", Json::Str("response"));
              response.Set("request_seq",
                           Json::Number(request_seq != nullptr && request_seq->IsNumber()
                                            ? request_seq->AsNumber()
                                            : 0));
              response.Set("success", Json::Bool(true));
              response.Set("command", Json::Str("threads"));
              Json body = Json::Object();
              Json threads = Json::Array();
              Json thread = Json::Object();
              thread.Set("id", Json::Number(1));
              thread.Set("name", Json::Str("main"));
              threads.Push(std::move(thread));
              body.Set("threads", std::move(threads));
              response.Set("body", std::move(body));
              WriteMessage(response);
              continue;
            }
          }
        }
        const bool frame_ok = frame.ok;
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          queue.push_back(std::move(frame));
          queue_cv.notify_one();
        }
        if (!frame_ok) {
          // Malformed framing: the main thread reports it and stops, so no
          // further input is expected.
          std::lock_guard<std::mutex> lock(queue_mutex);
          eof = true;
          queue_cv.notify_one();
          return;
        }
      }
    });

    for (;;) {
      Frame frame;
      {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [&] { return !queue.empty() || eof; });
        if (queue.empty() && eof)
          break;
        frame = std::move(queue.front());
        queue.pop_front();
      }
      if (!frame.ok) {
        // The framing is broken, so the stream position is unknown: report
        // and stop rather than emit garbage forever.
        WriteMessage(ErrorResponse("", "framing error: " + frame.error));
        break;
      }
      Json request;
      std::string error;
      if (!Json::Parse(frame.body, &request, &error)) {
        // Framing is intact, so the session can continue after reporting.
        WriteMessage(ErrorResponse("", "malformed request: " + error));
        continue;
      }
      std::vector<Json> outgoing;
      const Handler::RunAction action = handler_.Handle(request, &outgoing, false);
      if (action != Handler::RunAction::None)
        running.store(true);
      for (const Json& message : outgoing) {
        WriteMessage(message);
      }
      if (action != Handler::RunAction::None) {
        // The response and continued event are on the wire before this blocks;
        // the reader thread can now raise pause or answer threads while the VM
        // runs.
        std::vector<Json> after;
        handler_.RunActionNow(action, &after);
        running.store(false);
        for (const Json& message : after) {
          WriteMessage(message);
        }
      }
      ++handled;
    }

    if (reader.joinable())
      reader.join();
    return handled;
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
  std::atomic<bool> pause_requested_{false};
};

} // namespace newbasic::dap
