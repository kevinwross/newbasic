#pragma once

#include <cstdint>

namespace newbasic::screen {

// The sixteen CGA colours, which every compatible mode draws from. This lives in core
// so the Screen can resolve attributes to RGB without depending on the rendering
// library; the renderer uses the same table.
struct Rgb {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

inline Rgb CgaRgb(int index) {
  static const Rgb kColours[16] = {
      {0, 0, 0},     {0, 0, 170},    {0, 170, 0},    {0, 170, 170},
      {170, 0, 0},   {170, 0, 170},  {170, 85, 0},   {170, 170, 170},
      {85, 85, 85},  {85, 85, 255},  {85, 255, 85},  {85, 255, 255},
      {255, 85, 85}, {255, 85, 255}, {255, 255, 85}, {255, 255, 255},
  };
  // Out-of-range indices clamp, which is what the text renderer relied on before this
  // table moved into core.
  if (index < 0)
    return kColours[0];
  if (index > 15)
    return kColours[15];
  return kColours[index];
}

} // namespace newbasic::screen
