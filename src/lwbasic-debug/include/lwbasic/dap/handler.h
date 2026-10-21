#pragma once

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "lwbasic/dap/json.h"
#include "lwbasic/debugger.h"
#include "lwbasic/errors.h"
#include "lwbasic/frontend.h"
#include "lwbasic/line_store.h"
#include "lwbasic/vm.h"

namespace lwbasic::dap {

// Handles Debug Adapter Protocol requests against the debugger library.
//
// Pure: it takes a request value and returns a response plus any events, with
// no stdio, so the whole protocol surface is testable by feeding request JSON
// and inspecting response JSON. The lwbasic-dap executable only adds framing
// and the stdin/stdout loop on top.
class Handler {
public:
  Handler(Vm& vm, debug::Debugger& debugger) : vm_(vm), debugger_(debugger) {}

  // Handles one request; appends responses/events to `outgoing`.
  void Handle(const Json& request, std::vector<Json>* outgoing) {
    const std::string command = String(request, "command");
    const Json* arguments = request.Find("arguments");
    if (command == "initialize") {
      Json body = Json::Object();
      body.Set("supportsConfigurationDoneRequest", Json::Bool(true));
      body.Set("supportsConditionalBreakpoints", Json::Bool(true));
      body.Set("supportsEvaluateForHovers", Json::Bool(true));
      body.Set("supportsTerminateRequest", Json::Bool(false));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "launch" || command == "attach") {
      if (arguments != nullptr) {
        const std::string text = String(*arguments, "programText");
        const std::string path = String(*arguments, "program");
        if (!text.empty()) {
          LoadSource(text, outgoing);
          return;
        }
        if (!path.empty()) {
          LineStore store;
          std::string error;
          if (!LineStore::ParseTextBas(ReadFile(path), &store, &error)) {
            Fail(request, error, outgoing);
            return;
          }
          Program program;
          CompileError compile_error;
          if (!Compile(store, &program, &compile_error)) {
            Fail(request, compile_error.message, outgoing);
            return;
          }
          debugger_.ClearBreakpoints();
          debugger_.ClearHistory(); // a new program invalidates recorded history
          vm_.Load(std::move(program));
          Respond(request, Json::Object(), outgoing);
          return;
        }
      }
      Respond(request, Json::Object(), outgoing);
      return;
    }
    if (command == "setBreakpoints") {
      debugger_.ClearBreakpoints();
      Json body = Json::Object();
      Json verified = Json::Array();
      if (arguments != nullptr) {
        const Json* list = arguments->Find("breakpoints");
        if (list != nullptr && list->IsArray()) {
          for (const Json& entry : list->Items()) {
            if (!entry.IsObject())
              continue;
            const Json* line = entry.Find("line");
            if (line == nullptr || !line->IsNumber())
              continue;
            const int number = static_cast<int>(line->AsNumber());
            debugger_.SetBreakpoint(number);
            const Json* condition = entry.Find("condition");
            if (condition != nullptr && condition->IsString()) {
              debugger_.SetCondition(number, condition->AsString());
            }
            Json item = Json::Object();
            item.Set("verified", Json::Bool(true));
            item.Set("line", Json::Number(number));
            verified.Push(std::move(item));
          }
        }
      }
      body.Set("breakpoints", std::move(verified));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "configurationDone" || command == "continue") {
      Respond(request, Json::Object(), outgoing);
      RunAndReport(outgoing);
      return;
    }
    if (command == "next" || command == "stepIn" || command == "stepOver") {
      Respond(request, Json::Object(), outgoing);
      ReportStop(debugger_.StepLine(), "step", outgoing);
      return;
    }
    if (command == "stepBack" || command == "reverseContinue") {
      // Time travel (M7): the debugger keeps a bounded ring of snapshots taken
      // at line boundaries, so these walk back through recorded history.
      Respond(request, Json::Object(), outgoing);
      const debug::StopState state =
          command == "stepBack" ? debugger_.StepBack() : debugger_.ReverseContinue();
      ReportStop(state, "step", outgoing);
      return;
    }
    if (command == "threads") {
      Json body = Json::Object();
      Json threads = Json::Array();
      Json thread = Json::Object();
      thread.Set("id", Json::Number(1));
      thread.Set("name", Json::Str("main"));
      threads.Push(std::move(thread));
      body.Set("threads", std::move(threads));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "stackTrace") {
      Json body = Json::Object();
      Json frames = Json::Array();
      int id = 0;
      Json top = Json::Object();
      top.Set("id", Json::Number(id++));
      top.Set("name", Json::Str("line " + std::to_string(debugger_.CurrentLine())));
      top.Set("line", Json::Number(debugger_.CurrentLine()));
      top.Set("column", Json::Number(1));
      frames.Push(std::move(top));
      for (const Vm::CallFrame& frame : debugger_.CallFrames()) {
        Json item = Json::Object();
        item.Set("id", Json::Number(id++));
        const std::string label = frame.kind == Vm::CallFrame::Kind::Gosub
                                      ? "GOSUB line " + std::to_string(frame.line)
                                      : "FOR " + frame.name + " line " + std::to_string(frame.line);
        item.Set("name", Json::Str(label));
        item.Set("line", Json::Number(frame.line));
        item.Set("column", Json::Number(1));
        frames.Push(std::move(item));
      }
      body.Set("stackFrames", std::move(frames));
      body.Set("totalFrames", Json::Number(id));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "scopes") {
      Json body = Json::Object();
      Json scopes = Json::Array();
      Json locals = Json::Object();
      locals.Set("name", Json::Str("Locals"));
      locals.Set("variablesReference", Json::Number(1));
      locals.Set("expensive", Json::Bool(false));
      scopes.Push(std::move(locals));
      body.Set("scopes", std::move(scopes));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "variables") {
      Json body = Json::Object();
      Json variables = Json::Array();
      for (const std::string& name : debugger_.VariableNames()) {
        const std::optional<Value> value = debugger_.Variable(name);
        if (!value.has_value())
          continue;
        Json item = Json::Object();
        item.Set("name", Json::Str(name));
        item.Set("value", Json::Str(FormatValue(*value)));
        item.Set("variablesReference", Json::Number(0));
        variables.Push(std::move(item));
      }
      body.Set("variables", std::move(variables));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "evaluate") {
      const std::string expression =
          arguments == nullptr ? std::string() : String(*arguments, "expression");
      // Exactly a variable name: a stop-gap until full expression evaluation.
      const std::optional<Value> value = debugger_.Variable(expression);
      if (!value.has_value()) {
        Fail(request, "No such variable: " + expression, outgoing);
        return;
      }
      Json body = Json::Object();
      body.Set("result", Json::Str(FormatValue(*value)));
      body.Set("variablesReference", Json::Number(0));
      Respond(request, std::move(body), outgoing);
      return;
    }
    if (command == "disconnect" || command == "terminate") {
      Respond(request, Json::Object(), outgoing);
      return;
    }
    Fail(request, "Unsupported request: " + command, outgoing);
  }

private:
  static std::string String(const Json& object, const char* key) {
    const Json* value = object.Find(key);
    return value != nullptr && value->IsString() ? value->AsString() : std::string();
  }

