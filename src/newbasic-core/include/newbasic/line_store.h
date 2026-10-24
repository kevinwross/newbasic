#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace newbasic {

// Line numbers are signed 32-bit values. 0 is not a program line; GOTO/GOSUB
// targets may still round to 0 and are then reported as undefined.
inline constexpr int kMaxLineNumber = 2147483647;

// Parses a decimal line number without overflowing. Returns false for empty,
// non-numeric, or out-of-range input. The caller decides whether 0 is valid.
bool ParseLineNumberText(const std::string& text, int* out);

// Rounds a numeric expression to a line number (ties away from zero) and
// rejects values outside 0..kMaxLineNumber or non-finite values.
bool LineNumberFromNumber(double value, int* out);

// One program line: line number + raw source bytes (CP437 byte values
// preserved for LIST/SAVE round-trips; decoded to Unicode only for display).
struct SourceLine {
  int line_number = 0;
  std::string source; // raw bytes, no trailing newline
};

// Ordered line store (line number ascending). The ordered vector keeps the
// VM's line table and time-travel snapshots cheap (plan §3).
class LineStore {
public:
  // Adds or replaces a line. Returns false if the line number is out of
  // range (1..kMaxLineNumber, a signed 32-bit value).
  bool Add(int line_number, std::string source);
  bool Remove(int line_number);
  std::optional<std::size_t> FindIndex(int line_number) const;
  const std::vector<SourceLine>& Lines() const { return lines_; }
  std::size_t Size() const { return lines_.size(); }
  void Clear() { lines_.clear(); }

  // Parses plain-text .BAS input: physical lines of "<number><ws><source>",
  // LF or CRLF; blank lines ignored. Line numbers 1..kMaxLineNumber, unique.
  // Lines without a number are direct-mode statements (M1) and are an
  // error in file mode here. Returns false and fills *error_out (with the
  // offending physical line number, 1-based) on failure.
  static bool ParseTextBas(const std::string& text, LineStore* store, std::string* error_out);

  // Serializes the plain-text .BAS form (plan section 4): one
  // "<line number><tab><source>" line per program line, CRLF endings, CP437
  // source bytes preserved.
  std::string ToTextBas() const;

private:
  std::vector<SourceLine> lines_; // kept sorted ascending
};

} // namespace newbasic
