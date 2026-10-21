#pragma once

#include <memory>
#include <string>

#include "lwbasic/window/window_screen.h"

namespace lwbasic::screen {

// Returns nullptr and fills `error` when no window/device is available (or when
// this build has no window backend). The caller falls back to the console.
std::unique_ptr<WindowScreen> CreateWindowScreen(std::string* error, int scale = 1);

// True when a window backend was compiled in.
bool WindowBackendAvailable();

// Diagnostic name: "D3D11", "SDL2", or "" when no backend is available.
const char* WindowBackendName();

} // namespace lwbasic::screen
