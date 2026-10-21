#pragma once

#include <string>

#include "lwbasic/errors.h"
#include "lwbasic/line_store.h"
#include "lwbasic/vm.h"

namespace lwbasic {

struct CompileError {
  int line = 0;
  int code = kErrSyntax;
  std::string message;
  int column = 0;
};

// Compiles the whole line store into a Program (M1 language subset:
// expressions with the dialect's operator precedence, LET assignments,
// PRINT, INPUT/LINE INPUT, IF/THEN/ELSE, FOR/NEXT, WHILE/WEND,
// GOTO/GOSUB/RETURN, ON ... GOTO/GOSUB, CLS/LOCATE/COLOR, RANDOMIZE, SWAP,
// END/STOP/REM/BEEP and the built-in functions). Statements from later
// milestones compile to an "Advanced Feature" error at run time so the
// parser stays dialect-complete.
bool Compile(const LineStore& store, Program* out, CompileError* error);

// Compiles one direct-mode statement list (no line number) for immediate
// execution. Statements that are illegal in direct mode report error 12.
bool CompileDirect(const std::string& text, Program* out, CompileError* error);

// True when a statement keyword may not be used in direct mode.
bool IsIllegalDirect(const std::string& keyword);

} // namespace lwbasic
