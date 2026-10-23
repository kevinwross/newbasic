#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "newbasic/screen/canvas.h"
#include "newbasic/screen/palette.h"

namespace newbasic::screen {

// CP437 byte value (0x80-0xFF) -> Unicode code unit. 0-0x7F map to
// themselves. Extended block per the CP437 specification; see the table in
// screen.cpp.
wchar_t Cp437ToUnicode(unsigned char b);
// Unicode code unit -> CP437 byte (first match; '?' if unmapped).
unsigned char UnicodeToCp437(wchar_t c);
// Decodes a CP437/ASCII byte string to Unicode.
std::wstring DecodeCp437(const std::string& bytes);
// Encodes Unicode to CP437/ASCII bytes (unmapped -> '?').
std::string EncodeCp437(const std::wstring& text);

// Screen: the 80x25 text grid + cursor (plan section 1). The base class owns
// A source of key events for line editing. Windowed and console backends block until a key
// arrives; tests supply a scripted stream. Declared before Screen because EditLine takes one.
class KeySource {
public:
  virtual ~KeySource() = default;
  // Blocks until a key is available. Returns false when the input has ended.
  virtual bool WaitKey(wchar_t* key) = 0;
};

// the grid and all cursor/wrap/scroll/zone logic; backends implement the
// device hooks (Present, full-screen transitions, input). This is the single
// point where text, graphics (M7) and input attach.
class Screen {
public:
  static constexpr int kCols = 80;
  static constexpr int kRows = 25;
  // The grid is dynamic so a mode can choose its own shape: the compatible screens keep
  // 80x25 and SCREEN 720 uses 160x45. These are the ceilings.
  static constexpr int kMaxCols = 200;
  static constexpr int kMaxRows = 60;
  static constexpr int kZoneWidth = 14; // print zones for comma separation

  // CGA-style text attribute: low nibble = foreground, high nibble =
  // background (GW-BASIC COLOR 0-15 / 0-7).
  static constexpr std::uint8_t MakeAttr(int fg, int bg) {
    return static_cast<std::uint8_t>(((bg & 0x0F) << 4) | (fg & 0x0F));
  }
  static constexpr int AttrFg(std::uint8_t attr) { return attr & 0x0F; }
  static constexpr int AttrBg(std::uint8_t attr) { return (attr >> 4) & 0x0F; }
  // White on black (attribute 0x07).
  static constexpr std::uint8_t kDefaultAttr = 0x07;

  virtual ~Screen() = default;

  // --- grid state ---
  int Width() const { return width_; }
  int Height() const { return height_; }

  // VIEW PRINT: the text window (0-based, inclusive). Printing wraps and scrolls
  // inside it, and CLS 2 clears only it. The default is the whole grid; GW-BASIC
  // reserves row 25 for the function-key bar, which newbasic does not display
  // unless KEY ON is used, so the default differs deliberately.
  void SetTextWindow(int top, int bottom);
  int TextWindowTop() const { return window_top_; }
  int TextWindowBottom() const { return window_bottom_; }

  void Clear();           // CLS: fill with spaces, cursor to 0,0
  void ClearTextWindow(); // CLS 2: clear only the text window

