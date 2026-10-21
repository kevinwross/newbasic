#include "lwbasic/renum.h"

#include <cctype>
#include <map>
#include <string>
#include <vector>

#include "lwbasic/errors.h"
#include "lwbasic/line_store.h"

namespace lwbasic {
namespace {

constexpr int kMaxLine = 65529;

std::string Upper(const std::string& text) {
  std::string out = text;
  for (char& c : out)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

bool IsWordStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) != 0;
}
bool IsWordChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '.' || c == '$' || c == '%';
}

// Words after which a line number may follow.
bool ExpectsLineNumber(const std::string& word) {
  return word == "GOTO" || word == "GOSUB" || word == "THEN" || word == "ELSE" ||
         word == "RESTORE" || word == "RESUME" || word == "RUN";
}

// Rewrites line-number references in one source line. Strings and REM comments are
// skipped so that PRINT "GOTO 100" and REM GOTO 100 are left alone. When `map`
// lacks a line, the number is left untouched and *undefined is set to it.
std::string RewriteReferences(const std::string& source, const std::map<int, int>& map,
                              int* undefined) {
  std::string out;
  out.reserve(source.size());
  bool expect_line = false;
  std::size_t i = 0;
  while (i < source.size()) {
    const char c = source[i];
    if (c == '"') { // string literal: copy verbatim
      out += c;
      ++i;
      while (i < source.size()) {
        out += source[i];
        const bool done = source[i] == '"';
        ++i;
        if (done)
          break;
      }
      expect_line = false;
      continue;
    }
    if (IsWordStart(c)) {
      const std::size_t start = i;
      while (i < source.size() && IsWordChar(source[i]))
        ++i;
      const std::string word = Upper(source.substr(start, i - start));
      if (word == "REM") { // comment: rest of the line verbatim
        out += source.substr(start);
        break;
      }
      out += source.substr(start, i - start);
      expect_line = ExpectsLineNumber(word);
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
      const std::size_t start = i;
      while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])) != 0)
        ++i;
      const std::string digits = source.substr(start, i - start);
      if (expect_line) {
        const int number = std::stoi(digits);
        const auto found = map.find(number);
        if (found != map.end()) {
          out += std::to_string(found->second);
        } else {
          out += digits;
          if (undefined != nullptr)
            *undefined = number;
        }
        // A comma continues a list (ON n GOTO 100,200); anything else ends it.
        expect_line = i < source.size() && source[i] == ',';
      } else {
        out += digits;
      }
      continue;
    }
    if (c == ',') {
      out += c;
      ++i;
      continue; // expect_line survives a comma for ON ... GOTO lists
    }
    if (!std::isspace(static_cast<unsigned char>(c)))
      expect_line = false;
    out += c;
    ++i;
  }
  return out;
}

} // namespace

RenumOutcome RenumberProgram(LineStore* store, int new_start, int old_start, int increment) {
  RenumOutcome outcome;
  if (store == nullptr) {
    outcome.ok = false;
    outcome.error_code = kErrIllegalFunctionCall;
    outcome.message = "no program";
    return outcome;
  }
  if (increment < 1 || new_start < 0 || new_start > kMaxLine || old_start < 0) {
    outcome.ok = false;
    outcome.error_code = kErrIllegalFunctionCall;
    outcome.message = "invalid RENUM arguments";
    return outcome;
  }

  // Assign new numbers to the lines from old_start upward, in order.
  std::map<int, int> map;
  int next = new_start;
  int last_kept = -1;
  std::vector<SourceLine> kept;
  std::vector<SourceLine> moving;
  for (const SourceLine& line : store->Lines()) {
    if (line.line_number < old_start) {
      kept.push_back(line);
      last_kept = line.line_number;
    } else {
      if (next > kMaxLine) {
        outcome.ok = false;
        outcome.error_code = kErrIllegalFunctionCall;
        outcome.message = "line number above 65529";
        return outcome;
      }
      map[line.line_number] = next;
      moving.push_back(line);
      next += increment;
    }
  }
  if (!moving.empty() && map.begin()->second <= last_kept) {
    // Renumbering would reorder the program, which the reference forbids.
    outcome.ok = false;
    outcome.error_code = kErrIllegalFunctionCall;
    outcome.message = "RENUM would reorder the program";
    return outcome;
  }

  // Rewrite references in every line, including the ones keeping their numbers.
  LineStore rebuilt;
  int undefined = 0;
  for (const SourceLine& line : kept) {
    rebuilt.Add(line.line_number, RewriteReferences(line.source, map, &undefined));
  }
  for (const SourceLine& line : moving) {
    const std::string source = RewriteReferences(line.source, map, &undefined);
    rebuilt.Add(map[line.line_number], source);
  }
  if (undefined != 0) {
    outcome.warned = true;
    outcome.message = "Undefined line " + std::to_string(undefined);
  }
  *store = std::move(rebuilt);
  return outcome;
}

} // namespace lwbasic
