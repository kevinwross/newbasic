#include "newbasic/window/screen_factory.h"

#if defined(NEWBASIC_WINDOW_D3D11)
#include "newbasic/window/d3d11_screen.h"
#elif defined(NEWBASIC_WINDOW_SDL)
#include "newbasic/window/sdl_screen.h"
#endif

namespace newbasic::screen {
namespace {

#if defined(NEWBASIC_WINDOW_D3D11)
constexpr const char* kBackendName = "D3D11";
#elif defined(NEWBASIC_WINDOW_SDL)
constexpr const char* kBackendName = "SDL2";
#else
constexpr const char* kBackendName = "";
#endif

} // namespace

std::unique_ptr<WindowScreen> CreateWindowScreen(std::string* error, int scale) {
#if defined(NEWBASIC_WINDOW_D3D11)
  return D3D11Screen::Create(error, scale);
#elif defined(NEWBASIC_WINDOW_SDL)
  return SdlScreen::Create(error, scale);
#else
  (void)scale;
  if (error != nullptr)
    *error = "no window backend in this build";
  return nullptr;
#endif
}

bool WindowBackendAvailable() {
  return kBackendName[0] != '\0';
}

const char* WindowBackendName() {
  return kBackendName;
}

} // namespace newbasic::screen
