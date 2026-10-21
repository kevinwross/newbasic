#include "lwbasic/window/sdl_screen.h"

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "lwbasic/screen/glyph_atlas.h"

namespace lwbasic::screen {
namespace {

constexpr const char* kWindowTitle = "lwbasic";
constexpr int kInitialScale = 1;

std::wstring DecodeUtf8(const char* text) {
  std::wstring out;
  const auto* bytes = reinterpret_cast<const unsigned char*>(text);
  while (*bytes != 0) {
    std::uint32_t code = 0;
    int extra = 0;
    if ((*bytes & 0x80) == 0) {
      code = *bytes++;
    } else if ((*bytes & 0xE0) == 0xC0) {
      code = *bytes++ & 0x1F;
      extra = 1;
    } else if ((*bytes & 0xF0) == 0xE0) {
      code = *bytes++ & 0x0F;
      extra = 2;
    } else if ((*bytes & 0xF8) == 0xF0) {
      code = *bytes++ & 0x07;
      extra = 3;
    } else {
      ++bytes;
      continue;
    }
    for (int i = 0; i < extra && (*bytes & 0xC0) == 0x80; ++i)
      code = (code << 6) | (*bytes++ & 0x3F);
    out.push_back(static_cast<wchar_t>(code));
  }
  return out;
}

std::uint32_t Pack(const std::array<std::uint8_t, 3>& rgb) {
  return 0xFF000000u | (static_cast<std::uint32_t>(rgb[0]) << 16) |
         (static_cast<std::uint32_t>(rgb[1]) << 8) | static_cast<std::uint32_t>(rgb[2]);
}

std::uint32_t Blend(std::uint32_t under, const std::array<std::uint8_t, 3>& ink, int coverage) {
  if (coverage <= 0)
    return under;
  if (coverage >= 255)
    return Pack(ink);
  const int inverse = 255 - coverage;
  const int r =
      (static_cast<int>(ink[0]) * coverage + static_cast<int>((under >> 16) & 0xFF) * inverse) /
      255;
  const int g =
      (static_cast<int>(ink[1]) * coverage + static_cast<int>((under >> 8) & 0xFF) * inverse) / 255;
  const int b =
      (static_cast<int>(ink[2]) * coverage + static_cast<int>(under & 0xFF) * inverse) / 255;
  return 0xFF000000u | (static_cast<std::uint32_t>(r) << 16) |
         (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b);
}

struct FrameRect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  bool valid() const { return width > 0 && height > 0; }
};

} // namespace

struct SdlScreen::Impl {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  GlyphAtlas atlas;
  int scale = kInitialScale;
  int texture_width = 0;
  int texture_height = 0;
  bool closed = false;
  bool break_requested = false;
  std::deque<wchar_t> keys;
  std::deque<Screen::MouseClick> clicks;
  std::vector<std::uint32_t> frame;
  int frame_width = 0;
  int frame_height = 0;
  FrameRect graphics_rect;

  ~Impl() {
    if (texture != nullptr)
      SDL_DestroyTexture(texture);
    if (renderer != nullptr)
      SDL_DestroyRenderer(renderer);
    if (window != nullptr)
      SDL_DestroyWindow(window);
    SDL_Quit();
  }
};

namespace {

class SdlKeySource : public KeySource {
public:
  explicit SdlKeySource(SdlScreen* screen) : screen_(screen) {}
  bool WaitKey(wchar_t* key) override {
    while (!screen_->TryGetKey(key)) {
      if (screen_->Closed())
        return false;
      SDL_Delay(5);
    }
    return true;
  }

private:
  SdlScreen* screen_;
};

} // namespace

SdlScreen::SdlScreen() : impl_(std::make_unique<Impl>()) {}

SdlScreen::~SdlScreen() {
  if (impl_ != nullptr && impl_->window != nullptr)
    SDL_StopTextInput();
}

