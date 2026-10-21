#pragma once

#include <string>
#include <vector>

#include "newbasic/screen/screen.h"

namespace newbasic::screen {

// Deterministic grid capture for tests (plan §1): no device I/O; input is
// scripted via ScriptKey/ScriptLine; Render() snapshots the grid.
class HeadlessScreen : public Screen {
public:
  // Device hooks are no-ops for capture.
  void Present() override {}
  void EnterFullScreen() override {}
  void ExitFullScreen() override {}

  std::wstring ReadLine() override;
  bool TryGetKey(wchar_t* key) override;
  // Scripted input is exhausted once the queue is empty; that is how the
  // REPL and INPUT tests terminate.
  bool InputExhausted() const override { return lines_.empty(); }

  void ScriptKey(wchar_t key) { keys_.push_back(key); }
  void ScriptLine(std::wstring line) { lines_.push_back(std::move(line)); }

  // Grid rows as CP437 bytes, trimmed of trailing spaces (trailing blanks
  // are not meaningful in the M0 golden comparisons).
  std::vector<std::string> RenderRows() const;
  // All rows joined with '\n'.
  std::string Render() const;

private:
  std::vector<wchar_t> keys_;
  std::vector<std::wstring> lines_;
};

} // namespace newbasic::screen
