#include "newbasic/screen/screen.h"

#include <algorithm>
#include <cstddef>

#include "newbasic/screen/palette.h"

namespace newbasic::screen {
namespace {

// CP437 high half (0x80-0xFF) -> Unicode code unit. Per the CP437
// specification; ASCII (0x00-0x7F) maps to itself.
const wchar_t kCp437Hi[128] = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, // 80
    0x00EA, 0x00EB, 0x00EF, 0x00EE, 0x00EC, 0x00C5, 0x00CE, 0x00CF, // 88
    0x00C9, 0x00D1, 0x00D3, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, // 90
    0x00B0, 0x00A3, 0x00A5, 0x00E8, 0x00EA, 0x00EB, 0x00EC, 0x00ED, // 98
    0x00FF, 0x00D6, 0x00D1, 0x00D2, 0x00CB, 0x00CC, 0x00CD, 0x00A2, // A0
    0x00DC, 0x00C9, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00CA, // A8
    0x00CB, 0x00E8, 0x00EA, 0x00EB, 0x00EF, 0x00EE, 0x00EC, 0x00CF, // B0
    0x00D0, 0x00F0, 0x00CA, 0x00FF, 0x00D3, 0x00DF, 0x00F6, 0x00F2, // B8
    0x00D3, 0x00F9, 0x00D9, 0x00DB, 0x00FB, 0x00F9, 0x02DC, 0x0160, // C0
    0x0161, 0x02DD, 0x0152, 0x0153, 0x017D, 0x017E, 0x0178, 0x00B0, // C8
    0x2514, 0x2534, 0x251C, 0x252C, 0x2500, 0x253C, 0x2524, 0x252C, // D0
    0x2502, 0x2534, 0x252C, 0x2524, 0x251C, 0x2500, 0x253C, 0x2580, // D8
    0x2660, 0x265E, 0x2666, 0x2663, 0x2663, 0x2660, 0x265E, 0x2666, // E0
    0x25A0, 0x25A1, 0x258C, 0x2590, 0x2591, 0x2592, 0x2593, 0x2593, // E8
    0x2320, 0x2321, 0x00AC, 0x00BE, 0x215F, 0x215B, 0x215C, 0x215D, // F0
    0x215E, 0x2264, 0x2265, 0x03C0, 0x00B2, 0x00B9, 0x00B3, 0x00B0, // F8
};

// The text grid is a vector of rows so a mode can choose its own shape: 80x25 for the
// GW-BASIC-compatible screens, 160x45 for SCREEN 720.
void FillRow(std::vector<wchar_t>& row) {
  for (std::size_t c = 0; c < row.size(); ++c)
    row[c] = L' ';
}

} // namespace

wchar_t Cp437ToUnicode(unsigned char b) {
  return b < 0x80 ? static_cast<wchar_t>(b) : kCp437Hi[b - 0x80];
}

unsigned char UnicodeToCp437(wchar_t c) {
  if (c < 0x80)
    return static_cast<unsigned char>(c);
  for (std::size_t i = 0; i < sizeof(kCp437Hi) / sizeof(kCp437Hi[0]); ++i) {
    if (kCp437Hi[i] == c)
      return static_cast<unsigned char>(i + 0x80);
  }
  return '?';
}

std::wstring DecodeCp437(const std::string& bytes) {
  std::wstring out;
  out.reserve(bytes.size());
  for (unsigned char b : bytes)
    out.push_back(Cp437ToUnicode(b));
  return out;
}

std::string EncodeCp437(const std::wstring& text) {
  std::string out;
  out.reserve(text.size());
  for (wchar_t c : text)
    out.push_back(static_cast<char>(UnicodeToCp437(c)));
  return out;
}

void Screen::SetGridSize(int width, int height) {
  width_ = std::max(1, std::min(width, kMaxCols));
  height_ = std::max(1, std::min(height, kMaxRows));
  grid_.assign(static_cast<std::size_t>(height_), std::vector<wchar_t>());
  for (auto& row : grid_) {
    row.assign(static_cast<std::size_t>(width_), L' ');
  }
  attr_grid_.assign(static_cast<std::size_t>(height_), std::vector<std::uint8_t>());
  for (auto& row : attr_grid_)
    row.assign(static_cast<std::size_t>(width_), attr_);
  // The text window always starts as the whole grid, in both directions.
  window_top_ = 0;
  window_bottom_ = height_ - 1;
  if (cursor_row_ >= height_)
    cursor_row_ = height_ - 1;
  if (cursor_col_ >= width_)
    cursor_col_ = width_ - 1;
  MarkDirty();
  OnGridChanged();
}

