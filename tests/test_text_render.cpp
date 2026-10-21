// M7: headless-testable pieces of the GPU text renderer - palette and quad
// geometry. The window/device/present path cannot be tested here (see
// docs/m7-d3d11-backend.md), so everything deterministic is covered instead.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include "newbasic/screen/glyph_atlas.h"
#include "newbasic/screen/text_render.h"
#include "newbasic/window/screen_factory.h"
#include "newbasic/window/window_screen.h"

using newbasic::screen::BackgroundColor;
using newbasic::screen::CellQuad;
using newbasic::screen::CgaPaletteColor;
using newbasic::screen::ForegroundColor;

TEST_CASE("Renderer: the 16 CGA colours", "[render]") {
  // Anchors of the IRGB palette the dialect uses.
  CHECK(CgaPaletteColor(0).r == 0);
  CHECK(CgaPaletteColor(0).g == 0);
  CHECK(CgaPaletteColor(0).b == 0);
  CHECK(CgaPaletteColor(1).b == 170); // blue
  CHECK(CgaPaletteColor(4).r == 170); // red
  CHECK(CgaPaletteColor(7).r == 170); // light gray, the default foreground
  CHECK(CgaPaletteColor(7).g == 170);
  CHECK(CgaPaletteColor(15).r == 255); // white
  CHECK(CgaPaletteColor(15).g == 255);
  CHECK(CgaPaletteColor(15).b == 255);
  // Out-of-range indexes clamp rather than reading out of bounds.
  CHECK(CgaPaletteColor(-1).r == 0);
  CHECK(CgaPaletteColor(99).r == 255);
  // Every entry is opaque.
  for (int i = 0; i < 16; ++i)
    CHECK(CgaPaletteColor(i).a == 255);
}

TEST_CASE("Renderer: text attributes split into foreground and background", "[render]") {
  // Default GW-BASIC attribute: light gray on black.
  CHECK(ForegroundColor(0x07).r == 170);
  CHECK(BackgroundColor(0x07).r == 0);
  // 0x1E = blue background (1), yellow foreground (14).
  CHECK(ForegroundColor(0x1E).b == 85);
  CHECK(ForegroundColor(0x1E).g == 255);
  CHECK(BackgroundColor(0x1E).b == 170);
  // 0x4F = red background (4), white foreground (15).
  CHECK(ForegroundColor(0x4F).g == 255);
  CHECK(ForegroundColor(0x4F).b == 255);
  CHECK(BackgroundColor(0x4F).r == 170);
}

TEST_CASE("Renderer: cell quads tile the grid without gaps", "[render]") {
  // Row 0 sits at the top: normalised device coordinates have y up.
  const auto first = CellQuad(0, 0, 80, 25, 'A');
  CHECK(first.top_left.x == -1.0f);
  CHECK(first.top_left.y == 1.0f);
  CHECK(first.bottom_right.x == first.top_right.x); // one cell wide
  CHECK(first.bottom_right.y < first.top_left.y);

  // Adjacent cells share an edge exactly, so there are no seams.
  const auto next = CellQuad(1, 0, 80, 25, 'B');
  CHECK(next.top_left.x == first.top_right.x);
  CHECK(next.top_left.y == first.top_left.y);

  // The last cell of the grid ends exactly at the far edges (within float
  // rounding: 2/25 * 25 is not bit-exact).
  const auto last = CellQuad(79, 24, 80, 25, 'Z');
  CHECK(last.top_right.x == Catch::Approx(1.0f));
  CHECK(last.bottom_right.y == Catch::Approx(-1.0f));

  // Cell aspect: the 80x25 grid is wider than tall in NDC, as it should be.
  const float cell_width = first.top_right.x - first.top_left.x;
  const float cell_height = first.top_left.y - first.bottom_left.y;
  CHECK(cell_width > 0.0f);
  CHECK(cell_height > 0.0f);
  CHECK(cell_width < cell_height); // 80 columns => narrower cells in NDC
}