  // --- graphics (M12) ---
  // Mode 0 is text only. 1 = 320x200 4-colour, 2 = 640x200 2-colour, 9 = 640x350
  // 16-colour. In text mode the canvas exists but is not displayed.
  static constexpr int kTextMode = 0;
  int GraphicsMode() const { return graphics_mode_; }
  void SetGraphicsMode(int mode);
  Canvas& Graphics() { return canvas_; }
  const Canvas& Graphics() const { return canvas_; }
  // Plotting through the screen keeps the dirty flag consistent, so the backends
  // present a changed canvas.
  void Plot(int x, int y, int colour) {
    canvas_.SetPixel(x, y, static_cast<std::uint8_t>(colour));
    MarkDirty();
  }
  void DrawGraphicsLine(int x1, int y1, int x2, int y2, int colour) {
    canvas_.DrawLine(x1, y1, x2, y2, static_cast<std::uint8_t>(colour));
    MarkDirty();
  }
  void ClearGraphics(std::uint8_t colour = 0) {
    canvas_.Clear(colour);
    MarkDirty();
  }
  // CLS while a graphics VIEW is active: clear the viewport interior, using the
  // viewport fill colour when VIEW supplied one and the current background
  // attribute (0) otherwise. Pixels outside the viewport are untouched.
  void ClearGraphicsViewport() {
    if (!viewport_active_) {
      ClearGraphics(0);
      return;
    }
    const std::uint8_t colour =
        static_cast<std::uint8_t>(view_fill_ >= 0 ? view_fill_ : 0);
    canvas_.FillBox(view_x_, view_y_, view_x_ + view_width_ - 1,
                    view_y_ + view_height_ - 1, colour);
    MarkDirty();
  }
  // PALETTE (M12 phase 3): maps a colour attribute to a CGA colour index. The
  // reference allows 0..63, whose upper blocks are intensity variants; the palette
  // here is the 16 colours, so an entry stores colour % 16 (documented divergence).
  // In the 256-colour mode the second argument is an 18-bit RGB value instead:
  // red + 256*green + 65536*blue, six bits per channel, as GW-BASIC's VGA palettes
  // work.
  void SetPaletteEntry(int attribute, int colour) {
    if (attribute < 0)
      return;
    if (IsTrueColourMode()) {
      if (attribute > 255)
        return;
      const int red = colour & 0x3F;
      const int green = (colour >> 8) & 0x3F;
      const int blue = (colour >> 16) & 0x3F;
      std::array<std::uint8_t, 3>& entry = rgb_palette_[static_cast<std::size_t>(attribute)];
      entry[0] = static_cast<std::uint8_t>(red * 255 / 63);
      entry[1] = static_cast<std::uint8_t>(green * 255 / 63);
      entry[2] = static_cast<std::uint8_t>(blue * 255 / 63);
      MarkDirty();
      return;
    }
    if (attribute > 15)
      return;
    palette_[static_cast<std::size_t>(attribute)] =
        static_cast<std::uint8_t>((colour < 0 ? 0 : colour) % 16);
    RefreshCgaRgb();
    MarkDirty();
  }
  void ResetPalette() {
    for (std::size_t i = 0; i < palette_.size(); ++i)
      palette_[i] = static_cast<std::uint8_t>(i);
    RefreshCgaRgb();
    MarkDirty();
  }
  // The display reads rgb_palette_, so every change to the CGA mapping has to refresh it.
  // Missing this made PALETTE have no visible effect in a graphics mode.
  void RefreshCgaRgb() {
    for (std::size_t i = 0; i < palette_.size(); ++i) {
      const Rgb rgb = CgaRgb(palette_[i]);
      rgb_palette_[i] = {rgb.r, rgb.g, rgb.b};
    }
  }
  // COLOR ,n in a graphics mode: an even n selects the first colour set (1 green,
  // 2 red, 3 brown) and an odd n the second (1 cyan, 2 magenta, 3 white), per the
  // reference's COLOR table.
  void SetGraphicsColourSet(int which) {
    const bool second = (which % 2) != 0;
    palette_[1] = static_cast<std::uint8_t>(second ? 3 : 2);
    palette_[2] = static_cast<std::uint8_t>(second ? 5 : 4);
    palette_[3] = static_cast<std::uint8_t>(second ? 15 : 6);
    RefreshCgaRgb();
    MarkDirty();
  }
  // COLOR b in a graphics mode selects the background, which is the palette entry
  // for colour 0.
  void SetGraphicsBackground(int colour) {
    palette_[0] = static_cast<std::uint8_t>((colour < 0 ? 0 : colour) % 16);
    MarkDirty();
  }
  const std::array<std::uint8_t, 16>& Palette() const { return palette_; }

