#include "newbasic/line_store.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>

namespace newbasic {
namespace {
constexpr std::int64_t kMaxLine64 = 2147483647;
} // namespace

bool ParseLineNumberText(const std::string& text, int* out) {
  if (text.empty())
    return false;
  std::int64_t value = 0;
  for (char c : text) {
    if (c < '0' || c > '9')
      return false;
    value = value * 10 + (c - '0');
    if (value > kMaxLine64)
      return false;
  }
  *out = static_cast<int>(value);
  return true;
}

bool LineNumberFromNumber(double value, int* out) {
  if (!std::isfinite(value) || value < 0.0 || value > static_cast<double>(kMaxLineNumber))
    return false;
  const double rounded = value < 0.0 ? -std::floor(-value + 0.5) : std::floor(value + 0.5);
  if (rounded > static_cast<double>(kMaxLineNumber))
    return false;
  *out = static_cast<int>(rounded);
  return true;
}

bool LineStore::Add(int line_number, std::string source) {
  if (line_number < 1 || line_number > kMaxLineNumber)
    return false;
  auto it = std::lower_bound(lines_.begin(), lines_.end(), line_number,
                             [](const SourceLine& l, int n) { return l.line_number < n; });
  if (it != lines_.end() && it->line_number == line_number) {
    it->source = std::move(source);
    return true;
  }
  lines_.insert(it, SourceLine{line_number, std::move(source)});
  return true;
}

bool LineStore::Remove(int line_number) {
  std::optional<std::size_t> idx = FindIndex(line_number);
  if (!idx)
    return false;
  lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(*idx));
  return true;
}

std::optional<std::size_t> LineStore::FindIndex(int line_number) const {
  auto it = std::lower_bound(lines_.begin(), lines_.end(), line_number,
                             [](const SourceLine& l, int n) { return l.line_number < n; });
  if (it != lines_.end() && it->line_number == line_number) {
    return static_cast<std::size_t>(std::distance(lines_.begin(), it));
  }
  return std::nullopt;
}

std::string LineStore::ToTextBas() const {
  std::string out;
  for (const SourceLine& line : lines_) {
    out += std::to_string(line.line_number);
    out += '\t';
    out += line.source;
    out += "\r\n";
  }
  return out;
}

bool LineStore::ParseTextBas(const std::string& text, LineStore* store, std::string* error_out) {
  std::size_t pos = 0;
  std::size_t phys = 0;
  while (pos < text.size()) {
    const std::size_t nl = text.find('\n', pos);
    const std::size_t end = nl == std::string::npos ? text.size() : nl;
    std::string line = text.substr(pos, end - pos);
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
    ++phys;

    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
      ++i;
    if (i >= line.size())
      continue; // blank line

    if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
      *error_out = "line " + std::to_string(phys) + ": missing line number (direct mode is M1)";
      return false;
    }
    const std::size_t digits_begin = i;
    while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])))
      ++i;
    int num = 0;
    if (!ParseLineNumberText(line.substr(digits_begin, i - digits_begin), &num)) {
      *error_out = "line " + std::to_string(phys) + ": line number too large";
      return false;
    }
    if (num < 1) {
      *error_out = "line " + std::to_string(phys) + ": line number must be >= 1";
      return false;
    }
    if (i < line.size() && line[i] != ' ' && line[i] != '\t') {
      *error_out = "line " + std::to_string(phys) + ": expected space or tab after line number";
      return false;
    }
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
      ++i;
    if (store->FindIndex(num)) {
      *error_out =
          "line " + std::to_string(phys) + ": duplicate line number " + std::to_string(num);
      return false;
    }
    store->Add(num, line.substr(i));
  }
  return true;
}

} // namespace newbasic
