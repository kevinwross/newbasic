add_library(newbasic-window STATIC
  src/screen_factory.cpp
)
target_include_directories(newbasic-window PUBLIC include)
target_link_libraries(newbasic-window PUBLIC newbasic-screen)

if(NEWBASIC_WINDOW_D3D11)
  target_sources(newbasic-window PRIVATE src/d3d11_screen.cpp)
  target_compile_definitions(newbasic-window PUBLIC NEWBASIC_WINDOW_D3D11=1)
  target_link_libraries(newbasic-window PRIVATE d3d11 dxgi d3dcompiler)
endif()

if(NEWBASIC_HAVE_SDL)
  target_sources(newbasic-window PRIVATE src/sdl_screen.cpp)
  target_compile_definitions(newbasic-window PUBLIC NEWBASIC_WINDOW_SDL=1)
  target_link_libraries(newbasic-window PRIVATE ${SDL2_TARGET})
endif()

newbasic_target_warnings(newbasic-window)