  // The 256-colour mode (SCREEN 720, an extension) stores real RGB per attribute
  // rather than a CGA index, so the conversion to what the display shows reads this.
  static constexpr int kWideMode = 720;
  bool IsTrueColourMode() const { return graphics_mode_ == kWideMode || graphics_mode_ == 13; }
  const std::array<std::array<std::uint8_t, 3>, 256>& PaletteRgb() const { return rgb_palette_; }
  // Largest colour number the current mode accepts.
  int MaxColour() const { return IsTrueColourMode() ? 255 : 15; }
  // Display shape: 4:3 for the CGA-compatible modes, 16:9 for the wide mode, whose
  // 1280x720 canvas has square pixels.
  double DisplayAspect() const { return graphics_mode_ == kWideMode ? (16.0 / 9.0) : (4.0 / 3.0); }
  // VIEW (M12 phase 3): drawing is clipped to the viewport and coordinates are
  // relative to its top-left corner. A fill of -1 leaves the area unchanged.
  void SetGraphicsViewport(int x1, int y1, int x2, int y2, int fill, int border) {
    const int left = std::max(0, std::min(x1, x2));
    const int right = std::min(canvas_.Width() - 1, std::max(x1, x2));
    const int top = std::max(0, std::min(y1, y2));
    const int bottom = std::min(canvas_.Height() - 1, std::max(y1, y2));
    viewport_active_ = true;
    view_x_ = left;
    view_y_ = top;
    view_width_ = right - left + 1;
    view_height_ = bottom - top + 1;
    view_fill_ = fill;
    canvas_.SetClip(left, top, right, bottom);
    if (fill >= 0) {
      canvas_.FillBox(left, top, right, bottom, static_cast<std::uint8_t>(fill));
    }
    if (border >= 0) {
      canvas_.DrawBox(left, top, right, bottom, static_cast<std::uint8_t>(border));
    }
    MarkDirty();
  }
  void ResetGraphicsViewport() {
    viewport_active_ = false;
    view_x_ = 0;
    view_y_ = 0;
    view_width_ = canvas_.Width();
    view_height_ = canvas_.Height();
    view_fill_ = -1;
    canvas_.ClearClip();
    MarkDirty();
  }
  int ViewX() const { return view_x_; }
  int ViewY() const { return view_y_; }
  int ViewWidth() const { return view_width_; }
  int ViewHeight() const { return view_height_; }
  bool HasViewport() const { return viewport_active_; }

