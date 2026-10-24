#pragma once

#include <memory>
#include <string>

#include "newbasic/window/window_screen.h"

namespace newbasic::screen {

// Windowed GPU text backend (M7). Renders the text grid in an owned window with
// D3D11: one background quad and one glyph quad per cell, sampling the CP437
// atlas. See docs/m7-d3d11-backend.md.
//
// Create() returns nullptr and fills `error` when no window or device can be
// obtained (headless session, no DXGI, WARP also unavailable); the caller must
// then fall back to ConsoleScreen. It never throws and never aborts.
class D3D11Screen final : public WindowScreen {
public:
  // Defined in the .cpp so the Windows and D3D headers stay out of this one.
  struct Impl;

  // `scale` multiplies the 8x16 text cell before the display DPI does, so the window
  // can be sized to taste: 1 is 640x400 at 96 DPI, 2 is 1280x800.
  static std::unique_ptr<WindowScreen> Create(std::string* error, int scale = 1);

  ~D3D11Screen() override;

  D3D11Screen(const D3D11Screen&) = delete;
  D3D11Screen& operator=(const D3D11Screen&) = delete;

  void Present() override;
  bool TakeBreak() override;
  bool TakeMouseClick(MouseClick* click) override;
  int MousePositionX() const override;
  int MousePositionY() const override;
  int MouseButtons() const override;
  int TakeMouseWheel() override;
  void InjectClick(int canvas_x, int canvas_y, int button) override;
  int GlyphWidth() const override;
  int GlyphHeight() const override;
  void PumpMessages() override;
  void OnGridChanged() override;
  void EnterFullScreen() override {}
  void ExitFullScreen() override {}
  std::wstring ReadLine() override;
  // Appends keys to the window's input queue, so a scripted session takes the same path
  // as a typed one.
  void InjectKeys(const std::wstring& keys) override;
  bool TryGetKey(wchar_t* key) override;
  bool InputExhausted() const override { return Closed(); }

  // True once the user closed the window; the caller can stop cleanly.
  bool Closed() const override;

  // Test hook (M12): reads back one pixel that the GPU actually rendered, taken by
  // canvas coordinate so it can be compared with the canvas directly. Without this,
  // a test can only prove the drawing path ran, never that the pixels arrived - which
  // is how a window that showed nothing stayed green.
  bool ReadBackCanvasPixel(int canvas_x, int canvas_y, std::uint8_t* rgba) override;

private:
  D3D11Screen();
  std::unique_ptr<Impl> impl_;
};

} // namespace newbasic::screen
