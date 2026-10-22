#pragma once

#include <cstdint>
#include <string>

#include "newbasic/screen/screen.h"

namespace newbasic::screen {

// Common interface for windowed backends (D3D11 on Windows, SDL2 on Linux).
// The CLI and the GPU tests need a little more than Screen: scripted input and
// a read-back of what the backend actually presented.
class WindowScreen : public Screen {
public:
  // Scripted input, used by --keys / --clicks and the [.gpu] tests. The click
  // coordinates are in canvas pixels.
  virtual void InjectClick(int canvas_x, int canvas_y, int button) = 0;
  virtual void InjectKeys(const std::wstring& keys) = 0;

  // Reads one pixel the backend actually presented, by canvas coordinate.
  virtual bool ReadBackCanvasPixel(int canvas_x, int canvas_y, std::uint8_t* rgba) = 0;

  // A window is not a terminal: an interactive SHELL has nowhere to attach.
  bool CanRunInteractiveShell() const override { return false; }
};

} // namespace newbasic::screen
