add_library(lwbasic-screen STATIC
  src/console_screen.cpp
  src/headless_screen.cpp
  src/glyph_atlas.cpp
  src/d3d11_screen.cpp
)
target_include_directories(lwbasic-screen PUBLIC include)
target_link_libraries(lwbasic-screen PUBLIC lwbasic-core)
if(MSVC)
  target_compile_options(lwbasic-screen PRIVATE /WX)
  # The windowed backend needs D3D11, DXGI and the shader compiler from the
  # Windows SDK.
  target_link_libraries(lwbasic-screen PRIVATE d3d11 dxgi d3dcompiler)
endif()