TEST_CASE("Renderer: texture coordinates pick the atlas cell", "[render]") {
  // Glyph 0 is the top-left atlas cell; its v range starts at 0.
  const auto glyph_zero = CellQuad(0, 0, 80, 25, 0);
  CHECK(glyph_zero.top_left.u == 0.0f);
  CHECK(glyph_zero.top_left.v == 0.0f);
  CHECK(glyph_zero.top_right.u == 1.0f / 16.0f);
  CHECK(glyph_zero.bottom_left.v == 1.0f / 16.0f);

  // Glyph 1 is the next cell across; glyph 16 starts the second atlas row.
  const auto glyph_one = CellQuad(0, 0, 80, 25, 1);
  CHECK(glyph_one.top_left.u == 1.0f / 16.0f);
  const auto glyph_sixteen = CellQuad(0, 0, 80, 25, 16);
  CHECK(glyph_sixteen.top_left.u == 0.0f);
  CHECK(glyph_sixteen.top_left.v == 1.0f / 16.0f);

  // A high CP437 glyph (0xDB, the solid block) still lands inside the atlas.
  const auto block = CellQuad(0, 0, 80, 25, 0xDB);
  CHECK(block.top_right.u <= 1.0f);
  CHECK(block.bottom_left.v <= 1.0f);

  // Out-of-range glyph numbers clamp instead of producing coordinates outside
  // the atlas (which would sample garbage).
  const auto high = CellQuad(0, 0, 80, 25, 999);
  CHECK(high.top_right.u <= 1.0f);
  const auto negative = CellQuad(0, 0, 80, 25, -5);
  CHECK(negative.top_left.u == 0.0f);
}

TEST_CASE("Renderer: the glyph atlas rasterises the CP437 set", "[render]") {
  const newbasic::screen::GlyphAtlas atlas = newbasic::screen::RenderGlyphAtlas();
  if (!atlas.ok) {
    // A session without fonts or a DC cannot rasterise. That is a legitimate
    // environment, not a failure: the GPU backend falls back in exactly this
    // case, so record it and skip the pixel checks.
    WARN("GDI glyph rasterisation unavailable in this session; skipping atlas checks");
    SUCCEED();
    return;
  }

  // 256 glyphs in a 16x16 grid; the cell size depends on the rasterisation scale.
  CHECK(atlas.width == atlas.glyph_width * 16);
  CHECK(atlas.height == atlas.glyph_height * 16);
  CHECK(atlas.pixels.size() ==
        static_cast<std::size_t>(atlas.width) * static_cast<std::size_t>(atlas.height));

  // Letters have ink, the space glyph does not, and different letters differ.
  CHECK(atlas.GlyphHasInk('A'));
  CHECK(atlas.GlyphHasInk('Z'));
  CHECK_FALSE(atlas.GlyphHasInk(' '));
  CHECK(atlas.GlyphInkCount('A') > 0);
  CHECK(atlas.GlyphInkCount('A') != atlas.GlyphInkCount('B'));

  // The solid block (0xDB) is the densest glyph, which also proves the CP437
  // high range was rasterised rather than substituted.
  CHECK(atlas.GlyphInkCount(0xDB) > atlas.GlyphInkCount('A'));
  // A shade character (0xB0) has some - but not full - ink.
  CHECK(atlas.GlyphHasInk(0xB0));
  CHECK(atlas.GlyphInkCount(0xB0) < atlas.GlyphInkCount(0xDB));

  // Out-of-range glyph numbers are clamped by the accessors.
  CHECK(atlas.GlyphInkCount(-3) == atlas.GlyphInkCount(0));
  CHECK(atlas.GlyphInkCount(999) == atlas.GlyphInkCount(255));
}

