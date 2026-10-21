#include "newbasic/screen/console_screen.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace newbasic::screen {
namespace {

volatile sig_atomic_t g_sigint = 0;
volatile sig_atomic_t g_sigwinch = 0;
volatile sig_atomic_t g_sigterm = 0;

void SignalHandler(int signal) {
  if (signal == SIGINT)
    g_sigint = 1;
  else if (signal == SIGWINCH)
    g_sigwinch = 1;
  else if (signal == SIGTERM || signal == SIGHUP)
    g_sigterm = 1;
}

bool WriteAll(int fd, const char* data, std::size_t size) {
  while (size > 0) {
    const ssize_t written = ::write(fd, data, size);
    if (written < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    data += written;
    size -= static_cast<std::size_t>(written);
  }
  return true;
}

void WriteText(const std::string& text) {
  if (!text.empty())
    WriteAll(STDOUT_FILENO, text.data(), text.size());
}

// CGA IRGB colour index -> ANSI SGR foreground/background code.
int AnsiColour(int cga, bool background) {
  static const int kBase[8] = {30, 34, 32, 36, 31, 35, 33, 37};
  const int index = cga & 0x0F;
  if (index < 8)
    return kBase[index] + (background ? 10 : 0);
  return (kBase[index - 8] + 60) + (background ? 10 : 0);
}

std::string SgrFor(std::uint8_t attr) {
  return "\x1b[" + std::to_string(AnsiColour(Screen::AttrFg(attr), false)) + ";" +
         std::to_string(AnsiColour(Screen::AttrBg(attr), true)) + "m";
}

void AppendUtf8(std::string* out, wchar_t ch) {
  std::uint32_t code = static_cast<std::uint32_t>(ch);
  if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
    code = '?';
  if (code < 0x80) {
    out->push_back(static_cast<char>(code));
  } else if (code < 0x800) {
    out->push_back(static_cast<char>(0xC0 | (code >> 6)));
    out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
  } else if (code < 0x10000) {
    out->push_back(static_cast<char>(0xE0 | (code >> 12)));
    out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
  } else {
    out->push_back(static_cast<char>(0xF0 | (code >> 18)));
    out->push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
    out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
  }
}

// Reads one byte. Returns 1 on success, 0 when no byte is available in
// non-blocking mode, and -1 on EOF/error.
int ReadByte(bool blocking) {
  if (!blocking) {
    pollfd item{};
    item.fd = STDIN_FILENO;
    item.events = POLLIN;
    const int ready = ::poll(&item, 1, 0);
    if (ready == 0)
      return 0;
    if (ready < 0)
      return errno == EINTR ? 0 : -1;
  }
  unsigned char byte = 0;
  for (;;) {
    const ssize_t count = ::read(STDIN_FILENO, &byte, 1);
    if (count == 1)
      return byte;
    if (count == 0)
      return -1;
    if (errno == EINTR)
      continue;
    return -1;
  }
}

// Returns 1 with a decoded key, 0 for an ignored sequence or no input, and -1
// on EOF/error. Arrow/function-key CSI/SS3 sequences are consumed rather than
// aborting line editing.
int ReadKey(bool blocking, wchar_t* key) {
  const int first = ReadByte(blocking);
  if (first <= 0)
    return first;
  unsigned char byte = static_cast<unsigned char>(first);

  if (byte == 0x1B) {
    // A bare Escape is the editor's cancel key; a sequence starting with
    // Escape is a special key and is ignored. The short timeout distinguishes
    // the two without making Escape feel sluggish.
    pollfd item{};
    item.fd = STDIN_FILENO;
    item.events = POLLIN;
    if (::poll(&item, 1, 25) <= 0) {
      *key = 0x1B;
      return 1;
    }
    const int second = ReadByte(true);
    if (second <= 0)
      return second;
    if (second == '[' || second == 'O') {
      for (;;) {
        const int next = ReadByte(true);
        if (next <= 0)
          return next;
        if (next >= 0x40 && next <= 0x7E)
          break;
      }
    }
    return 0;
  }

  if (byte < 0x80) {
    // GW-BASIC's INKEY$ reports the Backspace key as CHR$(8). Most POSIX
    // terminals send DEL (0x7F), so normalise it here as well as accepting
    // it in the line editor.
    *key = byte == 0x7F ? static_cast<wchar_t>(0x08) : static_cast<wchar_t>(byte);
    return 1;
  }

  int continuation = 0;
  std::uint32_t code = 0;
  if ((byte & 0xE0) == 0xC0) {
    continuation = 1;
    code = byte & 0x1F;
  } else if ((byte & 0xF0) == 0xE0) {
    continuation = 2;
    code = byte & 0x0F;
  } else if ((byte & 0xF8) == 0xF0) {
    continuation = 3;
    code = byte & 0x07;
  } else {
    *key = L'?';
    return 1;
  }
  for (int i = 0; i < continuation; ++i) {
    const int next = ReadByte(true);
    if (next <= 0)
      return next;
    if ((next & 0xC0) != 0x80) {
      *key = L'?';
      return 1;
    }
    code = (code << 6) | static_cast<std::uint32_t>(next & 0x3F);
  }
  *key = static_cast<wchar_t>(code);
  return 1;
}

class PosixKeySource : public KeySource {
public:
  bool WaitKey(wchar_t* key) override {
    for (;;) {
      const int result = ReadKey(true, key);
      if (result < 0)
        return false;
      if (result == 1)
        return true;
    }
  }
};

} // namespace

struct ConsoleScreen::Impl {
  termios saved{};
  bool have_saved = false;
  bool raw = false;
  bool full_screen = false;
  bool suspended = false;
  bool closed = false;
  int cols = 80;
  int rows = 25;
  bool saved_cursor_visible = true;
  bool caret_visible = true;
  std::atomic<bool>* break_flag = nullptr;
  struct sigaction old_int{};
  struct sigaction old_winch{};
  struct sigaction old_term{};
  struct sigaction old_hup{};
};

ConsoleScreen::ConsoleScreen() : impl_(std::make_unique<Impl>()) {}

ConsoleScreen::~ConsoleScreen() {
  if (impl_ == nullptr)
    return;
  if (impl_->full_screen)
    ExitFullScreen();
  if (impl_->raw && impl_->have_saved)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->saved);
  if (impl_->break_flag != nullptr) {
    sigaction(SIGINT, &impl_->old_int, nullptr);
    sigaction(SIGWINCH, &impl_->old_winch, nullptr);
    sigaction(SIGTERM, &impl_->old_term, nullptr);
    sigaction(SIGHUP, &impl_->old_hup, nullptr);
  }
  if (impl_->closed)
    WriteText("\x1b[0m\x1b[?25h\x1b[?1049l");
}