  // PCOPY (M12): copies one graphics page to another. Page 0 is the displayed canvas;
  // pages 1-7 are snapshots, because the backend has no display-page flipping. Copying
  // page 0 snapshots the canvas, and copying *to* page 0 restores a snapshot.
  static constexpr int kGraphicsPages = 8;
  bool CopyGraphicsPage(int source, int destination) {
    if (source < 0 || destination < 0 || source >= kGraphicsPages ||
        destination >= kGraphicsPages) {
      return false;
    }
    if (source == destination)
      return true; // nothing to do
    const std::size_t size =
        static_cast<std::size_t>(canvas_.Width()) * static_cast<std::size_t>(canvas_.Height());
    if (source == 0) {
      // Snapshot the canvas into the destination page.
      graphics_pages_[static_cast<std::size_t>(destination)] = canvas_.Pixels();
    } else if (destination == 0) {
      const std::vector<std::uint8_t>& page = graphics_pages_[static_cast<std::size_t>(source)];
      if (page.size() != size)
        return false; // never filled, or a different mode
      for (int y = 0; y < canvas_.Height(); ++y) {
        for (int x = 0; x < canvas_.Width(); ++x) {
          canvas_.SetPixel(x, y, page[static_cast<std::size_t>(y) * canvas_.Width() + x]);
        }
      }
    } else {
      const std::vector<std::uint8_t>& page = graphics_pages_[static_cast<std::size_t>(source)];
      if (page.size() != size)
        return false;
      graphics_pages_[static_cast<std::size_t>(destination)] = page;
    }
    MarkDirty();
    return true;
  }
  void DrawGraphicsStyledLine(int x1, int y1, int x2, int y2, int colour, int style) {
    canvas_.DrawLineStyled(x1, y1, x2, y2, static_cast<std::uint8_t>(colour), style);
    MarkDirty();
  }
  void DrawGraphicsBox(int x1, int y1, int x2, int y2, int colour) {
    canvas_.DrawBox(x1, y1, x2, y2, static_cast<std::uint8_t>(colour));
    MarkDirty();
  }
  void FillGraphicsBox(int x1, int y1, int x2, int y2, int colour) {
    canvas_.FillBox(x1, y1, x2, y2, static_cast<std::uint8_t>(colour));
    MarkDirty();
  }
  void DrawGraphicsEllipse(int cx, int cy, int rx, int ry, double start, double end, int colour) {
    canvas_.DrawEllipse(cx, cy, rx, ry, start, end, static_cast<std::uint8_t>(colour));
    MarkDirty();
  }
  void FillGraphics(int x, int y, int fill, int border) {
    canvas_.FloodFill(x, y, static_cast<std::uint8_t>(fill), border);
    MarkDirty();
  }
  // For code that draws straight into the canvas (the DRAW interpreter).
  void InvalidateGraphics() { MarkDirty(); }
  // LOCATE r, c, 0 hides the cursor and ,1 shows it. Renderers draw the cursor cell in
  // inverted video only while this is true.
  void SetCursorVisible(bool visible) {
    cursor_visible_ = visible;
    MarkDirty();
  }
  bool CursorVisible() const { return cursor_visible_; }

  void SetCursor(int row, int col); // clamped to the grid
  int CursorRow() const { return cursor_row_; }
  int CursorCol() const { return cursor_col_; }

  void PutCharAt(int row, int col, wchar_t ch);
  wchar_t CellAt(int row, int col) const;
  const std::vector<std::vector<wchar_t>>& Grid() const { return grid_; }

  // --- colors (COLOR statement) ---
  void SetColor(int fg, int bg) {
    attr_ = MakeAttr(fg < 0 ? 7 : fg, bg < 0 ? 0 : bg);
    MarkDirty();
  }
  int ColorFg() const { return AttrFg(attr_); }
  int ColorBg() const { return AttrBg(attr_); }
  std::uint8_t CurrentAttr() const { return attr_; }
  std::uint8_t CellAttrAt(int row, int col) const {
    if (row < 0 || row >= height_ || col < 0 || col >= width_)
      return attr_;
    return attr_grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
  }
  const std::vector<std::vector<std::uint8_t>>& AttrGrid() const { return attr_grid_; }

