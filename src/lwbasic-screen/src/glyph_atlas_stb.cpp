#include "glyph_atlas_internal.h"

#include "lwbasic/screen/screen.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

namespace lwbasic::screen {
namespace {

std::vector<unsigned char> ReadFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return {};
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in),
                                    std::istreambuf_iterator<char>());
}

std::vector<unsigned char> LoadFont() {
  std::vector<std::string> candidates;
  if (const char* configured = std::getenv("LWBASIC_FONT"); configured != nullptr &&
                                                             *configured != '\0') {
    candidates.emplace_back(configured);
  }
#if defined(LWBASIC_FONT_PATH)
  candidates.emplace_back(LWBASIC_FONT_PATH);
#endif
  candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
  candidates.emplace_back("/usr/share/fonts/TTF/DejaVuSansMono.ttf");
  candidates.emplace_back("/usr/share/fonts/dejavu/DejaVuSansMono.ttf");
  candidates.emplace_back("/Library/Fonts/DejaVuSansMono.ttf");
  for (const std::string& path : candidates) {
    std::vector<unsigned char> data = ReadFile(path);
    if (!data.empty())
      return data;
  }
  return {};
}

} // namespace

void RasterizeGlyphsStb(GlyphAtlas* atlas) {
  const std::vector<unsigned char> data = LoadFont();
  if (data.empty())
    return;

  stbtt_fontinfo font{};
  const int offset = stbtt_GetFontOffsetForIndex(data.data(), 0);
  if (offset < 0 || !stbtt_InitFont(&font, data.data(), offset))
    return;

  // Pick a pixel height whose "W" advance fits the cell, mirroring the GDI
  // backend's shrink-to-fit rule. Every CP437 glyph is then centred in its
  // cell, which keeps the monospace grid even.
  float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(atlas->glyph_height));
  for (int height = atlas->glyph_height; height >= 8; --height) {
    const float candidate = stbtt_ScaleForPixelHeight(&font, static_cast<float>(height));
    int advance = 0;
    int left_bearing = 0;
    stbtt_GetCodepointHMetrics(&font, 'W', &advance, &left_bearing);
    (void)left_bearing;
    if (static_cast<float>(advance) * candidate <= static_cast<float>(atlas->glyph_width)) {
      scale = candidate;
      break;
    }
  }

  int ascent = 0;
  int descent = 0;
  int line_gap = 0;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
  (void)line_gap;
  const int text_height =
      static_cast<int>(std::ceil(static_cast<float>(ascent - descent) * scale));
  int top_offset = (atlas->glyph_height - text_height) / 2;
  if (top_offset < 0)
    top_offset = 0;
  const int baseline = top_offset + static_cast<int>(std::ceil(static_cast<float>(ascent) * scale));

  bool drew_any = false;
  for (int glyph = 0; glyph < 256; ++glyph) {
    const wchar_t unicode = Cp437ToUnicode(static_cast<unsigned char>(glyph));
    int glyph_index = stbtt_FindGlyphIndex(&font, static_cast<int>(unicode));
    if (glyph_index == 0)
      glyph_index = stbtt_FindGlyphIndex(&font, '?');
    if (glyph_index == 0)
      continue;

    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    stbtt_GetGlyphBitmapBox(&font, glyph_index, scale, scale, &x0, &y0, &x1, &y1);
    const int glyph_width = x1 - x0;
    const int glyph_height = y1 - y0;
    if (glyph_width <= 0 || glyph_height <= 0)
      continue;

    std::vector<unsigned char> bitmap(static_cast<std::size_t>(glyph_width) * glyph_height, 0);
    stbtt_MakeGlyphBitmap(&font, bitmap.data(), glyph_width, glyph_height, glyph_width, scale,
                          scale, glyph_index);

    const int cell_x = (glyph % 16) * atlas->glyph_width;
    const int cell_y = (glyph / 16) * atlas->glyph_height;
    const int dst_x = cell_x + (atlas->glyph_width - glyph_width) / 2;
    const int dst_y = cell_y + baseline + y0;
    const bool binary = atlas->glyph_height <= 16;
    for (int y = 0; y < glyph_height; ++y) {
      const int ty = dst_y + y;
      if (ty < cell_y || ty >= cell_y + atlas->glyph_height)
        continue;
      for (int x = 0; x < glyph_width; ++x) {
        const int tx = dst_x + x;
        if (tx < cell_x || tx >= cell_x + atlas->glyph_width)
          continue;
        int value = bitmap[static_cast<std::size_t>(y) * glyph_width + x];
        if (binary && value < 96)
          value = 0;
        if (value == 0)
          continue;
        const std::size_t offset = static_cast<std::size_t>(ty) * atlas->width +
                                   static_cast<std::size_t>(tx);
        atlas->pixels[offset] = static_cast<std::uint8_t>(value);
        drew_any = true;
      }
    }
  }

  atlas->ok = drew_any;
}

} // namespace lwbasic::screen
