#pragma once

#include <cstdint>
#include <vector>

namespace lwbasic::screen {

// A 256-glyph CP437 atlas rasterised on the CPU (M7). The GPU backend uploads
// `pixels` as a single-channel texture; tests inspect it directly. GDI does the
// rasterising so there is no embedded font-ROM data to license or maintain.
//
// Layout: 16x16 glyph cells, each glyph_width x glyph_height pixels, row-major,
// one byte per pixel where 0 is background and 255 is ink.
struct GlyphAtlas {
  int glyph_width = 8;
  int glyph_height = 16;
  int width = 0;                    // glyph_width * 16
  int height = 0;                   // glyph_height * 16
  std::vector<std::uint8_t> pixels; // width * height, row-major
  bool ok = false;                  // false when GDI could not rasterise

  // Value at (x, y), or 0 outside the atlas.
  std::uint8_t At(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height)
      return 0;
    return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                  static_cast<std::size_t>(x)];
  }

  // True when the given glyph cell has any ink.
  bool GlyphHasInk(int glyph) const {
    const int g = glyph < 0 ? 0 : (glyph > 255 ? 255 : glyph);
    const int cell_x = (g % 16) * glyph_width;
    const int cell_y = (g / 16) * glyph_height;
    for (int y = 0; y < glyph_height; ++y) {
      for (int x = 0; x < glyph_width; ++x) {
        if (At(cell_x + x, cell_y + y) != 0)
          return true;
      }
    }
    return false;
  }

  // Number of inked pixels in a glyph cell (useful for distinctness checks).
  int GlyphInkCount(int glyph) const {
    const int g = glyph < 0 ? 0 : (glyph > 255 ? 255 : glyph);
    const int cell_x = (g % 16) * glyph_width;
    const int cell_y = (g / 16) * glyph_height;
    int count = 0;
    for (int y = 0; y < glyph_height; ++y) {
      for (int x = 0; x < glyph_width; ++x) {
        if (At(cell_x + x, cell_y + y) != 0)
          ++count;
      }
    }
    return count;
  }
};

// Rasterises all 256 CP437 glyphs. Returns ok=false when no suitable font or DC
// is available (a headless session without fonts); callers must fall back.
GlyphAtlas RenderGlyphAtlas(int glyph_width = 8, int glyph_height = 16);

} // namespace lwbasic::screen