  static std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
      return std::string();
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
  }

  static std::string FormatValue(const Value& value) {
    if (value.IsString())
      return "\"" + value.AsString() + "\"";
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%g", value.AsNumber());
    return buffer;
  }

  void LoadSource(const std::string& text, std::vector<Json>* outgoing) {
    // Requests are handled one at a time; the failing request is unknown here,
    // so report through a synthetic response.
    Json request = Json::Object();
    request.Set("command", Json::Str("launch"));
    request.Set("seq", Json::Number(0));
    LineStore store;
    std::string error;
    if (!LineStore::ParseTextBas(text, &store, &error)) {
      Fail(request, error, outgoing);
      return;
    }
    Program program;
    CompileError compile_error;
    if (!Compile(store, &program, &compile_error)) {
      Fail(request, compile_error.message, outgoing);
      return;
    }
    debugger_.ClearBreakpoints();
    debugger_.ClearHistory(); // a new program invalidates recorded history
    vm_.Load(std::move(program));
    Respond(request, Json::Object(), outgoing);
  }

  void RunAndReport(std::vector<Json>* outgoing) {
    ReportStop(debugger_.Continue(), "breakpoint", outgoing);
  }

  void ReportStop(const debug::StopState& state, const char* step_reason,
                  std::vector<Json>* outgoing) {
    if (state.reason == debug::StopReason::ProgramEnd) {
      Json event = Event("terminated");
      Json body = Json::Object();
      event.Set("body", std::move(body));
      outgoing->push_back(std::move(event));
      return;
    }
    Json event = Event("stopped");
    Json body = Json::Object();
    const char* reason = state.reason == debug::StopReason::Error ? "exception" : step_reason;
    if (state.reason == debug::StopReason::Breakpoint)
      reason = "breakpoint";
    body.Set("reason", Json::Str(reason));
    body.Set("threadId", Json::Number(1));
    body.Set("allThreadsStopped", Json::Bool(true));
    body.Set("line", Json::Number(state.line));
    if (state.reason == debug::StopReason::Error) {
      body.Set("text", Json::Str(ErrorMessage(state.result.code)));
    }
    event.Set("body", std::move(body));
    outgoing->push_back(std::move(event));
  }

  Json Event(const char* name) {
    Json event = Json::Object();
    event.Set("seq", Json::Number(seq_++));
    event.Set("type", Json::Str("event"));
    event.Set("event", Json::Str(name));
    return event;
  }

  void Respond(const Json& request, Json body, std::vector<Json>* outgoing) {
    Json response = Json::Object();
    response.Set("seq", Json::Number(seq_++));
    response.Set("type", Json::Str("response"));
    response.Set("request_seq", Json::Number(Number(request, "seq")));
    response.Set("success", Json::Bool(true));
    response.Set("command", Json::Str(String(request, "command")));
    response.Set("body", std::move(body));
    outgoing->push_back(std::move(response));
  }

  void Fail(const Json& request, const std::string& message, std::vector<Json>* outgoing) {
    Json response = Json::Object();
    response.Set("seq", Json::Number(seq_++));
    response.Set("type", Json::Str("response"));
    response.Set("request_seq", Json::Number(Number(request, "seq")));
    response.Set("success", Json::Bool(false));
    response.Set("command", Json::Str(String(request, "command")));
    response.Set("message", Json::Str(message));
    outgoing->push_back(std::move(response));
  }

  static double Number(const Json& object, const char* key) {
    const Json* value = object.Find(key);
    return value != nullptr && value->IsNumber() ? value->AsNumber() : 0.0;
  }

  Vm& vm_;
  debug::Debugger& debugger_;
  double seq_ = 1;
};

} // namespace lwbasic::dap
