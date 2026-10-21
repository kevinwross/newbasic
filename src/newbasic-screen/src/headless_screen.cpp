#include "newbasic/screen/headless_screen.h"

#include "newbasic/screen/screen.h"

namespace newbasic::screen {

std::wstring HeadlessScreen::ReadLine() {
  if (lines_.empty())
    return {};
  std::wstring line = std::move(lines_.front());
  lines_.erase(lines_.begin());
  return line;
}

bool HeadlessScreen::TryGetKey(wchar_t* key) {
  if (keys_.empty())
    return false;
  *key = keys_.front();
  keys_.erase(keys_.begin());
  return true;
}

std::vector<std::string> HeadlessScreen::RenderRows() const {
  std::vector<std::string> rows;
  rows.reserve(static_cast<std::size_t>(Height()));
  for (int r = 0; r < Height(); ++r) {
    std::wstring row;
    row.reserve(static_cast<std::size_t>(Width()));
    for (int c = 0; c < Width(); ++c) {
      row += CellAt(r, c);
    }
    std::string bytes = EncodeCp437(row);
    while (!bytes.empty() && bytes.back() == ' ')
      bytes.pop_back();
    rows.push_back(std::move(bytes));
  }
  return rows;
}

std::string HeadlessScreen::Render() const {
  const std::vector<std::string> rows = RenderRows();
  std::string out;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i > 0)
      out += '\n';
    out += rows[i];
  }
  return out;
}

} // namespace newbasic::screen