  // Writes text at the cursor, advancing it; wraps at the right edge and
  // scrolls one line when the cursor passes the bottom row.
  void WriteAtCursor(const std::wstring& text);
  void NewLine();
  // Records a line the user typed into the grid and advances past it. A backend that
  // repaints the grid would otherwise erase the typed line, because the console echoes
  // characters the grid never saw.
  void EchoInputLine(const std::wstring& text) {
    if (!text.empty())
      WriteAtCursor(text);
    NewLine();
  }
  // Backspace during line editing: clear the character before the cursor and step back.
  // At the start of a line there is nothing to pull back into, so it does nothing.
  void Backspace() {
    if (cursor_col_ <= 0)
      return;
    cursor_col_ -= 1;
    PutCharAt(cursor_row_, cursor_col_, L' ');
    MarkDirty();
  }
  // Reads one line with echo, editing as it goes: printable keys are written into the grid,
  // backspace removes them, Enter or Return finishes, Escape abandons what was typed.
  // Returns false when the source ends (a closed window, or a scripted stream that ran out),
  // so a caller can tell "empty line" from "no more input".
  bool EditLine(KeySource& keys, std::wstring* out) {
    // A prompt needs a visible caret: a program may have hidden it with LOCATE ,,0 while
    // drawing, and waiting for input with no cursor leaves the user with nowhere to look.
    // Turn it back on and show the frame before blocking.
    SetCursorVisible(true);
    PresentIfDirty();
    std::wstring line;
    for (;;) {
      wchar_t key = 0;
      if (!keys.WaitKey(&key))
        return false;
      if (key == L'\r' || key == L'\n')
        break;
      // Some terminals send DEL (0x7F) for the Backspace key and others send
      // BS (0x08). Both must edit the line; the POSIX backend additionally
      // normalises 0x7F to 0x08 for INKEY$ compatibility.
      if (key == 0x08 || key == 0x7F) {
        if (!line.empty()) {
          line.pop_back();
          Backspace();
          PresentIfDirty();
        }
        continue;
      }
      if (key == 0x1B) { // Escape: drop the line
        line.clear();
        break;
      }
      if (key >= 0x20 && key != 0x7F) {
        line.push_back(key);
        WriteAtCursor(std::wstring(1, key));
      }
      // Repaint as the user types: the throttled present is enough at human key rates and
      // keeps the window responsive without pacing the program.
      PresentIfDirty();
    }
    // The typed text is already in the grid, so this only moves past it.
    NewLine();
    if (out != nullptr)
      *out = line;
    return true;
  }
  // Advances the cursor to the start of the next print zone (comma
  // separator semantics).
  void AdvanceToNextZone();

  // --- device hooks (implemented by backends) ---
  // True once when the user asked to interrupt the running program, and the request is
  // consumed so the next run is not broken immediately. The console backend raises its break
  // flag from a control handler; the windowed one looks for Ctrl-C in its key queue.
  // A click on the drawing, in canvas pixels. Button 1 is the left button, 2 the right.
  struct MouseClick {
    int x = 0;
    int y = 0;
    int button = 0;
  };

  // True once per pending click, consuming it. Backends without a pointing device return
  // false, so a program that polls simply never sees a click.
  virtual bool TakeMouseClick(MouseClick*) { return false; }

  virtual bool TakeBreak() { return false; }

  // Cell metrics of the device, so a dump or a test can match what the backend draws.
  virtual int GlyphWidth() const { return 8; }
  virtual int GlyphHeight() const { return 16; }
  virtual void Present() = 0;
  virtual void EnterFullScreen() = 0;
  virtual void ExitFullScreen() = 0;

  // A child process (SHELL) needs the terminal in its normal cooked mode and
  // without the alternate screen. Backends that do not own the terminal leave
  // these as no-ops; the POSIX console restores and then re-enters raw mode.
  virtual void SuspendForChildProcess() {}
  virtual void ResumeAfterChildProcess() {}
  // True once the backend can no longer display anything, for example because the user
  // closed the window. The VM ends the run instead of looping forever into a destroyed
  // window.
  virtual bool Closed() const { return false; }
  // True when this backend can hand the real terminal to an interactive SHELL.
  // A window has no terminal to attach to, so it must reject the no-argument
  // form rather than freeze while a shell waits for invisible input.
  virtual bool CanRunInteractiveShell() const { return true; }
  // Cooked line input (GW-BASIC INPUT/LINE INPUT).
  virtual std::wstring ReadLine() = 0;
  // Non-blocking key poll (INKEY$). Returns true and fills *key if a key is
  // available.
  virtual bool TryGetKey(wchar_t* key) = 0;
  // True when a scripted input source has no lines left (headless tests);
  // interactive backends always report false.
  virtual bool InputExhausted() const { return false; }

  // Windowed backends dispatch their messages here. It must happen whether or not a
  // frame is due, because that is the only way a window close is noticed while the
  // program is busy: with the pump inside Present(), a throttled or idle program never
  // dispatched anything, so closing the window did nothing.
  virtual void PumpMessages() {}

