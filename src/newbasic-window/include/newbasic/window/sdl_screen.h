#pragma once

#include <memory>
#include <string>

#include "newbasic/window/window_screen.h"

namespace newbasic::screen {

// SDL2 windowed backend (Linux; optionally Windows). It composes the text grid
// and graphics canvas into one RGBA frame and presents it through an SDL2
// renderer. The CPU composition keeps the renderer simple and mirrors the
// deterministic view dump used by --gpu-shot.
class SdlScreen final : public WindowScreen {
public:
  struct Impl;

  static std::unique_ptr<WindowScreen> Create(std::string* error, int scale = 1);

  ~SdlScreen() override;

  SdlScreen(const SdlScreen&) = delete;
  SdlScreen& operator=(const SdlScreen&) = delete;

  void Present() override;
  void OnGridChanged() override;
  void EnterFullScreen() override {}
  void ExitFullScreen() override {}
  void PumpMessages() override;
  std::wstring ReadLine() override;
  bool TryGetKey(wchar_t* key) override;
  bool InputExhausted() const override { return Closed(); }
  bool Closed() const override;
  bool TakeBreak() override;
  bool TakeMouseClick(MouseClick* click) override;
  int MousePositionX() const override;
  int MousePositionY() const override;
  int MouseButtons() const override;
  int TakeMouseWheel() override;

  void InjectClick(int canvas_x, int canvas_y, int button) override;
  void InjectKeys(const std::wstring& keys) override;
  bool ReadBackCanvasPixel(int canvas_x, int canvas_y, std::uint8_t* rgba) override;

  int GlyphWidth() const override;
  int GlyphHeight() const override;

private:
  SdlScreen();
  // Maps a window client point through the letterboxed graphics rect to a canvas
  // pixel. Returns false when the point is outside the graphics canvas.
  bool MapMousePoint(int window_x, int window_y, int* canvas_x, int* canvas_y) const;
  std::unique_ptr<Impl> impl_;
};

} // namespace newbasic::screen