// ---------------------------------------------------------------------------
// Manual GPU check. Hidden from CI by the [.tag] convention, because a build
// machine may have no device (and this opens a real window). Run by hand:
//
//   out\tests\Release\newbasic-tests.exe "[.gpu]" -s
//
// It proves the whole pipeline constructs and presents one frame. Whether the
// picture is *correct* still needs a human eye - see docs/m7-d3d11-backend.md.
// ---------------------------------------------------------------------------
TEST_CASE("Renderer: the window backend constructs and presents a frame", "[.gpu]") {
  std::string error;
  const std::unique_ptr<newbasic::screen::WindowScreen> screen =
      newbasic::screen::CreateWindowScreen(&error);
  if (screen == nullptr) {
    WARN("no window backend available here (" << error << "); skipping the GPU check");
    SUCCEED();
    return;
  }
  // Draw something so the glyph path runs, then present once.
  screen->Clear();
  screen->WriteAtCursor(L"GPU PATH OK");
  screen->PresentNow();
  CHECK(screen->CursorCol() == 11);
  CHECK(screen->CellAt(0, 0) == L'G');

  // And a graphics frame, which exercises the canvas texture, the blit shader and
  // the glyph overlay without backgrounds (M12 phase 4).
  screen->SetGraphicsMode(1);
  screen->DrawGraphicsLine(0, 0, 100, 100, 2);
  screen->Plot(10, 30, 4); // off the diagonal, so the line cannot overwrite it
  screen->DrawGraphicsEllipse(160, 100, 40, 48, 0.0, 6.283185307179586, 3);
  screen->WriteAtCursor(L"GRAPHICS");
  screen->PresentNow();
  CHECK(screen->Graphics().Pixel(10, 30) == 4);
  CHECK(screen->GraphicsMode() == 1);

  // What matters is not that the path ran but what the GPU actually drew: read a pixel
  // back from the render target. A window that showed nothing once kept this test
  // green because it never looked at the pixels.
  // The patches are a few canvas pixels wide on purpose: a canvas pixel covers more
  // than one screen pixel once the canvas is scaled up, so a single plotted pixel can
  // have its sample land on a neighbour.
  screen->ClearGraphics(0);                      // a known background
  screen->FillGraphicsBox(158, 98, 162, 102, 4); // red at the canvas centre
  screen->Present();
  std::uint8_t rgba[4] = {0, 0, 0, 0};
  const bool read = screen->ReadBackCanvasPixel(160, 100, rgba);
  INFO("readback ok=" << read << " rgba=" << static_cast<int>(rgba[0]) << ","
                      << static_cast<int>(rgba[1]) << "," << static_cast<int>(rgba[2]));
  REQUIRE(read);
  CHECK(rgba[0] > 120); // red
  CHECK(rgba[1] < 90);
  CHECK(rgba[2] < 90);

  // The clear reaches the target too: a pixel far from anything drawn is black.
  std::uint8_t dark[4] = {255, 255, 255, 255};
  REQUIRE(screen.get()->ReadBackCanvasPixel(5, 195, dark));
  CHECK(dark[0] < 60);
  CHECK(dark[1] < 60);
  CHECK(dark[2] < 60);

  // SCREEN 2 is monochrome at the GPU, not just in the palette array.
  screen->SetGraphicsMode(2);
  screen->ClearGraphics(0);
  screen->FillGraphicsBox(8, 8, 12, 12, 1);
  screen->Present();
  std::uint8_t white[4] = {0, 0, 0, 0};
  REQUIRE(screen.get()->ReadBackCanvasPixel(10, 10, white));
  CHECK(white[0] > 200);
  CHECK(white[1] > 200);
  CHECK(white[2] > 200);

  // VIEW clips at the target: a plot outside the viewport never reaches the GPU.
  screen->SetGraphicsMode(1);
  screen->ClearGraphics(0);
  screen->SetGraphicsViewport(50, 50, 150, 120, -1, -1);
  screen->FillGraphicsBox(196, 56, 204, 64, 4); // outside the viewport
  screen->FillGraphicsBox(56, 56, 64, 64, 4);   // inside it
  screen->Present();
  std::uint8_t inside[4] = {0, 0, 0, 0};
  std::uint8_t outside[4] = {255, 255, 255, 255};
  REQUIRE(screen.get()->ReadBackCanvasPixel(60, 60, inside));
  REQUIRE(screen.get()->ReadBackCanvasPixel(200, 60, outside));
  CHECK(inside[0] > 120); // the plot inside was drawn
  CHECK(outside[0] < 60); // the clipped one was not
  CHECK(outside[1] < 60);
  screen->ResetGraphicsViewport();

  // PALETTE reaches the GPU: attribute 1 remapped to magenta renders magenta.
  screen->ClearGraphics(0);
  screen->SetPaletteEntry(1, 5);
  screen->FillGraphicsBox(116, 116, 124, 124, 1);
  screen->Present();
  std::uint8_t magenta[4] = {0, 0, 0, 0};
  REQUIRE(screen.get()->ReadBackCanvasPixel(120, 120, magenta));
  CHECK(magenta[0] > 120); // red
  CHECK(magenta[1] < 90);  // no green
  CHECK(magenta[2] > 120); // blue

  // A second mode reaches the GPU too: SCREEN 9 is 640x350 with sixteen colours, so the
  // letterbox and the texture size both have to follow the canvas.
  screen->SetGraphicsMode(9);
  screen->ClearGraphics(0);
  screen->SetPaletteEntry(7, 5); // attribute 7 as magenta, to prove it is not the default
  screen->FillGraphicsBox(312, 167, 328, 183, 7);
  screen->Present();
  std::uint8_t mode9[4] = {0, 0, 0, 0};
  REQUIRE(screen.get()->ReadBackCanvasPixel(320, 175, mode9));
  CHECK(mode9[0] > 120);
  CHECK(mode9[1] < 90);
  CHECK(mode9[2] > 120);
}
