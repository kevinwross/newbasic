#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "newbasic/screen/canvas.h"
#include "newbasic/screen/palette.h"

namespace newbasic::screen {

// Pure geometry and colour helpers for a GPU text renderer (M7). Deliberately
// free of D3D, GDI and windowing: everything here is deterministic arithmetic,
// so the parts of the windowed backend that *can* be tested headlessly are
// tested, and only device creation and presentation are left to a manual check.

struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

// The 16 CGA colours GW-BASIC uses for text attributes (IRGB order). The table lives in
// core (palette.h) so the Screen can resolve attributes to RGB without depending on this
// library; this wraps it in the renderer's Color type.
inline Color CgaPaletteColor(int index) {
  const Rgb rgb = CgaRgb(index);
  return Color{rgb.r, rgb.g, rgb.b, 255};
}

// A CGA text attribute packs the foreground in the low nibble and the background
// in the high nibble, exactly as the text-mode screen buffer does.
inline Color ForegroundColor(std::uint8_t attribute) {
  return CgaPaletteColor(attribute & 0x0F);
}
inline Color BackgroundColor(std::uint8_t attribute) {
  return CgaPaletteColor((attribute >> 4) & 0x0F);
}

// One glyph quad: four corners in normalised device coordinates (D3D convention,
// y up) plus texture coordinates into a 16x16 glyph atlas.
struct QuadVertex {
  float x = 0.0f;
  float y = 0.0f;
  float u = 0.0f;
  float v = 0.0f;
};

struct GlyphQuad {
  QuadVertex top_left;
  QuadVertex top_right;
  QuadVertex bottom_left;
  QuadVertex bottom_right;
};

// Atlas geometry: 256 CP437 glyphs in a 16x16 grid, so each glyph occupies
// 1/16 of the texture in each axis.
constexpr int kAtlasColumns = 16;
constexpr int kAtlasRows = 16;

// Builds the quad for cell (column, row) of a columns x rows text grid.
// Row 0 is the *top* row: normalised device coordinates have y up, so the top
// edge of row 0 is y = +1 and the bottom edge of the last row is y = -1.
inline GlyphQuad CellQuad(int column, int row, int columns, int rows, int glyph) {
  GlyphQuad quad;
  const float cell_width = 2.0f / static_cast<float>(columns);
  const float cell_height = 2.0f / static_cast<float>(rows);

  const float left = -1.0f + static_cast<float>(column) * cell_width;
  const float right = left + cell_width;
  const float top = 1.0f - static_cast<float>(row) * cell_height;
  const float bottom = top - cell_height;

  const int glyph_index = glyph < 0 ? 0 : (glyph > 255 ? 255 : glyph);
  const float u0 =
      static_cast<float>(glyph_index % kAtlasColumns) / static_cast<float>(kAtlasColumns);
  const float v0 = static_cast<float>(glyph_index / kAtlasColumns) / static_cast<float>(kAtlasRows);
  const float u1 = u0 + 1.0f / static_cast<float>(kAtlasColumns);
  const float v1 = v0 + 1.0f / static_cast<float>(kAtlasRows);

  quad.top_left = {left, top, u0, v0};
  quad.top_right = {right, top, u1, v0};
  quad.bottom_left = {left, bottom, u0, v1};
  quad.bottom_right = {right, bottom, u1, v1};
  return quad;
}

// Converts an indexed-colour canvas to RGBA8 through the screen's RGB palette (M12
// phases 3-4). Keeping this CPU-side and pure makes the conversion testable, and gives
// the D3D11 backend bytes to upload without it needing to know about palettes. The
// palette comes from Screen::PaletteRgb(), which is the sixteen CGA colours for the
// compatible modes and a real 256-entry ramp for SCREEN 720.
inline std::vector<std::uint8_t>
CanvasToRgba(const Canvas& canvas, const std::array<std::array<std::uint8_t, 3>, 256>& palette) {
  std::vector<std::uint8_t> rgba(
      static_cast<std::size_t>(canvas.Width()) * static_cast<std::size_t>(canvas.Height()) * 4, 0);
  for (int y = 0; y < canvas.Height(); ++y) {
    for (int x = 0; x < canvas.Width(); ++x) {
      const std::uint8_t index = canvas.Pixel(x, y);
      const std::array<std::uint8_t, 3> entry = palette[static_cast<std::size_t>(index)];
      const Color colour{entry[0], entry[1], entry[2], 255};
      const std::size_t offset =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.Width()) +
           static_cast<std::size_t>(x)) *
          4;
      rgba[offset + 0] = colour.r;
      rgba[offset + 1] = colour.g;
      rgba[offset + 2] = colour.b;
      rgba[offset + 3] = 255;
    }
  }
  return rgba;
}

} // namespace newbasic::screen
