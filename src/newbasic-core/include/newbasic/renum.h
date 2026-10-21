#pragma once

#include <string>

namespace newbasic {

class LineStore;

// RENUM (plan.md 7b): renumber the stored program and rewrite the line references
// that follow ELSE, GOTO, GOSUB, THEN, ON ... GOTO, ON ... GOSUB, RESTORE, RESUME
// and RUN. The reference's defaults are the caller's business (10, first line, 10).
//
// References to lines that do not exist are left untouched and reported, as the
// reference describes ("Undefined line X in Y"). Reordering the program, or a line
// number above 65529, is Illegal function call.
struct RenumOutcome {
  bool ok = true;
  int error_code = 0;  // set when !ok
  std::string message; // set when !ok, or when `warned`
  bool warned = false; // a reference pointed at a line that does not exist
};

RenumOutcome RenumberProgram(LineStore* store, int new_start, int old_start, int increment);

} // namespace newbasic
