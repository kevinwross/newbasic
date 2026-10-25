#pragma once

#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
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
#include "newbasic/program_files.h"
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
  enum class RunAction {
    None,
    Continue,
    StepLine,
    StepOver,
    StepOut,
    StepBack,
    ReverseContinue
  };

  Handler(Vm& vm, debug::Debugger& debugger, bool history = true)
      : vm_(vm), debugger_(debugger), history_enabled_(history) {
    debugger_.SetHistoryEnabled(history);
  }

  void SetPauseFlag(std::atomic<bool>* flag) { debugger_.SetPauseFlag(flag); }

  // Protocol message sequence numbers are shared with the reader thread, which
  // answers the simple `threads` request directly while the VM is running.
  int NextSeq() { return seq_++; }

  // Handles one request; appends responses/events to `outgoing`. When
  // run_immediately is false, a run/step action is returned to the caller
  // instead of being executed here.
  RunAction Handle(const Json& request, std::vector<Json>* outgoing, bool run_immediately = true) {
    const std::string command = String(request, "command");
    const Json* arguments = request.Find("arguments");
    if (command == "initialize") {
      Json body = Json::Object();
      body.Set("supportsConfigurationDoneRequest", Json::Bool(true));
      body.Set("supportsConditionalBreakpoints", Json::Bool(true));
      body.Set("supportsSetVariable", Json::Bool(true));
      body.Set("supportsEvaluateForHovers", Json::Bool(true));
      body.Set("supportsStepBack", Json::Bool(history_enabled_));
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
        if (path.empty())
          return RunAction::None;
        {
          const std::string source = ReadFile(path);
          LineStore store;
          int error_code = 0;
          std::string error;
          if (!LoadProgramFromFile(path, &store, &error_code, &error)) {
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
          vm_.Reset();              // do not carry variables/arrays into a new launch
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
      std::string request_source;
      if (arguments != nullptr) {
        const Json* source = arguments->Find("source");
        if (source != nullptr)
          request_source = String(*source, "path");
      }
      // VS Code can send setBreakpoints for every open source, in any order.
      // Only the loaded program's source may change adapter breakpoint state;
      // a request for another editor must not resurrect old breakpoints.
      if (!source_path_.empty() && !request_source.empty() &&
          !SameSource(source_path_, request_source)) {
        Json body = Json::Object();
        Json verified = Json::Array();
        const Json* list = arguments->Find("breakpoints");
        if (list != nullptr && list->IsArray()) {
          for (const Json& entry : list->Items()) {
            if (!entry.IsObject())
              continue;
            const Json* line = entry.Find("line");
            if (line == nullptr || !line->IsNumber())
              continue;
            Json item = Json::Object();
            item.Set("line", Json::Number(line->AsNumber()));
            item.Set("verified", Json::Bool(false));
            item.Set("message", Json::Str("Breakpoint is for a different source"));
            verified.Push(std::move(item));
          }
        }
        body.Set("breakpoints", std::move(verified));
        Respond(request, std::move(body), outgoing);
        return RunAction::None;
      }
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
      const bool step_over = command != "stepIn";
      Respond(request, Json::Object(), outgoing);
      PushContinued(outgoing);
      if (run_immediately)
        ReportStop(step_over ? debugger_.StepOver() : debugger_.StepLine(), "step", outgoing);
      return step_over ? RunAction::StepOver : RunAction::StepLine;
    }
    if (command == "stepOut") {
      Respond(request, Json::Object(), outgoing);
      PushContinued(outgoing);
      if (run_immediately)
        ReportStop(debugger_.StepOut(), "step", outgoing);
      return RunAction::StepOut;
    }
    if (command == "stepBack" || command == "reverseContinue") {
      if (!history_enabled_) {
        Fail(request, "Step Back is disabled; start the adapter with --history.", outgoing);
        return RunAction::None;
      }
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
        const std::string label =
            frame.kind == Vm::CallFrame::Kind::Gosub
                ? "GOSUB line " + std::to_string(frame.line)
                : (frame.kind == Vm::CallFrame::Kind::For_
                       ? "FOR " + frame.name + " line " + std::to_string(frame.line)
                       : (frame.function ? "FUNCTION " : "SUB ") + frame.name +
                             " line " + std::to_string(frame.line));
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
      locals.Set("variablesReference", Json::Number(2));
      locals.Set("expensive", Json::Bool(false));
      scopes.Push(std::move(locals));
      Json globals = Json::Object();
      globals.Set("name", Json::Str("Globals"));
      globals.Set("variablesReference", Json::Number(1));
      globals.Set("expensive", Json::Bool(false));
      scopes.Push(std::move(globals));
      body.Set("scopes", std::move(scopes));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
    }
    if (command == "variables") {
      const int reference =
          arguments == nullptr ? 1 : static_cast<int>(Number(*arguments, "variablesReference"));
      Json body = Json::Object();
      Json variables = Json::Array();
      const std::vector<Vm::ArrayInfo> arrays = debugger_.Arrays();
      if (reference >= kArrayReferenceBase) {
        const std::size_t index = static_cast<std::size_t>(reference - kArrayReferenceBase);
        if (index < arrays.size())
          AppendArrayElements(arrays[index], &variables);
      } else if (reference == 2) {
        for (const auto& entry : debugger_.LocalVariables()) {
          Json item = Json::Object();
          item.Set("name", Json::Str(entry.first));
          item.Set("value", Json::Str(FormatValue(entry.second)));
          item.Set("variablesReference", Json::Number(0));
          variables.Push(std::move(item));
        }
      } else {
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
        // Arrays are a separate VM table. Give each summary row a child
        // reference so the client can expand it into individual elements.
        for (std::size_t i = 0; i < arrays.size(); ++i) {
          const Vm::ArrayInfo& array = arrays[i];
          Json item = Json::Object();
          item.Set("name", Json::Str(array.name));
          item.Set("value", Json::Str(FormatArray(array)));
          item.Set("type", Json::Str("array"));
          item.Set("variablesReference", Json::Number(kArrayReferenceBase + static_cast<int>(i)));
          item.Set("indexedVariables", Json::Number(static_cast<int>(array.elements)));
          variables.Push(std::move(item));
        }
      }
      body.Set("variables", std::move(variables));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
    }
    if (command == "setVariable") {
      if (arguments == nullptr) {
        Fail(request, "setVariable needs arguments", outgoing);
        return RunAction::None;
      }
      const int reference = static_cast<int>(Number(*arguments, "variablesReference"));
      const std::string name = String(*arguments, "name");
      const std::string text = String(*arguments, "value");
      if (reference != 1 && reference != 2) {
        Fail(request, "Only Locals/Globals variables can be edited", outgoing);
        return RunAction::None;
      }
      const std::optional<Value> current =
          reference == 2 ? debugger_.LocalVariable(name) : debugger_.Variable(name);
      if (!current.has_value()) {
        Fail(request, "No such variable: " + name, outgoing);
        return RunAction::None;
      }
      Value updated;
      if (current->IsString()) {
        updated = Value::Str(Unquote(text));
      } else {
        double number = 0.0;
        if (!ParseNumber(text, &number)) {
          Fail(request, "Not a number: " + text, outgoing);
          return RunAction::None;
        }
        const char suffix = VarTypeSuffix(CanonicalVarName(name));
        if (suffix == '%' && (number < -2147483648.0 || number > 2147483647.0)) {
          Fail(request, "Integer variable out of range", outgoing);
          return RunAction::None;
        }
        if (suffix == '&' && (number < -2147483648.0 || number > 2147483647.0)) {
          Fail(request, "Long variable out of range", outgoing);
          return RunAction::None;
        }
        updated = Value::Sgl(number);
      }
      if (reference == 2)
        debugger_.SetLocalVariable(name, updated);
      else
        vm_.SetVar(name, updated);
      const std::optional<Value> result =
          reference == 2 ? debugger_.LocalVariable(name) : debugger_.Variable(name);
      Json body = Json::Object();
      body.Set("value", Json::Str(result.has_value() ? FormatValue(*result) : text));
      body.Set("variablesReference", Json::Number(0));
      Respond(request, std::move(body), outgoing);
      return RunAction::None;
    }
    if (command == "evaluate") {
      const std::string expression =
          arguments == nullptr ? std::string() : String(*arguments, "expression");
      // Exactly a variable name: a stop-gap until full expression evaluation.
      std::optional<Value> value = debugger_.LocalVariable(expression);
      if (!value.has_value())
        value = debugger_.Variable(expression);
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
    case RunAction::StepOver:
      ReportStop(debugger_.StepOver(), "step", outgoing);
      return;
    case RunAction::StepOut:
      ReportStop(debugger_.StepOut(), "step", outgoing);
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
  static constexpr int kArrayReferenceBase = 1000;
  static constexpr std::size_t kMaxArrayElements = 1000;

  static std::string String(const Json& object, const char* key) {
    const Json* value = object.Find(key);
    return value != nullptr && value->IsString() ? value->AsString() : std::string();
  }

  static bool SameSource(const std::string& first, const std::string& second) {
    if (first.empty() || second.empty())
      return true;
    std::error_code error;
    std::filesystem::path a = std::filesystem::weakly_canonical(first, error);
    error.clear();
    std::filesystem::path b = std::filesystem::weakly_canonical(second, error);
    std::string left = a.lexically_normal().string();
    std::string right = b.lexically_normal().string();
#if defined(_WIN32)
    for (char& c : left)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : right)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
#endif
    return left == right;
  }

  static std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
      return std::string();
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
  }

  static std::string FormatArray(const Vm::ArrayInfo& array) {
    std::ostringstream text;
    text << "ARRAY(";
    for (std::size_t i = 0; i < array.dims.size(); ++i) {
      if (i != 0)
        text << ", ";
      text << array.base << ".." << array.dims[i];
    }
    text << ")  " << array.elements << " elements";
    return text.str();
  }

  void AppendArrayElements(const Vm::ArrayInfo& array, Json* variables) {
    if (array.dims.empty())
      return;
    const std::size_t limit =
        array.elements < kMaxArrayElements ? array.elements : kMaxArrayElements;
    std::vector<int> subscripts(array.dims.size(), array.base);
    for (std::size_t flat = 0; flat < limit; ++flat) {
      std::size_t remaining = flat;
      for (std::size_t i = array.dims.size(); i-- > 0;) {
        const int extent = array.dims[i] - array.base + 1;
        if (extent <= 0)
          return;
        subscripts[i] = array.base + static_cast<int>(remaining % static_cast<std::size_t>(extent));
        remaining /= static_cast<std::size_t>(extent);
      }
      const std::optional<Value> value = debugger_.ArrayElement(array.name, subscripts);
      if (!value.has_value())
        continue;
      std::ostringstream name;
      name << array.name << "(";
      for (std::size_t i = 0; i < subscripts.size(); ++i) {
        if (i != 0)
          name << ",";
        name << subscripts[i];
      }
      name << ")";
      Json item = Json::Object();
      item.Set("name", Json::Str(name.str()));
      item.Set("value", Json::Str(FormatValue(*value)));
      item.Set("variablesReference", Json::Number(0));
      variables->Push(std::move(item));
    }
    if (array.elements > limit) {
      Json item = Json::Object();
      item.Set("name", Json::Str(array.name + "(...)"));
      item.Set("value",
               Json::Str("... " + std::to_string(array.elements - limit) + " more elements"));
      item.Set("variablesReference", Json::Number(0));
      variables->Push(std::move(item));
    }
  }

  static std::string FormatValue(const Value& value) {
    if (value.IsString())
      return "\"" + value.AsString() + "\"";
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%g", value.AsNumber());
    return buffer;
  }

  static bool ParseNumber(const std::string& text, double* out) {
    if (text.empty())
      return false;
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end == text.c_str())
      return false;
    while (*end == ' ' || *end == '\t')
      ++end;
    if (*end != '\0' || !std::isfinite(value))
      return false;
    *out = value;
    return true;
  }

  static std::string Unquote(std::string text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
      text = text.substr(1, text.size() - 2);
    return text;
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
    int error_code = 0;
    std::string error;
    std::error_code cwd_error;
    const std::string base_dir = std::filesystem::current_path(cwd_error).string();
    if (!LoadProgramFromText(text, cwd_error ? "." : base_dir, &store, &error_code, &error)) {
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
    vm_.Reset();              // do not carry variables/arrays into a new launch
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
  bool history_enabled_ = true;
  std::atomic<int> seq_{1};
  std::vector<int> file_line_to_program_line_;
  std::map<int, int> program_line_to_file_line_;
  std::string source_path_;
};

} // namespace newbasic::dap
