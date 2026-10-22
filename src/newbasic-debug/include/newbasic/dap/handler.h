#pragma once

#include <atomic>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "newbasic/dap/json.h"
#include "newbasic/debugger.h"
#include "newbasic/errors.h"
#include "newbasic/frontend.h"
#include "newbasic/line_store.h"
#include "newbasic/vm.h"

namespace newbasic::dap {

// Handles Debug Adapter Protocol requests against the debugger library.
//
// Pure: it takes a request value and returns a response plus any events, with
// no stdio, so the whole protocol surface is testable by feeding request JSON
// and inspecting response JSON. The newbasic-dap executable only adds framing
// and the stdin/stdout loop on top.
class Handler {
public:
  // What execution, if any, a request asks for. Session flushes the response
  // and continued event before calling RunActionNow, so a client enters its
  // running state before the VM blocks; tests can keep the old immediate
  // behaviour by leaving run_immediately at its default.
  enum class RunAction { None, Continue, StepLine, StepBack, ReverseContinue };

  Handler(Vm& vm, debug::Debugger& debugger) : vm_(vm), debugger_(debugger) {}

  void SetPauseFlag(std::atomic<bool>* flag) { debugger_.SetPauseFlag(flag); }

  // Handles one request; appends responses/events to `outgoing`. When
  // run_immediately is false, a run/step action is returned to the caller
  // instead of being executed here.
  RunAction Handle(const Json& request, std::vector<Json>* outgoing,
                   bool run_immediately = true) {
    const std::string command = String(request, "command");
    const Json* arguments = request.Find("arguments");
    if (command == "initialize") {
      Json body = Json::Object();
      body.Set("supportsConfigurationDoneRequest", Json::Bool(true));
      body.Set("supportsConditionalBreakpoints", Json::Bool(true));
      body.Set("supportsEvaluateForHovers", Json::Bool(true));
      body.Set("supportsStepBack", Json::Bool(true));
      body.Set("supportsTerminateRequest", Json::Bool(false));
      Respond(request, std::move(body), outgoing);
      // DAP requires this event after the initialize response. Clients such as
      // VS Code wait for it before sending breakpoints and configurationDone.
      outgoing->push_back(Event("initialized"));
      return RunAction::None;
    }
    if (command == "launch" || command == "attach") {
      ResetLineMap();
      if (arguments != nullptr) {
        const std::string text = String(*arguments, "programText");
        const std::string path = String(*arguments, "program");
        if (!text.empty()) {
          LoadSource(text, outgoing);
          return RunAction::None;
        }
        if (!path.empty()) {
          const std::string source = ReadFile(path);
          LineStore store;
          std::string error;
          if (!LineStore::ParseTextBas(source, &store, &error)) {
            Fail(request, error, outgoing);
            return RunAction::None;
          }
          Program program;
          CompileError compile_error;
          if (!Compile(store, &program, &compile_error)) {
            Fail(request, compile_error.message, outgoing);
            return RunAction::None;
          }
          debugger_.ClearBreakpoints();
          debugger_.ClearHistory(); // a new program invalidates recorded history
          vm_.Load(std::move(program));
          BuildLineMap(source);
          source_path_ = path;
          Respond(request, Json::Object(), outgoing);
          return RunAction::None;
        }
      }
      Respond(request, Json::Object(), outgoing);
      return RunAction::None;
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
            const int file_line = static_cast<int>(line->AsNumber());
            const int program_line = FileLineToProgramLine(file_line);
            Json item = Json::Object();
            item.Set("line", Json::Number(file_line));
            if (program_line == 0) {
              item.Set("verified", Json::Bool(false));
              item.Set("message", Json::Str("No BASIC line on this source line"));
            } else {
              debugger_.SetBreakpoint(program_line);
              const Json* condition = entry.Find("condition");
              if (condition != nullptr && condition->IsString()) {
                debugger_.SetCondition(program_line, condition->AsString());
              }
              item.Set("verified", Json::Bool(true));
            }
            verified.Push(std::move(item));
          }
        }
      }
      body.Set("breakpoints", std::move(verified));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
    }
    if (command == "configurationDone" || command == "continue") {
      Respond(request, Json::Object(), outgoing);
      PushContinued(outgoing);
      if (run_immediately)
        RunAndReport(outgoing);
      return RunAction::Continue;
    }
    if (command == "next" || command == "stepIn" || command == "stepOver") {
      Respond(request, Json::Object(), outgoing);
      PushContinued(outgoing);
      if (run_immediately)
        ReportStop(debugger_.StepLine(), "step", outgoing);
      return RunAction::StepLine;
    }
    if (command == "stepBack" || command == "reverseContinue") {
      // Time travel (M7): the debugger keeps a bounded ring of snapshots taken
      // at line boundaries, so these walk back through recorded history.
      Respond(request, Json::Object(), outgoing);
      PushContinued(outgoing);
      if (run_immediately) {
        const debug::StopState state =
            command == "stepBack" ? debugger_.StepBack() : debugger_.ReverseContinue();
        ReportStop(state, "step", outgoing);
      }
      return command == "stepBack" ? RunAction::StepBack : RunAction::ReverseContinue;
    }
    if (command == "pause") {
      // The reader thread has already set the debugger's pause flag; this is
      // the acknowledgement. The stopped event follows when Continue notices it.
      // A pause received while stopped must not stop the next Continue.
      debugger_.ClearPause();
      Respond(request, Json::Object(), outgoing);
      return RunAction::None;
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
      return RunAction::None;
    }
    if (command == "stackTrace") {
      Json body = Json::Object();
      Json frames = Json::Array();
      int id = 0;
      Json top = Json::Object();
      top.Set("id", Json::Number(id++));
      const int current_program_line = debugger_.CurrentLine();
      top.Set("name", Json::Str("line " + std::to_string(current_program_line)));
      top.Set("line", Json::Number(ProgramLineToFileLine(current_program_line)));
      top.Set("column", Json::Number(1));
      AddSource(&top);
      frames.Push(std::move(top));
      for (const Vm::CallFrame& frame : debugger_.CallFrames()) {
        Json item = Json::Object();
        item.Set("id", Json::Number(id++));
        const std::string label = frame.kind == Vm::CallFrame::Kind::Gosub
                                      ? "GOSUB line " + std::to_string(frame.line)
                                      : "FOR " + frame.name + " line " + std::to_string(frame.line);
        item.Set("name", Json::Str(label));
        item.Set("line", Json::Number(ProgramLineToFileLine(frame.line)));
        item.Set("column", Json::Number(1));
        AddSource(&item);
        frames.Push(std::move(item));
      }
      body.Set("stackFrames", std::move(frames));
      body.Set("totalFrames", Json::Number(id));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
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
      return RunAction::None;
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
      return RunAction::None;
    }
    if (command == "evaluate") {
      const std::string expression =
          arguments == nullptr ? std::string() : String(*arguments, "expression");
      // Exactly a variable name: a stop-gap until full expression evaluation.
      const std::optional<Value> value = debugger_.Variable(expression);
      if (!value.has_value()) {
        Fail(request, "No such variable: " + expression, outgoing);
        return RunAction::None;
      }
      Json body = Json::Object();
      body.Set("result", Json::Str(FormatValue(*value)));
      body.Set("variablesReference", Json::Number(0));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
    }
    if (command == "disconnect" || command == "terminate") {
      Respond(request, Json::Object(), outgoing);
      return RunAction::None;
    }
    Fail(request, "Unsupported request: " + command, outgoing);
    return RunAction::None;
  }

  // Executes the action a deferred request asked for. Session calls this after
  // it has written the response and continued event, so the client sees the
  // session enter its running state before the VM blocks.
  void RunActionNow(RunAction action, std::vector<Json>* outgoing) {
    switch (action) {
    case RunAction::None:
      return;
    case RunAction::Continue:
      RunAndReport(outgoing);
      return;
    case RunAction::StepLine:
      ReportStop(debugger_.StepLine(), "step", outgoing);
      return;
    case RunAction::StepBack:
      ReportStop(debugger_.StepBack(), "step", outgoing);
      return;
    case RunAction::ReverseContinue:
      ReportStop(debugger_.ReverseContinue(), "step", outgoing);
      return;
    }
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

  // DAP clients (VS Code) use physical editor lines, while the debugger's
  // breakpoints and StopState use BASIC line numbers. Build both directions
  // from the loaded source so a gutter breakpoint on editor line 2 maps to
  // BASIC line 20, and a stop at BASIC 20 is reported back as editor line 2.
  void ResetLineMap() {
    file_line_to_program_line_.clear();
    program_line_to_file_line_.clear();
    source_path_.clear();
  }

  void BuildLineMap(const std::string& text) {
    file_line_to_program_line_.clear();
    program_line_to_file_line_.clear();
    std::size_t pos = 0;
    while (pos < text.size()) {
      const std::size_t nl = text.find('\n', pos);
      const std::size_t end = nl == std::string::npos ? text.size() : nl;
      std::string line = text.substr(pos, end - pos);
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      const int number = ParseProgramLine(line);
      file_line_to_program_line_.push_back(number);
      if (number != 0)
        program_line_to_file_line_[number] = static_cast<int>(file_line_to_program_line_.size());
      pos = nl == std::string::npos ? text.size() : nl + 1;
    }
  }

  static int ParseProgramLine(const std::string& line) {
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
      ++i;
    if (i >= line.size() || !std::isdigit(static_cast<unsigned char>(line[i])))
      return 0;
    int number = 0;
    int digits = 0;
    while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])) && digits < 6) {
      number = number * 10 + (line[i] - '0');
      ++i;
      ++digits;
    }
    if (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])))
      return 0;
    if (number < 1)
      return 0;
    if (i < line.size() && line[i] != ' ' && line[i] != '\t')
      return 0;
    return number;
  }

  int FileLineToProgramLine(int file_line) const {
    // With no source map (for example an attach without a program), keep the
    // historical identity behaviour instead of rejecting every breakpoint.
    if (file_line_to_program_line_.empty())
      return file_line;
    if (file_line < 1 || file_line > static_cast<int>(file_line_to_program_line_.size()))
      return 0;
    return file_line_to_program_line_[static_cast<std::size_t>(file_line - 1)];
  }

  int ProgramLineToFileLine(int program_line) const {
    if (program_line_to_file_line_.empty())
      return program_line;
    const auto it = program_line_to_file_line_.find(program_line);
    return it == program_line_to_file_line_.end() ? program_line : it->second;
  }

  static std::string BaseName(const std::string& path) {
    const std::size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? path : path.substr(slash + 1);
  }

  void AddSource(Json* frame) const {
    if (source_path_.empty())
      return;
    Json source = Json::Object();
    source.Set("name", Json::Str(BaseName(source_path_)));
    source.Set("path", Json::Str(source_path_));
    frame->Set("source", std::move(source));
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
    BuildLineMap(text);
    source_path_.clear();
    Respond(request, Json::Object(), outgoing);
  }

  void RunAndReport(std::vector<Json>* outgoing) {
    ReportStop(debugger_.Continue(), "breakpoint", outgoing);
  }

  void PushContinued(std::vector<Json>* outgoing) {
    Json event = Event("continued");
    Json body = Json::Object();
    body.Set("threadId", Json::Number(1));
    body.Set("allThreadsContinued", Json::Bool(true));
    event.Set("body", std::move(body));
    outgoing->push_back(std::move(event));
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
    else if (state.reason == debug::StopReason::Pause)
      reason = "pause";
    body.Set("reason", Json::Str(reason));
    body.Set("threadId", Json::Number(1));
    body.Set("allThreadsStopped", Json::Bool(true));
    body.Set("line", Json::Number(ProgramLineToFileLine(state.line)));
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
  std::vector<int> file_line_to_program_line_;
  std::map<int, int> program_line_to_file_line_;
  std::string source_path_;
};

} // namespace newbasic::dap
