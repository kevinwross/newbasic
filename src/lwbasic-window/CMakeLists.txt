add_library(lwbasic-window STATIC
  src/screen_factory.cpp
)
target_include_directories(lwbasic-window PUBLIC include)
target_link_libraries(lwbasic-window PUBLIC lwbasic-screen)

if(LWBASIC_WINDOW_D3D11)
  target_sources(lwbasic-window PRIVATE src/d3d11_screen.cpp)
  target_compile_definitions(lwbasic-window PUBLIC LWBASIC_WINDOW_D3D11=1)
  target_link_libraries(lwbasic-window PRIVATE d3d11 dxgi d3dcompiler)
endif()

if(LWBASIC_HAVE_SDL)
  target_sources(lwbasic-window PRIVATE src/sdl_screen.cpp)
  target_compile_definitions(lwbasic-window PUBLIC LWBASIC_WINDOW_SDL=1)
  target_link_libraries(lwbasic-window PRIVATE ${SDL2_TARGET})
endif()

lwbasic_target_warnings(lwbasic-window)
