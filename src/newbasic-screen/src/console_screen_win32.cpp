#include "newbasic/screen/console_screen.h"

// Keep the Win32 min/max macros (and the legacy COORD macro) out of the way:
// they break std::min/std::max and COORD{...} aggregate init.
#define NOMINMAX
#include <windows.h>
#undef COORD

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <string>

namespace newbasic::screen {
namespace {

std::atomic<bool>* g_break_flag = nullptr;

// Ctrl+C -> GW-BASIC break semantics (set the flag, keep the process alive).
BOOL WINAPI CtrlHandler(DWORD) {
  if (g_break_flag != nullptr) {
    g_break_flag->store(true, std::memory_order_relaxed);
  }
  return TRUE;
}

// CGA text attribute -> Win32 console attribute flags.
WORD AttrToWin32(std::uint8_t attr) {
  const int fg = Screen::AttrFg(attr);
  const int bg = Screen::AttrBg(attr);
  WORD flags = 0;
  if (fg & 1)
    flags |= FOREGROUND_BLUE;
  if (fg & 2)
    flags |= FOREGROUND_GREEN;
  if (fg & 4)
    flags |= FOREGROUND_RED;
  if (fg & 8)
    flags |= FOREGROUND_INTENSITY;
  if (bg & 1)
    flags |= BACKGROUND_BLUE;
  if (bg & 2)
    flags |= BACKGROUND_GREEN;
  if (bg & 4)
    flags |= BACKGROUND_RED;
  if (bg & 8)
    flags |= BACKGROUND_INTENSITY;
  return flags;
}

} // namespace

struct ConsoleScreen::Impl {
  void* out = nullptr; // HANDLE (kept opaque so <windows.h> stays in .cpp)
  bool full_screen = false;
  int saved_cols = 80;
  int saved_rows = 25;
  bool saved_cursor_visible = true;
};

ConsoleScreen::ConsoleScreen() : impl_(std::make_unique<Impl>()) {}

ConsoleScreen::~ConsoleScreen() {
  if (impl_ == nullptr || impl_->out == nullptr)
    return;
  if (impl_->full_screen)
    ExitFullScreen();
  if (g_break_flag != nullptr) {
    SetConsoleCtrlHandler(CtrlHandler, FALSE);
    g_break_flag = nullptr;
  }
  CloseHandle(static_cast<HANDLE>(impl_->out));
  impl_->out = nullptr;
}

bool ConsoleScreen::Init(std::atomic<bool>* break_flag) {
  const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h == nullptr || h == INVALID_HANDLE_VALUE)
    return false;
  CONSOLE_SCREEN_BUFFER_INFO info;
  if (!GetConsoleScreenBufferInfo(h, &info))
    return false; // no console
  impl_->out = static_cast<void*>(h);
  if (break_flag != nullptr) {
    g_break_flag = break_flag;
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
  }
  return true;
}

void ConsoleScreen::OnGridChanged() {
  // A mode can ask for more cells than the console has: SCREEN 720 wants 160x45 and a
  // console is typically 80x25 or 120x30. Painting a wider grid would wrap and corrupt the
  // display, so the console clamps the grid to its own size. The inner SetGridSize call
  // re-enters here and finds nothing to do, so this terminates after one level.
  const int max_cols = std::max(1, impl_->saved_cols);
  const int max_rows = std::max(1, impl_->saved_rows);
  if (Width() > max_cols || Height() > max_rows) {
    SetGridSize(std::min(Width(), max_cols), std::min(Height(), max_rows));
  }
}

void ConsoleScreen::Present() {
  if (impl_->out == nullptr)
    return;
  // Full-grid repaint in attribute runs: the grid is UTF-16 so WriteConsoleW
  // needs no code-page conversion, and SetConsoleTextAttribute is applied
  // whenever the CGA attribute of the next cell changes (COLOR statement).
  SetConsoleCursorPosition(static_cast<HANDLE>(impl_->out), COORD{0, 0});
  std::wstring run;
  std::uint8_t current = 0xFF; // force an initial attribute set
  auto flush = [&]() {
    if (run.empty())
      return;
    DWORD written = 0;
    WriteConsoleW(static_cast<HANDLE>(impl_->out), run.data(), static_cast<DWORD>(run.size()),
                  &written, nullptr);
    run.clear();
  };
  for (int r = 0; r < Height(); ++r) {
    if (r > 0)
      run += L"\r\n";
    for (int c = 0; c < Width(); ++c) {
      const std::uint8_t attr = CellAttrAt(r, c);
      if (attr != current) {
        flush();
        SetConsoleTextAttribute(static_cast<HANDLE>(impl_->out), AttrToWin32(attr));
        current = attr;
      }
      run += CellAt(r, c);
    }
  }
  flush();
  // Restore the default attribute and leave the (hidden) cursor at the
  // active cell, so text printed after the program looks normal.
  SetConsoleTextAttribute(static_cast<HANDLE>(impl_->out), AttrToWin32(Screen::kDefaultAttr));
  SetConsoleCursorPosition(static_cast<HANDLE>(impl_->out),
                           COORD{static_cast<SHORT>(CursorCol()), static_cast<SHORT>(CursorRow())});
  // LOCATE r, c, 0 hides the caret; the console cursor is otherwise left where the grid
  // says it is.
  CONSOLE_CURSOR_INFO caret;
  if (GetConsoleCursorInfo(static_cast<HANDLE>(impl_->out), &caret)) {
    caret.bVisible = CursorVisible() ? TRUE : FALSE;
    SetConsoleCursorInfo(static_cast<HANDLE>(impl_->out), &caret);
  }
}

