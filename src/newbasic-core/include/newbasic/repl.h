#pragma once

#include <string>
#include <vector>

#include "newbasic/frontend.h"
#include "newbasic/host.h"
#include "newbasic/lexer.h"
#include "newbasic/line_store.h"
#include "newbasic/screen/screen.h"
#include "newbasic/vm.h"

namespace newbasic {

// Direct-mode session (plan section 2): owns the program store, the VM and
// the screen, and processes one input line at a time. Numbered lines edit the
// program; other lines are commands (RUN/LIST/NEW/CLEAR/DELETE/SYSTEM) or
// direct statements, which execute in the same VM so variables persist.
class Repl {
public:
  Repl(screen::Screen& screen, Host* host = nullptr);

  // Processes one input line (numbered line, command or direct statement).
  RunResult Process(const std::string& line);

  // Prints "Ok", reads one line and processes it. Returns false when the
  // session should end (SYSTEM, or scripted input is exhausted).
  bool Step();

  Vm& Machine() { return vm_; }
  LineStore& Store() { return store_; }
  const Program& ProgramRef() const { return program_; }
  bool ShouldExit() const { return exit_; }
  void RequestExit() { exit_ = true; }

  // Rebuilds the compiled program from the line store.
  bool Recompile(CompileError* error);
  // RUN [line]: clears variables, loads the program and runs it.
  RunResult RunProgram(int start_line);

  void PrintLine(const std::string& text);
  // Prints "?<message>[ in <line>]" when the result is an error.
  void PrintError(const RunResult& result);

private:
  // Program files (plan section 4): SAVE writes plain text, LOAD reads it.
  RunResult SaveProgramFile(const std::string& path);
  RunResult LoadProgramFile(const std::string& path);
  bool HandleRun(const std::vector<Token>& tokens, RunResult* result);
  bool HandleList(const std::vector<Token>& tokens, RunResult* result);
  bool HandleDelete(const std::vector<Token>& tokens, RunResult* result);
  void ListRange(int start, int end, bool have_start, bool have_end);

  screen::Screen& screen_;
  Host* host_ = nullptr;
  LineStore store_;
  Vm vm_;
  Program program_;
  bool exit_ = false;
  // True when the statement just processed ran a program, so the readiness marker is
  // printed afterwards instead of before every line.
  bool ran_program_ = false;
  bool first_step_ = true;
};

} // namespace newbasic