std::unique_ptr<WindowScreen> SdlScreen::Create(std::string* error, int scale) {
  std::unique_ptr<SdlScreen> screen(new SdlScreen());
  Impl* impl = screen->impl_.get();
  impl->scale = std::max(1, std::min(scale, 8));
  impl->atlas = RenderGlyphAtlas(8, 16);
  if (!impl->atlas.ok) {
    if (error != nullptr)
      *error = "could not rasterise the CP437 glyph atlas (no usable font)";
    return nullptr;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    if (error != nullptr)
      *error = std::string("SDL_Init failed: ") + SDL_GetError();
    return nullptr;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  const int width = screen->Width() * impl->atlas.glyph_width * impl->scale;
  const int height = screen->Height() * impl->atlas.glyph_height * impl->scale;
  impl->window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (impl->window == nullptr) {
    if (error != nullptr)
      *error = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
    return nullptr;
  }

  impl->renderer = SDL_CreateRenderer(impl->window, -1, SDL_RENDERER_ACCELERATED);
  if (impl->renderer == nullptr)
    impl->renderer = SDL_CreateRenderer(impl->window, -1, SDL_RENDERER_SOFTWARE);
  if (impl->renderer == nullptr) {
    if (error != nullptr)
      *error = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
    return nullptr;
  }
  SDL_RenderSetLogicalSize(impl->renderer, screen->Width() * impl->atlas.glyph_width,
                           screen->Height() * impl->atlas.glyph_height);
  SDL_StartTextInput();
  return screen;
}

void SdlScreen::OnGridChanged() {
  if (impl_ == nullptr || impl_->renderer == nullptr)
    return;
  if (impl_->texture != nullptr) {
    SDL_DestroyTexture(impl_->texture);
    impl_->texture = nullptr;
  }
  impl_->texture_width = 0;
  impl_->texture_height = 0;
  SDL_RenderSetLogicalSize(impl_->renderer, Width() * GlyphWidth(), Height() * GlyphHeight());
  SDL_SetWindowSize(impl_->window, Width() * GlyphWidth() * impl_->scale,
                    Height() * GlyphHeight() * impl_->scale);
}

void SdlScreen::Present() {
  if (impl_ == nullptr || impl_->renderer == nullptr)
    return;
  const int width = Width() * impl_->atlas.glyph_width;
  const int height = Height() * impl_->atlas.glyph_height;
  if (width <= 0 || height <= 0)
    return;

  impl_->frame.assign(static_cast<std::size_t>(width) * height, 0xFF000000u);
  impl_->frame_width = width;
  impl_->frame_height = height;
  impl_->graphics_rect = FrameRect{};

  const auto& palette = PaletteRgb();
  const Canvas& canvas = Graphics();
  const bool graphics = GraphicsMode() != kTextMode && canvas.Width() > 0 && canvas.Height() > 0;
  if (graphics) {
    const double aspect = DisplayAspect();
    int frame_width = width;
    int frame_height = static_cast<int>(static_cast<double>(width) / aspect);
    if (frame_height > height) {
      frame_height = height;
      frame_width = static_cast<int>(static_cast<double>(height) * aspect);
    }
    const int left = (width - frame_width) / 2;
    const int top = (height - frame_height) / 2;
    impl_->graphics_rect = FrameRect{left, top, frame_width, frame_height};
    for (int y = 0; y < frame_height; ++y) {
      const int sy =
          std::min(canvas.Height() - 1,
                   static_cast<int>(((2LL * y + 1) * canvas.Height()) / (2LL * frame_height)));
      for (int x = 0; x < frame_width; ++x) {
        const int sx =
            std::min(canvas.Width() - 1,
                     static_cast<int>(((2LL * x + 1) * canvas.Width()) / (2LL * frame_width)));
        const std::uint8_t index = canvas.Pixel(sx, sy);
        impl_->frame[static_cast<std::size_t>(top + y) * width + left + x] =
            Pack(palette[static_cast<std::size_t>(index)]);
      }
    }
  }

  auto put = [&](int x, int y, std::uint32_t value) {
    if (x < 0 || y < 0 || x >= width || y >= height)
      return;
    impl_->frame[static_cast<std::size_t>(y) * width + x] = value;
  };

  for (int row = 0; row < Height(); ++row) {
    for (int col = 0; col < Width(); ++col) {
      const wchar_t ch = CellAt(row, col);
      const std::uint8_t attr = CellAttrAt(row, col);
      const bool wide = IsTrueColourMode();
      const int fg_index = wide ? attr : (attr & 0x0F);
      const int bg_index = wide ? attr : ((attr >> 4) & 0x0F);
      std::array<std::uint8_t, 3> foreground = palette[static_cast<std::size_t>(fg_index)];
      std::array<std::uint8_t, 3> background = palette[static_cast<std::size_t>(bg_index)];
      const bool cursor_cell = CursorVisible() && row == CursorRow() && col == CursorCol();
      if (cursor_cell) {
        const std::array<std::uint8_t, 3> swap = foreground;
        foreground = background;
        background = swap;
      }

      const int cell_x = col * impl_->atlas.glyph_width;
      const int cell_y = row * impl_->atlas.glyph_height;
      if (!graphics || cursor_cell) {
        const std::uint32_t bg = Pack(background);
        for (int y = 0; y < impl_->atlas.glyph_height; ++y)
          for (int x = 0; x < impl_->atlas.glyph_width; ++x)
            put(cell_x + x, cell_y + y, bg);
      }
      if (ch == L' ' || !impl_->atlas.ok)
        continue;

      const int glyph = static_cast<int>(UnicodeToCp437(ch));
      const int glyph_x = (glyph % 16) * impl_->atlas.glyph_width;
      const int glyph_y = (glyph / 16) * impl_->atlas.glyph_height;
      for (int y = 0; y < impl_->atlas.glyph_height; ++y) {
        for (int x = 0; x < impl_->atlas.glyph_width; ++x) {
          const int coverage = impl_->atlas.At(glyph_x + x, glyph_y + y);
          if (coverage == 0)
            continue;
          const int px = cell_x + x;
          const int py = cell_y + y;
          if (px < 0 || py < 0 || px >= width || py >= height)
            continue;
          std::uint32_t& under = impl_->frame[static_cast<std::size_t>(py) * width + px];
          under = Blend(under, foreground, coverage);
        }
      }
    }
  }

  if (impl_->texture == nullptr || impl_->texture_width != width ||
      impl_->texture_height != height) {
    if (impl_->texture != nullptr)
      SDL_DestroyTexture(impl_->texture);
    impl_->texture = SDL_CreateTexture(impl_->renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, width, height);
    impl_->texture_width = width;
    impl_->texture_height = height;
  }
  if (impl_->texture == nullptr)
    return;
  SDL_UpdateTexture(impl_->texture, nullptr, impl_->frame.data(),
                    width * static_cast<int>(sizeof(std::uint32_t)));
  SDL_RenderClear(impl_->renderer);
  SDL_RenderCopy(impl_->renderer, impl_->texture, nullptr, nullptr);
  SDL_RenderPresent(impl_->renderer);
}

void SdlScreen::PumpMessages() {
  if (impl_ == nullptr)
    return;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      impl_->closed = true;
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        impl_->closed = true;
      break;
    case SDL_TEXTINPUT: {
      const std::wstring text = DecodeUtf8(event.text.text);
      for (wchar_t ch : text)
        impl_->keys.push_back(ch);
      break;
    }
    case SDL_KEYDOWN: {
      const SDL_Keycode key = event.key.keysym.sym;
      if ((event.key.keysym.mod & KMOD_CTRL) != 0 && key == SDLK_c) {
        impl_->break_requested = true;
        break;
      }
      if (key == SDLK_BACKSPACE)
        impl_->keys.push_back(0x08);
      else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        impl_->keys.push_back(L'\r');
      else if (key == SDLK_ESCAPE)
        impl_->keys.push_back(0x1B);
      else if (key == SDLK_TAB)
        impl_->keys.push_back(L'\t');
      // Printable keys arrive through SDL_TEXTINPUT, which is UTF-8 aware.
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      if (!impl_->graphics_rect.valid() || Graphics().Width() <= 0 || Graphics().Height() <= 0)
        break;
      int window_width = 1;
      int window_height = 1;
      SDL_GetWindowSize(impl_->window, &window_width, &window_height);
      const int logical_width = Width() * impl_->atlas.glyph_width;
      const int logical_height = Height() * impl_->atlas.glyph_height;
      const int x = event.button.x * logical_width / std::max(1, window_width);
      const int y = event.button.y * logical_height / std::max(1, window_height);
      const FrameRect& rect = impl_->graphics_rect;
      if (x < rect.x || y < rect.y || x >= rect.x + rect.width || y >= rect.y + rect.height)
        break;
      Screen::MouseClick click;
      click.x =
          std::clamp((x - rect.x) * Graphics().Width() / rect.width, 0, Graphics().Width() - 1);
      click.y =
          std::clamp((y - rect.y) * Graphics().Height() / rect.height, 0, Graphics().Height() - 1);
      click.button = event.button.button == SDL_BUTTON_RIGHT ? 2 : 1;
      impl_->clicks.push_back(click);
      break;
    }
    default:
      break;
    }
  }
}