void Screen::SetGraphicsMode(int mode) {
  graphics_mode_ = mode;
  // The reference's modes: 1 is 320x200 with 4 colours, 2 is 640x200 monochrome,
  // 9 is 640x350 with 16, and 0 is text. Each keeps the 80x25 text grid; SCREEN 720 is
  // 1280x720 with a 160x45 grid, which is 8x16 cells exactly.
  switch (mode) {
  case 1:
    canvas_.Resize(320, 200);
    SetGridSize(kCols, kRows);
    break;
  case 2:
    canvas_.Resize(640, 200);
    SetGridSize(kCols, kRows);
    break;
  case 7:
    canvas_.Resize(320, 200);
    SetGridSize(kCols, kRows);
    break;
  case 8:
    canvas_.Resize(640, 200);
    SetGridSize(kCols, kRows);
    break;
  case 9:
    canvas_.Resize(640, 350);
    SetGridSize(kCols, kRows);
    break;
  case 12:
    canvas_.Resize(640, 480);
    SetGridSize(kCols, kRows);
    break;
  case 13:
    // The dialect's 256-colour VGA mode: 320x200, palette entries programmable.
    canvas_.Resize(320, 200);
    SetGridSize(kCols, kRows);
    break;
  case 3:
    // SCREEN 3 is 80x25 text on a colour card: the same text surface as SCREEN 0, so it stays a
    // text mode rather than becoming an empty graphics canvas.
    graphics_mode_ = kTextMode;
    canvas_.Resize(0, 0);
    SetGridSize(kCols, kRows);
    break;
  case 4:
  case 5:
    // 320x200 with 4 colours; the two modes differ only in which pages existed on the card.
    canvas_.Resize(320, 200);
    SetGridSize(kCols, kRows);
    break;
  case 6:
    // 640x200 with 2 colours.
    canvas_.Resize(640, 200);
    SetGridSize(kCols, kRows);
    break;
  case 10:
    // 640x350 monochrome EGA: the mode 9 canvas without colour.
    canvas_.Resize(640, 350);
    SetGridSize(kCols, kRows);
    break;
  case 11:
    // 640x480 monochrome: the mode 12 canvas without colour.
    canvas_.Resize(640, 480);
    SetGridSize(kCols, kRows);
    break;
  case kWideMode:
    canvas_.Resize(1280, 720);
    SetGridSize(160, 45);
    break;
  default:
    SetGridSize(kCols, kRows);
    break;
  }
  ResetPaletteForMode(mode);
  ResetGraphicsViewport(); // a new mode starts with the whole canvas as the viewport
  MarkDirty();
}

// Each mode has its own default colour set, and a SCREEN statement restores it (the
// reference notes that COLOR/PALETTE settings do not survive a mode change).
//   SCREEN 1, palette 0: 1 = green, 2 = red, 3 = brown
//   SCREEN 2: monochrome, 1 = white
//   SCREEN 9 and text mode: the identity mapping over the 16 CGA colours
void Screen::ResetPaletteForMode(int mode) {
  ResetPalette();
  if (mode == 1) {
    palette_[1] = 2; // green
    palette_[2] = 4; // red
    palette_[3] = 6; // brown (the reference's "yellow")
  } else if (mode == 2) {
    palette_[1] = 15; // white
  }
  // The RGB palette the display actually uses: the CGA colours for the compatible
  // modes, and a usable 256-entry ramp for SCREEN 720 (greys, then a hue sweep, so a
  // program that plots colours 0-255 gets something sensible before any PALETTE).
  for (std::size_t i = 0; i < rgb_palette_.size(); ++i) {
    if (mode == kWideMode) {
      const int step = static_cast<int>(i);
      if (step < 16) {
        const Rgb cga = CgaRgb(step);
        rgb_palette_[i] = {cga.r, cga.g, cga.b};
      } else if (step < 96) {
        const std::uint8_t level = static_cast<std::uint8_t>((step - 16) * 255 / 79);
        rgb_palette_[i] = {level, level, level};
      } else {
        // A hue sweep in 160 steps: three ramps with a rising middle channel.
        const int hue = (step - 96) % 160;
        const int phase = hue * 6 / 160;
        const int t = (hue * 255 / 26) % 255;
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        switch (phase) {
        case 0:
          r = static_cast<std::uint8_t>(255 - t);
          g = static_cast<std::uint8_t>(t);
          break;
        case 1:
          g = static_cast<std::uint8_t>(255 - t);
          b = static_cast<std::uint8_t>(t);
          break;
        case 2:
          b = static_cast<std::uint8_t>(255 - t);
          r = static_cast<std::uint8_t>(t);
          break;
        case 3:
          r = static_cast<std::uint8_t>(255 - t);
          b = static_cast<std::uint8_t>(t);
          break;
        case 4:
          b = static_cast<std::uint8_t>(255 - t);
          g = static_cast<std::uint8_t>(t);
          break;
        default:
          g = static_cast<std::uint8_t>(255 - t);
          r = static_cast<std::uint8_t>(t);
          break;
        }
        rgb_palette_[i] = {r, g, b};
      }
    } else {
      const Rgb mapped = CgaRgb(i < 16 ? palette_[i] : 0);
      rgb_palette_[i] = {mapped.r, mapped.g, mapped.b};
    }
  }
}

