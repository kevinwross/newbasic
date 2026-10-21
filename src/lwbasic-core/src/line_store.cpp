#include "lwbasic/line_store.h"

#include <algorithm>
#include <cctype>

namespace lwbasic {
namespace {
constexpr int kMaxLine = 999999;
} // namespace

bool LineStore::Add(int line_number, std::string source) {
  if (line_number < 1 || line_number > kMaxLine)
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
    int num = 0;
    int digits = 0;
    while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])) && digits < 6) {
      num = num * 10 + (line[i] - '0');
      ++i;
      ++digits;
    }
    if (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
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

} // namespace lwbasic
