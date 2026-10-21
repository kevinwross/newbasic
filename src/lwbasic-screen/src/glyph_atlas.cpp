#include "lwbasic/screen/glyph_atlas.h"

#include <windows.h>

#include <cstring>

namespace lwbasic::screen {

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

namespace {

// Fills a glyph cell with a function of (x, y).
template <typename Predicate> void FillGlyph(GlyphAtlas* atlas, int glyph, Predicate ink) {
  const int cell_x = (glyph % 16) * atlas->glyph_width;
  const int cell_y = (glyph / 16) * atlas->glyph_height;
  for (int y = 0; y < atlas->glyph_height; ++y) {
    for (int x = 0; x < atlas->glyph_width; ++x) {
      const std::size_t offset = static_cast<std::size_t>(cell_y + y) * atlas->width +
                                 static_cast<std::size_t>(cell_x + x);
      atlas->pixels[offset] = ink(x, y) ? 255 : 0;
    }
  }
}

// The CP437 block and shade glyphs are drawn procedurally. System fonts
// routinely lack them (the reference font returned a 15-pixel stub for the solid
// block), and GW-BASIC programs use them for crude bar graphics via CHR$(), so
// they are generated rather than borrowed from a font.
void DrawProceduralGlyphs(GlyphAtlas* atlas) {
  const int half_width = atlas->glyph_width / 2;
  const int half_height = atlas->glyph_height / 2;
  // 0xB0 light shade (25%), 0xB1 medium (50%), 0xB2 dark (75%).
  FillGlyph(atlas, 0xB0, [](int x, int y) { return (x % 2 == 0) && (y % 2 == 0); });
  FillGlyph(atlas, 0xB1, [](int x, int y) { return (x + y) % 2 == 0; });
  FillGlyph(atlas, 0xB2, [](int x, int y) { return (x % 2 == 0) || (y % 2 == 0); });
  // Solid block.
  FillGlyph(atlas, 0xDB, [](int, int) { return true; });
  // Half blocks: 0xDC lower, 0xDD left, 0xDE right, 0xDF upper.
  FillGlyph(atlas, 0xDC, [half_height](int, int y) { return y >= half_height; });
  FillGlyph(atlas, 0xDD, [half_width](int x, int) { return x < half_width; });
  FillGlyph(atlas, 0xDE, [half_width](int x, int) { return x >= half_width; });
  FillGlyph(atlas, 0xDF, [half_height](int, int y) { return y < half_height; });
}

// Thickens text glyphs by one pixel to the right, inside the cell. An 8x16 cell gives
// one-pixel stems, which read as thin and faint on a modern display even when the ink is
// solid; a second column gives the weight of a bold text-mode font. Block and shade glyphs
// are skipped, since they already fill their cell and must not bleed into the next.
void ThickenGlyphs(GlyphAtlas* atlas) {
  const int gw = atlas->glyph_width;
  const int gh = atlas->glyph_height;
  const std::vector<std::uint8_t> source = atlas->pixels;
  for (int glyph = 0; glyph < 256; ++glyph) {
    if (glyph >= 0xB0 && glyph <= 0xDF)
      continue;
    const int cell_x = (glyph % 16) * gw;
    const int cell_y = (glyph / 16) * gh;
    for (int y = 0; y < gh; ++y) {
      for (int x = 0; x + 1 < gw; ++x) {
        const std::size_t from = static_cast<std::size_t>(cell_y + y) * atlas->width +
                                 static_cast<std::size_t>(cell_x + x);
        if (source[from] == 0)
          continue;
        atlas->pixels[from + 1] = 255;
      }
    }
  }
}

} // namespace

GlyphAtlas RenderGlyphAtlas(int glyph_width, int glyph_height) {
  GlyphAtlas atlas;
  atlas.glyph_width = glyph_width < 1 ? 1 : glyph_width;
  atlas.glyph_height = glyph_height < 1 ? 1 : glyph_height;
  atlas.width = atlas.glyph_width * 16;
  atlas.height = atlas.glyph_height * 16;

  Dib dib = CreateDib(atlas.width, atlas.height);
  if (dib.dc == nullptr)
    return atlas;

  // Choosing the face matters more than any quality hint. "Terminal" is a *bitmap* font that
  // only exists at fixed sizes: asked for 16x32 it stretches its 8x16 glyphs, which is the
  // chunky look a scaled-up window had, and being a bitmap it ignores antialiasing hints
  // entirely. So Terminal is only used at the native 8x16 cell, and larger cells ask for a
  // vector monospace face, which GDI is happy to rasterise at whatever height it is given.
  //
  // The candidates are tried in order and each one is confirmed with GetTextFaceW, because
  // CreateFontW happily reports success while substituting a different family.
  const DWORD quality = atlas.glyph_height <= 16 ? NONANTIALIASED_QUALITY : ANTIALIASED_QUALITY;
  const wchar_t* candidates[4];
  if (atlas.glyph_height <= 16) {
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
    for (int height = atlas.glyph_height; height >= 8; --height) {
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
      if (extent.cx <= atlas.glyph_width)
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
    return atlas;
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
    top_offset = (atlas.glyph_height - static_cast<int>(metrics.tmHeight)) / 2;
  if (top_offset < 0)
    top_offset = 0;

  bool drew_any = false;
  for (int glyph = 0; glyph < 256; ++glyph) {
    wchar_t wide[2] = {0, 0};
    int length = 0;
    if (!Cp437ToWide(static_cast<unsigned char>(glyph), wide, &length))
      continue;
    const int cell_x = (glyph % 16) * atlas.glyph_width;
    const int cell_y = (glyph / 16) * atlas.glyph_height;
    if (TextOutW(dib.dc, cell_x, cell_y + top_offset, wide, length) != 0)
      drew_any = true;
  }

  SelectObject(dib.dc, old_font);
  DeleteObject(font);
  GdiFlush();

  if (drew_any) {
    atlas.pixels.resize(static_cast<std::size_t>(atlas.width) * atlas.height);
    const std::uint8_t* source = static_cast<const std::uint8_t*>(dib.bits);
    for (int y = 0; y < atlas.height; ++y) {
      for (int x = 0; x < atlas.width; ++x) {
        const std::size_t offset =
            (static_cast<std::size_t>(y) * atlas.width + static_cast<std::size_t>(x)) * 4;
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
        const bool binary = atlas.glyph_height <= 16;
        const std::uint8_t ink = (!binary || value >= 96) ? static_cast<std::uint8_t>(value) : 0;
        atlas.pixels[static_cast<std::size_t>(y) * atlas.width + static_cast<std::size_t>(x)] = ink;
      }
    }
    DrawProceduralGlyphs(&atlas);
    ThickenGlyphs(&atlas);
    atlas.ok = true;
  }

  DestroyDib(&dib);
  return atlas;
}

} // namespace lwbasic::screen