bool ConsoleScreen::Init(std::atomic<bool>* break_flag) {
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    return false;
  if (tcgetattr(STDIN_FILENO, &impl_->saved) != 0)
    return false;
  impl_->have_saved = true;
  winsize size{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
    impl_->cols = std::max(1, static_cast<int>(size.ws_col));
    impl_->rows = std::max(1, static_cast<int>(size.ws_row));
  }
  impl_->break_flag = break_flag;

  struct sigaction action{};
  action.sa_handler = SignalHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT, &action, &impl_->old_int);
  sigaction(SIGWINCH, &action, &impl_->old_winch);
  sigaction(SIGTERM, &action, &impl_->old_term);
  sigaction(SIGHUP, &action, &impl_->old_hup);
  return true;
}

void ConsoleScreen::EnterFullScreen() {
  if (impl_ == nullptr || !impl_->have_saved || impl_->full_screen)
    return;
  termios raw = impl_->saved;
  cfmakeraw(&raw);
  // Keep ISIG so Ctrl+C becomes SIGINT (the GW-BASIC break) rather than a
  // literal byte the editor would insert.
  raw.c_lflag |= ISIG;
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
    return;
  impl_->raw = true;
  impl_->full_screen = true;
  impl_->saved_cursor_visible = true;
  SetGridSize(std::min(80, impl_->cols), std::min(25, impl_->rows));
  WriteText("\x1b[?1049h\x1b[?25l\x1b[H\x1b[2J");
  Clear();
}

void ConsoleScreen::ExitFullScreen() {
  if (impl_ == nullptr || !impl_->full_screen)
    return;

  // Leaving the alternate screen discards it. Dump the final grid to the
  // normal screen first, so `newbasic prog.bas` leaves the program's output
  // visible instead of returning to an empty terminal. This mirrors what the
  // Win32 backend's restored console buffer shows.
  std::vector<std::string> lines;
  lines.reserve(static_cast<std::size_t>(Height()));
  for (int row = 0; row < Height(); ++row) {
    std::string line;
    for (int col = 0; col < Width(); ++col)
      AppendUtf8(&line, CellAt(row, col));
    while (!line.empty() && line.back() == ' ')
      line.pop_back();
    lines.push_back(std::move(line));
  }
  while (!lines.empty() && lines.back().empty())
    lines.pop_back();
  std::string final_screen = "\x1b[0m";
  for (const std::string& line : lines) {
    final_screen += line;
    final_screen += "\r\n";
  }

  WriteText("\x1b[0m\x1b[?25h\x1b[?1049l");
  WriteText(final_screen);
  if (impl_->raw && impl_->have_saved)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->saved);
  impl_->raw = false;
  impl_->full_screen = false;
  SetGridSize(80, 25);
}

