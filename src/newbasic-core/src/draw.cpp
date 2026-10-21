#include "newbasic/draw.h"

#include <cctype>
#include <cmath>
#include <string>

namespace newbasic {
namespace {

bool IsSpace(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

// Reads an optional unsigned integer, reporting the sign separately. `signed_value`
// (when given) says whether a sign was present at all, which is what makes an M move
// relative.
bool ReadNumber(const std::string& macro, std::size_t* pos, int* value, bool* negative,
                bool* signed_value = nullptr) {
  std::size_t i = *pos;
  while (i < macro.size() && IsSpace(macro[i]))
    ++i;
  bool minus = false;
  bool saw_sign = false;
  if (i < macro.size() && (macro[i] == '-' || macro[i] == '+')) {
    minus = macro[i] == '-';
    saw_sign = true;
    ++i;
  }
  if (i >= macro.size() || std::isdigit(static_cast<unsigned char>(macro[i])) == 0)
    return false;
  int result = 0;
  while (i < macro.size() && std::isdigit(static_cast<unsigned char>(macro[i])) != 0) {
    result = result * 10 + (macro[i] - '0');
    if (result > 32767)
      return false;
    ++i;
  }
  *pos = i;
  *value = result;
  if (negative != nullptr)
    *negative = minus;
  if (signed_value != nullptr)
    *signed_value = saw_sign;
  return true;
}

// Rotates a step by the current angle. Screen y grows downward, so a positive angle
// turns the step counter-clockwise on screen.
void Rotate(double angle_degrees, int* dx, int* dy) {
  const double radians = angle_degrees * 3.14159265358979323846 / 180.0;
  const double cos_a = std::cos(radians);
  const double sin_a = std::sin(radians);
  const double x = static_cast<double>(*dx) * cos_a + static_cast<double>(*dy) * sin_a;
  const double y = -static_cast<double>(*dx) * sin_a + static_cast<double>(*dy) * cos_a;
  *dx = static_cast<int>(std::lround(x));
  *dy = static_cast<int>(std::lround(y));
}

} // namespace

bool ExecuteDrawMacro(screen::Canvas* canvas, const std::string& macro, DrawState* state,
                      std::string* error) {
  const auto fail = [error](const std::string& message) {
    if (error != nullptr)
      *error = message;
    return false;
  };
  std::size_t i = 0;
  // B and N are prefixes: they apply to the next movement command only.
  bool blank_move = false;
  bool return_home = false;
  while (i < macro.size()) {
    if (IsSpace(macro[i])) {
      ++i;
      continue;
    }
    const char raw = macro[i];
    const char command = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    ++i;
    if (command == 'B') {
      blank_move = true;
      continue;
    }
    if (command == 'N') {
      return_home = true;
      continue;
    }

    const bool plot = !blank_move;
    const bool home_after = return_home;
    blank_move = false;
    return_home = false;

    switch (command) {
    case 'U':
    case 'D':
    case 'L':
    case 'R':
    case 'E':
    case 'F':
    case 'G':
    case 'H': {
      int distance = 1;
      ReadNumber(macro, &i, &distance, nullptr); // omitted means one unit
      const double factor = static_cast<double>(state->scale) / 4.0;
      const int step = static_cast<int>(std::lround(static_cast<double>(distance) * factor));
      int dx = 0;
      int dy = 0;
      switch (command) {
      case 'U':
        dy = -step;
        break;
      case 'D':
        dy = step;
        break;
      case 'L':
        dx = -step;
        break;
      case 'R':
        dx = step;
        break;
      case 'E':
        dx = step;
        dy = -step;
        break;
      case 'F':
        dx = step;
        dy = step;
        break;
      case 'G':
        dx = -step;
        dy = step;
        break;
      default:
        dx = -step;
        dy = -step;
        break; // H
      }
      Rotate(state->angle, &dx, &dy);
      const int start_x = state->x;
      const int start_y = state->y;
      if (plot) {
        canvas->DrawLine(start_x, start_y, start_x + dx, start_y + dy,
                         static_cast<std::uint8_t>(state->colour));
      }
      if (!home_after) {
        state->x = start_x + dx;
        state->y = start_y + dy;
      }
      break;
    }
    case 'M': {
      int x = 0;
      int y = 0;
      bool negative_x = false;
      bool negative_y = false;
      bool signed_x = false;
      if (!ReadNumber(macro, &i, &x, &negative_x, &signed_x))
        return fail("M needs x,y");
      while (i < macro.size() && IsSpace(macro[i]))
        ++i;
      if (i >= macro.size() || macro[i] != ',')
        return fail("M needs a comma between x and y");
      ++i;
      if (!ReadNumber(macro, &i, &y, &negative_y))
        return fail("M needs y");
      // A sign on x makes the move relative in both axes, per the reference; each
      // axis keeps its own sign.
      const int dx = negative_x ? -x : x;
      const int dy = negative_y ? -y : y;
      const int target_x = signed_x ? state->x + dx : x;
      const int target_y = signed_x ? state->y + dy : y;
      if (plot) {
        canvas->DrawLine(state->x, state->y, target_x, target_y,
                         static_cast<std::uint8_t>(state->colour));
      }
      if (!home_after) {
        state->x = target_x;
        state->y = target_y;
      }
      break;
    }
    case 'A': {
      int quarter_turns = 0;
      if (!ReadNumber(macro, &i, &quarter_turns, nullptr))
        return fail("A needs a value");
      if (quarter_turns < 0 || quarter_turns > 3)
        return fail("A must be 0 to 3");
      state->angle = 90.0 * quarter_turns;
      break;
    }
    case 'T': {
      // TA n: turn by n degrees, positive counter-clockwise.
      if (i >= macro.size() || std::toupper(static_cast<unsigned char>(macro[i])) != 'A') {
        return fail("T must be followed by A");
      }
      ++i;
      int turn = 0;
      bool negative = false;
      if (!ReadNumber(macro, &i, &turn, &negative))
        return fail("TA needs an angle");
      state->angle += negative ? -turn : turn;
      break;
    }
    case 'C': {
      int colour = 0;
      if (!ReadNumber(macro, &i, &colour, nullptr))
        return fail("C needs a colour");
      if (colour > 15)
        return fail("colour out of range");
      state->colour = colour;
      break;
    }
    case 'S': {
      int scale = 0;
      if (!ReadNumber(macro, &i, &scale, nullptr))
        return fail("S needs a scale");
      if (scale < 1 || scale > 255)
        return fail("scale must be 1 to 255");
      state->scale = scale;
      break;
    }
    case 'P': {
      int paint = 0;
      int boundary = 0;
      if (!ReadNumber(macro, &i, &paint, nullptr))
        return fail("P needs paint,boundary");
      while (i < macro.size() && IsSpace(macro[i]))
        ++i;
      if (i >= macro.size() || macro[i] != ',')
        return fail("P needs a comma");
      ++i;
      if (!ReadNumber(macro, &i, &boundary, nullptr))
        return fail("P needs a boundary colour");
      if (paint > 15 || boundary > 15)
        return fail("colour out of range");
      // The reference fills the figure; filling from the current position is the
      // honest approximation (documented in the design note).
      canvas->FloodFill(state->x, state->y, static_cast<std::uint8_t>(paint), boundary);
      break;
    }
    default:
      return fail(std::string("unknown DRAW command '") + raw + "'");
    }
  }
  return true;
}

} // namespace newbasic
