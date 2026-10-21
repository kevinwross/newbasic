#include "newbasic/screen/glyph_atlas.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "glyph_atlas_internal.h"

namespace newbasic::screen {
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
  atlas.glyph_width = std::max(1, glyph_width);
  atlas.glyph_height = std::max(1, glyph_height);
  atlas.width = atlas.glyph_width * 16;
  atlas.height = atlas.glyph_height * 16;
  atlas.pixels.assign(static_cast<std::size_t>(atlas.width) * atlas.height, 0);

#if defined(_WIN32)
  RasterizeGlyphsGdi(&atlas);
#else
  RasterizeGlyphsStb(&atlas);
#endif
  if (!atlas.ok)
    return atlas;

  DrawProceduralGlyphs(&atlas);
  ThickenGlyphs(&atlas);
  return atlas;
}

} // namespace newbasic::screen
