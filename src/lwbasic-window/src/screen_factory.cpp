#include "lwbasic/window/screen_factory.h"

#if defined(LWBASIC_WINDOW_D3D11)
#include "lwbasic/window/d3d11_screen.h"
#elif defined(LWBASIC_WINDOW_SDL)
#include "lwbasic/window/sdl_screen.h"
#endif

namespace lwbasic::screen {
namespace {

#if defined(LWBASIC_WINDOW_D3D11)
constexpr const char* kBackendName = "D3D11";
#elif defined(LWBASIC_WINDOW_SDL)
constexpr const char* kBackendName = "SDL2";
#else
constexpr const char* kBackendName = "";
#endif

} // namespace

std::unique_ptr<WindowScreen> CreateWindowScreen(std::string* error, int scale) {
#if defined(LWBASIC_WINDOW_D3D11)
  return D3D11Screen::Create(error, scale);
#elif defined(LWBASIC_WINDOW_SDL)
  return SdlScreen::Create(error, scale);
#else
  (void)scale;
  if (error != nullptr)
    *error = "no window backend in this build";
  return nullptr;
#endif
}

bool WindowBackendAvailable() { return kBackendName[0] != '\0'; }

const char* WindowBackendName() { return kBackendName; }

} // namespace lwbasic::screen