void ConsoleScreen::OnGridChanged() {
  const int max_cols = std::max(1, impl_->cols);
  const int max_rows = std::max(1, impl_->rows);
  if (Width() > max_cols || Height() > max_rows) {
    SetGridSize(std::min(Width(), max_cols), std::min(Height(), max_rows));
  }
}

void ConsoleScreen::Present() {
  if (impl_ == nullptr || !impl_->full_screen || impl_->suspended)
    return;
  std::string out;
  out.reserve(static_cast<std::size_t>(Width()) * static_cast<std::size_t>(Height()) * 2 + 256);
  out += "\x1b[H";
  std::uint8_t current = 0xFF;
  for (int row = 0; row < Height(); ++row) {
    for (int col = 0; col < Width(); ++col) {
      const std::uint8_t attr = CellAttrAt(row, col);
      if (attr != current) {
        out += SgrFor(attr);
        current = attr;
      }
      AppendUtf8(&out, CellAt(row, col));
    }
    out += "\x1b[K";
    if (row + 1 < Height())
      out += "\r\n";
  }
  out += "\x1b[0m";
  out += "\x1b[" + std::to_string(CursorRow() + 1) + ";" + std::to_string(CursorCol() + 1) + "H";
  out += CursorVisible() ? "\x1b[?25h" : "\x1b[?25l";
  WriteText(out);
}

std::wstring ConsoleScreen::ReadLine() {
  if (impl_ == nullptr || !impl_->full_screen)
    return {};
  PosixKeySource keys;
  std::wstring line;
  if (!EditLine(keys, &line))
    return {};
  return line;
}

void ConsoleScreen::ShowCaret(bool visible) {
  if (impl_ == nullptr)
    return;
  impl_->caret_visible = visible;
  WriteText(visible ? "\x1b[?25h" : "\x1b[?25l");
}

bool ConsoleScreen::TryGetKey(wchar_t* key) {
  if (impl_ == nullptr || !impl_->full_screen)
    return false;
  wchar_t value = 0;
  const int result = ReadKey(false, &value);
  if (result < 0) {
    impl_->closed = true;
    return false;
  }
  if (result == 0)
    return false;
  *key = value;
  return true;
}

void ConsoleScreen::PumpMessages() {
  if (impl_ == nullptr)
    return;
  if (g_sigint != 0) {
    g_sigint = 0;
    if (impl_->break_flag != nullptr)
      impl_->break_flag->store(true, std::memory_order_relaxed);
  }
  if (g_sigwinch != 0) {
    g_sigwinch = 0;
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
      impl_->cols = std::max(1, static_cast<int>(size.ws_col));
      impl_->rows = std::max(1, static_cast<int>(size.ws_row));
      OnGridChanged();
    }
  }
  if (g_sigterm != 0) {
    g_sigterm = 0;
    impl_->closed = true;
    if (impl_->break_flag != nullptr)
      impl_->break_flag->store(true, std::memory_order_relaxed);
  }
}

bool ConsoleScreen::Closed() const {
  return impl_ != nullptr && impl_->closed;
}

bool ConsoleScreen::InputExhausted() const {
  return Closed();
}

void ConsoleScreen::SuspendForChildProcess() {
  if (impl_ == nullptr || !impl_->full_screen || impl_->suspended)
    return;
  WriteText("\x1b[0m\x1b[?25h\x1b[?1049l");
  if (impl_->raw && impl_->have_saved)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->saved);
  impl_->raw = false;
  impl_->suspended = true;
}

void ConsoleScreen::ResumeAfterChildProcess() {
  if (impl_ == nullptr || !impl_->suspended || !impl_->have_saved)
    return;
  termios raw = impl_->saved;
  cfmakeraw(&raw);
  raw.c_lflag |= ISIG;
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0)
    impl_->raw = true;
  WriteText("\x1b[?1049h\x1b[?25l\x1b[2J");
  impl_->suspended = false;
  Clear();
}

} // namespace newbasic::screen