std::wstring SdlScreen::ReadLine() {
  if (impl_ == nullptr || impl_->closed)
    return {};
  SdlKeySource keys(this);
  std::wstring line;
  if (!EditLine(keys, &line))
    return {};
  return line;
}

bool SdlScreen::TryGetKey(wchar_t* key) {
  if (impl_ == nullptr || key == nullptr)
    return false;
  PumpMessages();
  if (impl_->keys.empty())
    return false;
  *key = impl_->keys.front();
  impl_->keys.pop_front();
  return true;
}

bool SdlScreen::Closed() const {
  return impl_ != nullptr && impl_->closed;
}

bool SdlScreen::TakeBreak() {
  if (impl_ == nullptr || !impl_->break_requested)
    return false;
  impl_->break_requested = false;
  return true;
}

bool SdlScreen::TakeMouseClick(MouseClick* click) {
  if (impl_ == nullptr || impl_->clicks.empty() || click == nullptr)
    return false;
  *click = impl_->clicks.front();
  impl_->clicks.pop_front();
  return true;
}

void SdlScreen::InjectClick(int canvas_x, int canvas_y, int button) {
  if (impl_ == nullptr)
    return;
  MouseClick click;
  click.x = canvas_x;
  click.y = canvas_y;
  click.button = button;
  impl_->clicks.push_back(click);
}