void ConsoleScreen::EnterFullScreen() {
  if (impl_->out == nullptr || impl_->full_screen)
    return;
  CONSOLE_SCREEN_BUFFER_INFO info;
  if (!GetConsoleScreenBufferInfo(static_cast<HANDLE>(impl_->out), &info))
    return;
  impl_->saved_cols = info.dwSize.X;
  impl_->saved_rows = info.dwSize.Y;
  CONSOLE_CURSOR_INFO cursor;
  if (GetConsoleCursorInfo(static_cast<HANDLE>(impl_->out), &cursor)) {
    impl_->saved_cursor_visible = cursor.bVisible != 0;
  }

  // Try the nominal 80x25 buffer; on failure (console too small) clamp to
  // the actual console size (plan section 1).
  const COORD buffer{80, 25};
  const SMALL_RECT window{0, 0, 79, 24};
  if (SetConsoleScreenBufferSize(static_cast<HANDLE>(impl_->out), buffer) &&
      SetConsoleWindowInfo(static_cast<HANDLE>(impl_->out), TRUE, &window)) {
    SetGridSize(80, 25);
  } else {
    SetGridSize(std::min(80, static_cast<int>(info.dwSize.X)),
                std::min(25, static_cast<int>(info.dwSize.Y)));
  }
  Clear();
  ShowCursor(FALSE);
  impl_->full_screen = true;
}

void ConsoleScreen::ExitFullScreen() {
  if (impl_->out == nullptr || !impl_->full_screen)
    return;
  ShowCursor(impl_->saved_cursor_visible ? TRUE : FALSE);
  const COORD buffer{static_cast<SHORT>(impl_->saved_cols), static_cast<SHORT>(impl_->saved_rows)};
  const SMALL_RECT window{0, 0, static_cast<SHORT>(impl_->saved_cols - 1),
                          static_cast<SHORT>(impl_->saved_rows - 1)};
  SetConsoleScreenBufferSize(static_cast<HANDLE>(impl_->out), buffer);
  SetConsoleWindowInfo(static_cast<HANDLE>(impl_->out), TRUE, &window);
  SetGridSize(80, 25);
  impl_->full_screen = false;
}

namespace {
// Raw key events, so the editor owns echoing and backspace. ReadConsoleW would let the
// console do the line editing and echo, which the grid never sees.
class ConsoleKeySource : public KeySource {
public:
  bool WaitKey(wchar_t* key) override {
    const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    for (;;) {
      INPUT_RECORD rec;
      DWORD n = 0;
      if (!ReadConsoleInputW(in, &rec, 1, &n))
        return false;
      if (n == 0 || rec.EventType != KEY_EVENT || rec.Event.KeyEvent.bKeyDown == 0)
        continue;
      const wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
      if (c == 0)
        continue; // arrows and function keys have no character in line input
      *key = c;
      return true;
    }
  }
};
} // namespace

std::wstring ConsoleScreen::ReadLine() {
  if (impl_->out == nullptr)
    return {};
  // Disable the console's own line editing and echo: the editor echoes into the grid, and
  // View/Present repaint from the grid.
  const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(in, &mode))
    SetConsoleMode(in, (mode | ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT) &
                           ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
  ConsoleKeySource keys;
  std::wstring line;
  if (!EditLine(keys, &line))
    return {};
  return line;
}

void ConsoleScreen::ShowCaret(bool visible) {
  ShowCursor(visible ? TRUE : FALSE);
}

bool ConsoleScreen::TryGetKey(wchar_t* key) {
  if (impl_->out == nullptr)
    return false;
  const HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  INPUT_RECORD rec;
  DWORD n = 0;
  // Drain the input queue; M1 adds arrow-key -> INKEY$ code mapping
  // (User's Guide Appendix H).
  while (ReadConsoleInputW(in, &rec, 1, &n) && n > 0) {
    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown != 0) {
      const wchar_t c = rec.Event.KeyEvent.uChar.UnicodeChar;
      if (c != 0 && c >= L' ') {
        *key = c;
        return true;
      }
    }
  }
  return false;
}

void ConsoleScreen::PumpMessages() {}

bool ConsoleScreen::Closed() const {
  // The Win32 backend does not currently detect a closed console; the windowless
  // DAP server constructs a ConsoleScreen without Init(), so "not initialised"
  // must not mean "closed".
  return false;
}

bool ConsoleScreen::InputExhausted() const {
  return false;
}

void ConsoleScreen::SuspendForChildProcess() {}

void ConsoleScreen::ResumeAfterChildProcess() {}

} // namespace newbasic::screen