void Screen::SetTextWindow(int top, int bottom) {
  const int clamped_top = std::max(0, std::min(top, height_ - 1));
  const int clamped_bottom = std::max(0, std::min(bottom, height_ - 1));
  window_top_ = std::min(clamped_top, clamped_bottom);
  window_bottom_ = std::max(clamped_top, clamped_bottom);
  // The cursor must stay inside the window once one is set.
  cursor_row_ = std::max(window_top_, std::min(cursor_row_, window_bottom_));
  MarkDirty();
}

void Screen::ClearTextWindow() {
  for (int row = window_top_; row <= window_bottom_; ++row) {
    auto& cells = grid_[static_cast<std::size_t>(row)];
    FillRow(cells);
    auto& attrs = attr_grid_[static_cast<std::size_t>(row)];
    for (std::size_t c = 0; c < attrs.size(); ++c)
      attrs[c] = attr_;
  }
  cursor_row_ = window_top_;
  cursor_col_ = 0;
  MarkDirty();
}

void Screen::Clear() {
  for (auto& row : grid_)
    FillRow(row);
  for (auto& row : attr_grid_)
    for (std::size_t c = 0; c < row.size(); ++c)
      row[c] = attr_;
  cursor_row_ = 0;
  cursor_col_ = 0;
  MarkDirty();
}

void Screen::SetCursor(int row, int col) {
  cursor_row_ = std::max(window_top_, std::min(row, window_bottom_));
  cursor_col_ = std::max(0, std::min(col, width_ - 1));
  MarkDirty();
}

void Screen::PutCharAt(int row, int col, wchar_t ch) {
  if (row < 0 || row >= height_ || col < 0 || col >= width_)
    return;
  grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = ch;
  attr_grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = attr_;
  MarkDirty();
}

wchar_t Screen::CellAt(int row, int col) const {
  if (row < 0 || row >= height_ || col < 0 || col >= width_)
    return L' ';
  return grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void Screen::WriteAtCursor(const std::wstring& text) {
  for (wchar_t ch : text) {
    if (cursor_col_ >= width_) { // safety: never write past the right edge
      cursor_col_ = 0;
      ++cursor_row_;
      if (cursor_row_ >= height_)
        ScrollUp();
    }
    grid_[static_cast<std::size_t>(cursor_row_)][static_cast<std::size_t>(cursor_col_)] = ch;
    attr_grid_[static_cast<std::size_t>(cursor_row_)][static_cast<std::size_t>(cursor_col_)] =
        attr_;
    ++cursor_col_;
    // The guide's "printing exactly 80 characters inserts an automatic
    // CR/LF" quirk: wrap as soon as the right edge is filled, so a following
    // PRINT lands on the row after next (the intervening row stays blank).
    if (cursor_col_ >= width_) {
      cursor_col_ = 0;
      ++cursor_row_;
      if (cursor_row_ > window_bottom_)
        ScrollUp();
    }
  }
  MarkDirty();
}

void Screen::NewLine() {
  ++cursor_row_;
  cursor_col_ = 0;
  if (cursor_row_ > window_bottom_)
    ScrollUp();
  MarkDirty();
}

void Screen::AdvanceToNextZone() {
  cursor_col_ = (cursor_col_ / kZoneWidth + 1) * kZoneWidth;
  if (cursor_col_ >= width_) {
    cursor_col_ = 0;
    ++cursor_row_;
    if (cursor_row_ > window_bottom_)
      ScrollUp();
  }
  MarkDirty();
}

// Scrolls the text window up one line (VIEW PRINT confines this to the window).
void Screen::ScrollUp() {
  for (int row = window_top_; row < window_bottom_; ++row) {
    grid_[static_cast<std::size_t>(row)] = grid_[static_cast<std::size_t>(row + 1)];
    attr_grid_[static_cast<std::size_t>(row)] = attr_grid_[static_cast<std::size_t>(row + 1)];
  }
  std::vector<wchar_t> blank(static_cast<std::size_t>(width_), L' ');
  grid_[static_cast<std::size_t>(window_bottom_)] = blank;
  for (auto& cell_attr : attr_grid_[static_cast<std::size_t>(window_bottom_)])
    cell_attr = attr_;
  cursor_row_ = window_bottom_;
}

} // namespace newbasic::screen