  // Called when the grid changes shape, so a windowed backend can size its window to the
  // new mode: SCREEN 720 wants 1280x720 rather than the 80x25 default.
  virtual void OnGridChanged() {}

  // Pushes the grid to the device only when it changed; the VM calls this
  // after each instruction. Drawing is throttled to about thirty frames a second:
  // presenting after every instruction made a plotting program spend all its time in the
  // backend (10000 PSETs took 93 seconds before this). Each present costs around ten
  // milliseconds on the software rasteriser, so a 32 ms interval keeps that below a
  // third of the run time instead of half.
  static constexpr int kPresentIntervalMs = 32;
  // Messages are pumped at most this often. Pumping is needed so a close is noticed while
  // a program runs, but PeekMessage on every instruction cost more than the instructions:
  // the Mandelbrot spent 44 of its 53 seconds with drawing removed, and 11 million
  // PumpMessages calls were the difference.
  static constexpr int kPumpIntervalMs = 16;
  void PresentIfDirty() {
    const auto now = std::chrono::steady_clock::now();
    if (last_pump_ == std::chrono::steady_clock::time_point{} ||
        now - last_pump_ >= std::chrono::milliseconds(kPumpIntervalMs)) {
      last_pump_ = now;
      PumpMessages();
    }
    if (!dirty_)
      return;
    if (last_present_ != std::chrono::steady_clock::time_point{} &&
        now - last_present_ < std::chrono::milliseconds(kPresentIntervalMs)) {
      return; // a frame is due shortly
    }
    last_present_ = now;
    dirty_ = false;
    Present();
  }

  // Presents regardless of the throttle or the dirty flag, for the final frame.
  void PresentNow() {
    last_present_ = std::chrono::steady_clock::now();
    dirty_ = false;
    Present();
  }

  // Backends shrink the grid (width_/height_) when the device is smaller
  // than the nominal 80x25 (plan: "clamped to the actual console size").
  void SetGridSize(int width, int height);

protected:
  Screen() {
    SetGridSize(kCols, kRows);
    // Text mode never calls SetGraphicsMode, so nothing else would fill the RGB palette and
    // every attribute would resolve to black. The window's text renderer reads the CGA
    // table directly, which is why only colour consumers such as the view dump saw this.
    RefreshCgaRgb();
  }

private:
  void ScrollUp();
  void ResetPaletteForMode(int mode);
  void MarkDirty() { dirty_ = true; }

  int width_ = kCols;
  int height_ = kRows;
  int window_top_ = 0;
  int window_bottom_ = kRows - 1;
  int cursor_row_ = 0;
  int cursor_col_ = 0;
  bool dirty_ = true;
  bool cursor_visible_ = true;
  // When the last frame was presented, for the throttle in PresentIfDirty.
  std::chrono::steady_clock::time_point last_pump_{};
  std::chrono::steady_clock::time_point last_present_{};
  std::uint8_t attr_ = kDefaultAttr;
  std::vector<std::vector<wchar_t>> grid_;
  std::vector<std::vector<std::uint8_t>> attr_grid_;
  Canvas canvas_;
  int graphics_mode_ = kTextMode;
  // VIEW state: the viewport origin and size, in canvas pixels.
  bool viewport_active_ = false;
  int view_x_ = 0;
  int view_y_ = 0;
  int view_width_ = kCols;
  int view_height_ = kRows;
  int view_fill_ = -1;
  std::array<std::uint8_t, 16> palette_{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  // What each attribute actually displays as: the CGA colour for the compatible modes,
  // a real RGB ramp for SCREEN 720.
  std::array<std::array<std::uint8_t, 3>, 256> rgb_palette_{};
  // PCOPY snapshots: page 0 is the canvas itself, 1-7 are stored copies.
  std::array<std::vector<std::uint8_t>, kGraphicsPages> graphics_pages_;
};

} // namespace newbasic::screen
