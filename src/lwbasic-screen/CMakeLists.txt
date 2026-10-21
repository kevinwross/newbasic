add_library(lwbasic-screen STATIC
  src/headless_screen.cpp
  src/glyph_atlas.cpp
)
if(WIN32)
  target_sources(lwbasic-screen PRIVATE
    src/console_screen_win32.cpp
    src/glyph_atlas_gdi.cpp
  )
else()
  target_sources(lwbasic-screen PRIVATE
    src/console_screen_posix.cpp
    src/glyph_atlas_stb.cpp
  )
endif()
target_include_directories(lwbasic-screen PUBLIC include)
target_include_directories(lwbasic-screen PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/third_party"
)
target_compile_definitions(lwbasic-screen PRIVATE
  LWBASIC_FONT_PATH="${CMAKE_SOURCE_DIR}/third_party/fonts/DejaVuSansMono.ttf"
)
target_link_libraries(lwbasic-screen PUBLIC lwbasic-core)
if(MSVC)
  target_compile_options(lwbasic-screen PRIVATE /WX)
endif()
