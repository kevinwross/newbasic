#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "glyph_atlas_internal.h"

namespace newbasic::screen {
namespace {

struct Dib {
  HDC dc = nullptr;
  HBITMAP bitmap = nullptr;
  HGDIOBJ old_bitmap = nullptr;
  void* bits = nullptr;
};

// Creates a 32bpp top-down DIB section filled with black.
Dib CreateDib(int width, int height) {
  Dib dib;
  dib.dc = CreateCompatibleDC(nullptr);
  if (dib.dc == nullptr)
    return dib;

  BITMAPINFO info;
  std::memset(&info, 0, sizeof(info));
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = -height; // top-down
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;

  dib.bitmap = CreateDIBSection(dib.dc, &info, DIB_RGB_COLORS, &dib.bits, nullptr, 0);
  if (dib.bitmap == nullptr || dib.bits == nullptr) {
    DeleteDC(dib.dc);
    dib.dc = nullptr;
    return dib;
  }
  dib.old_bitmap = SelectObject(dib.dc, dib.bitmap);
  std::memset(dib.bits, 0, static_cast<std::size_t>(width) * height * 4);
  return dib;
}

void DestroyDib(Dib* dib) {
  if (dib->dc == nullptr)
    return;
  if (dib->old_bitmap != nullptr)
    SelectObject(dib->dc, dib->old_bitmap);
  if (dib->bitmap != nullptr)
    DeleteObject(dib->bitmap);
  DeleteDC(dib->dc);
}

// CP437 byte -> UTF-16, so GDI draws the dialect's glyph rather than Latin-1.
bool Cp437ToWide(unsigned char code, wchar_t* out, int* length) {
  char narrow = static_cast<char>(code);
  const int written = MultiByteToWideChar(437, 0, &narrow, 1, out, 2);
  if (written <= 0)
    return false;
  *length = written;
  return true;
}

} // namespace

void RasterizeGlyphsGdi(GlyphAtlas* atlas) {
  Dib dib = CreateDib(atlas->width, atlas->height);
  if (dib.dc == nullptr)
    return;

  // Choosing the face matters more than any quality hint. "Terminal" is a *bitmap* font that
  // only exists at fixed sizes: asked for 16x32 it stretches its 8x16 glyphs, which is the
  // chunky look a scaled-up window had, and being a bitmap it ignores antialiasing hints
  // entirely. So Terminal is only used at the native 8x16 cell, and larger cells ask for a
  // vector monospace face, which GDI is happy to rasterise at whatever height it is given.
  //
  // The candidates are tried in order and each one is confirmed with GetTextFaceW, because
  // CreateFontW happily reports success while substituting a different family.
  const DWORD quality = atlas->glyph_height <= 16 ? NONANTIALIASED_QUALITY : ANTIALIASED_QUALITY;
  const wchar_t* candidates[4];
  if (atlas->glyph_height <= 16) {
    candidates[0] = L"Terminal";
    candidates[1] = L"Consolas";
    candidates[2] = L"Lucida Console";
    candidates[3] = L"Courier New";
  } else {
    candidates[0] = L"Consolas";
    candidates[1] = L"Cascadia Mono";
    candidates[2] = L"Lucida Console";
    candidates[3] = L"Courier New";
  }
  HFONT font = nullptr;
  for (const wchar_t* face : candidates) {
    // Size the face to the cell rather than to its own idea of a point size. A TrueType face
    // at the cell height has an advance wider than an 8:16 cell, so each glyph overflows its
    // cell and the letters crowd together. Shrinking the height until the advance fits keeps
    // the spacing even, at the cost of a slightly smaller glyph with a little air around it.
    HFONT candidate = nullptr;
    for (int height = atlas->glyph_height; height >= 8; --height) {
      candidate = CreateFontW(-height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, quality,
                              FIXED_PITCH | FF_MODERN, face);
      if (candidate == nullptr)
        break;
      HGDIOBJ previous = SelectObject(dib.dc, candidate);
      wchar_t actual[LF_FACESIZE] = {0};
      GetTextFaceW(dib.dc, LF_FACESIZE, actual);
      if (wcscmp(actual, face) != 0) {
        // Not this family: stop shrinking and try the next candidate.
        SelectObject(dib.dc, previous);
        DeleteObject(candidate);
        candidate = nullptr;
        break;
      }
      SIZE extent = {0, 0};
      const wchar_t widest[] = L"W";
      GetTextExtentPoint32W(dib.dc, widest, 1, &extent);
      SelectObject(dib.dc, previous);
      if (extent.cx <= atlas->glyph_width)
        break; // the advance fits a cell
      DeleteObject(candidate);
      candidate = nullptr;
    }
    if (candidate != nullptr) {
      font = candidate;
      break;
    }
  }
  if (font == nullptr) {
    DestroyDib(&dib);
    return;
  }
  HGDIOBJ old_font = SelectObject(dib.dc, font);
  SetBkColor(dib.dc, RGB(0, 0, 0));
  SetTextColor(dib.dc, RGB(255, 255, 255));
  SetBkMode(dib.dc, OPAQUE);
  SetTextAlign(dib.dc, TA_TOP | TA_LEFT);

  // Centre the glyph vertically in its cell, since the chosen height is usually a little
  // smaller than the cell.
  TEXTMETRICW metrics;
  int top_offset = 0;
  if (GetTextMetricsW(dib.dc, &metrics))
    top_offset = (atlas->glyph_height - static_cast<int>(metrics.tmHeight)) / 2;
  if (top_offset < 0)
    top_offset = 0;

  bool drew_any = false;
  for (int glyph = 0; glyph < 256; ++glyph) {
    wchar_t wide[2] = {0, 0};
    int length = 0;
    if (!Cp437ToWide(static_cast<unsigned char>(glyph), wide, &length))
      continue;
    const int cell_x = (glyph % 16) * atlas->glyph_width;
    const int cell_y = (glyph / 16) * atlas->glyph_height;
    if (TextOutW(dib.dc, cell_x, cell_y + top_offset, wide, length) != 0)
      drew_any = true;
  }

  SelectObject(dib.dc, old_font);
  DeleteObject(font);
  GdiFlush();

  if (drew_any) {
    const std::uint8_t* source = static_cast<const std::uint8_t*>(dib.bits);
    for (int y = 0; y < atlas->height; ++y) {
      for (int x = 0; x < atlas->width; ++x) {
        const std::size_t offset =
            (static_cast<std::size_t>(y) * atlas->width + static_cast<std::size_t>(x)) * 4;
        // BGRA; luminance is enough because the atlas is monochrome.
        const int blue = source[offset + 0];
        const int green = source[offset + 1];
        const int red = source[offset + 2];
        const int value = (red * 30 + green * 59 + blue * 11) / 100;
        // Ink is all or nothing. A text-mode cell at 8x16 has one-pixel stems: keeping the
        // antialiased grey made the text thin and washed out, because a half-covered stem
        // then rendered at half the foreground colour. Thresholding gives the solid strokes
        // a bitmap text-mode font has. 96 keeps faint diagonal coverage without thickening.
        // Binary ink suits the native 8x16 bitmap font. A larger cell is rasterised from a
        // vector font, where keeping the coverage looks smooth rather than chunky.
        const bool binary = atlas->glyph_height <= 16;
        const std::uint8_t ink = (!binary || value >= 96) ? static_cast<std::uint8_t>(value) : 0;
        atlas->pixels[static_cast<std::size_t>(y) * atlas->width + static_cast<std::size_t>(x)] =
            ink;
      }
    }
    atlas->ok = true;
  }

  DestroyDib(&dib);
}

} // namespace newbasic::screen
