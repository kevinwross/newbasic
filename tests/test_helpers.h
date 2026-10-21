#pragma once

// Shared helpers for the M1 language tests: run a BASIC program on a
// headless screen and inspect the rendered grid, compile errors and run
// errors.
#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <vector>

#include "newbasic/frontend.h"
#include "newbasic/host.h"
#include "newbasic/lexer.h"
#include "newbasic/line_store.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"

namespace lwtest {

// A temporary file name unique to this process, so a leftover file from an
// interrupted run cannot change this run's result (the isolation item in plan.md
// 7b). The same base always yields the same name within a process, so a test can
// write and read back one file; the map keeps the c_str pointers stable.
inline const char* TempName(const char* base) {
  static std::map<std::string, std::string> names;
  auto found = names.find(base);
  if (found == names.end()) {
    found = names.emplace(base, std::string(base) + "." + std::to_string(names.size() + 1)).first;
  }
  return found->second.c_str();
}

// Tokenizes one line (helper for lexer-facing tests).
inline std::vector<newbasic::Token> TokenizeLine(const std::string& line) {
  std::vector<newbasic::Token> tokens;
  std::string error;
  int position = 0;
  if (!newbasic::Lexer::Tokenize(line, &tokens, &error, &position)) {
    FAIL("Tokenize failed at " << position << ": " << error);
  }
  return tokens;
}

inline bool CompileSource(const std::string& source, newbasic::Program* program,
                          newbasic::CompileError* error) {
  newbasic::LineStore store;
  std::string parse_error;
  if (!newbasic::LineStore::ParseTextBas(source, &store, &parse_error)) {
    error->line = 0;
    error->code = newbasic::kErrSyntax;
    error->message = parse_error;
    return false;
  }
  return newbasic::Compile(store, program, error);
}

// Runs a program; returns the rendered grid rows (trailing blanks trimmed).
inline std::vector<std::string> RunRowsWithInput(const std::string& source,
                                                 const std::vector<std::wstring>& input,
                                                 newbasic::Host* host = nullptr) {
  newbasic::Program program;
  newbasic::CompileError error;
  if (!CompileSource(source, &program, &error)) {
    FAIL("Compile failed (line " << error.line << ", code " << error.code
                                 << "): " << error.message);
  }
  newbasic::screen::HeadlessScreen screen;
  for (const std::wstring& line : input)
    screen.ScriptLine(line);
  newbasic::Vm vm(screen, host);
  vm.Load(std::move(program));
  const newbasic::RunResult result = vm.Run();
  if (result.status == newbasic::RunResult::Status::Error) {
    FAIL("Run error " << result.code << " in line " << result.line << ": " << result.message);
  }
  return screen.RenderRows();
}

inline std::vector<std::string> RunRows(const std::string& source) {
  return RunRowsWithInput(source, {}, nullptr);
}

// First rendered row (what a single PRINT produces).
inline std::string Row0(const std::string& source) {
  const std::vector<std::string> rows = RunRows(source);
  return rows.empty() ? std::string() : rows[0];
}

// Runtime error code, or 0 when the program runs cleanly.
inline int RunErrorCode(const std::string& source, int* out_line = nullptr,
                        const std::vector<std::wstring>& input = {}) {
  newbasic::Program program;
  newbasic::CompileError error;
  if (!CompileSource(source, &program, &error)) {
    if (out_line != nullptr)
      *out_line = error.line;
    return error.code;
  }
  newbasic::screen::HeadlessScreen screen;
  for (const std::wstring& line : input)
    screen.ScriptLine(line);
  newbasic::Vm vm(screen);
  vm.Load(std::move(program));
  const newbasic::RunResult result = vm.Run();
  if (out_line != nullptr)
    *out_line = result.line;
  return result.status == newbasic::RunResult::Status::Error ? result.code : 0;
}

// Compile error code, or 0 when compilation succeeds.
inline int CompileCode(const std::string& source, int* out_line = nullptr) {
  newbasic::Program program;
  newbasic::CompileError error;
  if (CompileSource(source, &program, &error))
    return 0;
  if (out_line != nullptr)
    *out_line = error.line;
  return error.code;
}

} // namespace lwtest
