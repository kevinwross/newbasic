#pragma once

#include <atomic>
#include <memory>

#include "newbasic/screen/screen.h"

namespace newbasic::screen {

// Interactive terminal backend.
//
// Windows keeps the original Win32 console implementation (WriteConsoleW,
// SetConsoleScreenBufferSize, ReadConsoleInputW and a Ctrl+C handler). POSIX
// uses termios plus ANSI escape sequences. Both keep the same public interface
// and the same grid/attribute semantics.
class ConsoleScreen : public Screen {
public:
  ConsoleScreen();
  ~ConsoleScreen() override;

  ConsoleScreen(const ConsoleScreen&) = delete;
  ConsoleScreen& operator=(const ConsoleScreen&) = delete;

  // Opens the terminal; installs the break handler when *break_flag is given.
  // Returns false when no terminal is available.
  bool Init(std::atomic<bool>* break_flag);

  void Present() override;
  void OnGridChanged() override;
  void EnterFullScreen() override;
  void ExitFullScreen() override;
  std::wstring ReadLine() override;
  bool TryGetKey(wchar_t* key) override;
  void PumpMessages() override;
  bool Closed() const override;
  bool InputExhausted() const override;

  // Shows or hides the terminal caret: direct mode shows it (the user is
  // typing), a running program hides it.
  void ShowCaret(bool visible);

  void SuspendForChildProcess() override;
  void ResumeAfterChildProcess() override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace newbasic::screen
