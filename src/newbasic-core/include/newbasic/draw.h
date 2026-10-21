#pragma once

#include <string>

#include "newbasic/screen/canvas.h"

namespace newbasic {

// Graphics Macro Language interpreter for DRAW (M12 phase 2).
//
// Commands: U D L R E F G H (movement, optional distance), M x,y (absolute, or
// relative with a sign), B (move without plotting), N (return to the start), A n
// (angle in 90-degree steps), TA n (turn by n degrees), C n (colour), S n (scale,
// distance times n/4) and P paint,boundary (fill from the current position).
//
// The `X variable` form is handled by the VM, which expands the variable before
// calling this: the interpreter deliberately knows nothing about variables.
struct DrawState {
  int x = 160;
  int y = 100;
  int colour = 1;
  int scale = 4; // distance multiplier is scale/4
  double angle = 0.0;
};

// Executes `macro` from `state`, updating it. Returns false and fills *error when a
// command is malformed or out of range.
bool ExecuteDrawMacro(screen::Canvas* canvas, const std::string& macro, DrawState* state,
                      std::string* error);

} // namespace newbasic
