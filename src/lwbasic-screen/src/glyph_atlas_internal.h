#pragma once

#include "lwbasic/screen/glyph_atlas.h"

namespace lwbasic::screen {

// Platform font rasterisers. Each fills `pixels` with the 256 CP437 glyph cells
// and sets `ok`; the shared RenderGlyphAtlas then applies the procedural block
// glyphs and thickening that both platforms use.
void RasterizeGlyphsGdi(GlyphAtlas* atlas);
void RasterizeGlyphsStb(GlyphAtlas* atlas);

} // namespace lwbasic::screen