void SdlScreen::InjectKeys(const std::wstring& keys) {
  if (impl_ == nullptr)
    return;
  for (wchar_t ch : keys)
    impl_->keys.push_back(ch);
}

bool SdlScreen::ReadBackCanvasPixel(int canvas_x, int canvas_y, std::uint8_t* rgba) {
  if (impl_ == nullptr || rgba == nullptr || impl_->frame.empty() || !impl_->graphics_rect.valid())
    return false;
  const Canvas& canvas = Graphics();
  if (canvas_x < 0 || canvas_y < 0 || canvas_x >= canvas.Width() || canvas_y >= canvas.Height())
    return false;
  const FrameRect& rect = impl_->graphics_rect;
  // Sample the centre of the source pixel's destination footprint. Mapping the
  // top-left corner can land one canvas pixel early when the scale is fractional,
  // which made a single plotted pixel invisible to --gpu-pixel.
  const int x =
      rect.x + static_cast<int>(((2LL * canvas_x + 1) * rect.width) / (2LL * canvas.Width()));
  const int y =
      rect.y + static_cast<int>(((2LL * canvas_y + 1) * rect.height) / (2LL * canvas.Height()));
  if (x < 0 || y < 0 || x >= impl_->frame_width || y >= impl_->frame_height)
    return false;
  const std::uint32_t pixel = impl_->frame[static_cast<std::size_t>(y) * impl_->frame_width + x];
  rgba[0] = static_cast<std::uint8_t>((pixel >> 16) & 0xFF);
  rgba[1] = static_cast<std::uint8_t>((pixel >> 8) & 0xFF);
  rgba[2] = static_cast<std::uint8_t>(pixel & 0xFF);
  rgba[3] = 255;
  return true;
}

int SdlScreen::GlyphWidth() const {
  return impl_ != nullptr && impl_->atlas.glyph_width > 0 ? impl_->atlas.glyph_width : 8;
}

int SdlScreen::GlyphHeight() const {
  return impl_ != nullptr && impl_->atlas.glyph_height > 0 ? impl_->atlas.glyph_height : 16;
}

} // namespace lwbasic::screen
