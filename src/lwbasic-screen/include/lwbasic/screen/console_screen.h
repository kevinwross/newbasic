#pragma once

#include <atomic>

#include "lwbasic/screen/screen.h"

namespace lwbasic::screen {

// Win32 console backend (plan §1):
//  - output via WriteConsoleW (UTF-16; no SetConsoleOutputCP)
//  - full-screen mode via SetConsoleScreenBufferSize / SetConsoleWindowInfo
//    (buffer >= 80x25, clamped to the actual console size); the console
//    cursor is hidden while in program mode
//  - Ctrl+C maps to the GW-BASIC interrupt (break) rather than killing the
//    process, via SetConsoleCtrlHandler
//  - line input via ReadConsoleW; key polling via ReadConsoleInputW
//    (INKEY$ arrow-code mapping lands in M1)
class ConsoleScreen : public Screen {
public:
  ConsoleScreen() = default;
  ~ConsoleScreen() override;

  ConsoleScreen(const ConsoleScreen&) = delete;
  ConsoleScreen& operator=(const ConsoleScreen&) = delete;

  // Opens stdout; installs the Ctrl+C break handler when *break_flag is
  // given. Returns false when no console is available.
  bool Init(std::atomic<bool>* break_flag);

  void Present() override;
  void OnGridChanged() override;
  void EnterFullScreen() override;
  void ExitFullScreen() override;
  std::wstring ReadLine() override;
  bool TryGetKey(wchar_t* key) override;

  // Shows or hides the console caret: direct mode shows it (the user is
  // typing), a running program hides it.
  void ShowCaret(bool visible);

private:
  void* Handle() const { return out_; }
  void* out_ = nullptr; // HANDLE (kept opaque so <windows.h> stays in .cpp)
  bool full_screen_ = false;
  int saved_cols_ = 80;
  int saved_rows_ = 25;
  bool saved_cursor_visible_ = true;
};

} // namespace lwbasic::screen
