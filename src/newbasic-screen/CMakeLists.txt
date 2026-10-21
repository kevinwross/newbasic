add_library(newbasic-screen STATIC
  src/headless_screen.cpp
  src/glyph_atlas.cpp
)
if(WIN32)
  target_sources(newbasic-screen PRIVATE
    src/console_screen_win32.cpp
    src/glyph_atlas_gdi.cpp
  )
else()
  target_sources(newbasic-screen PRIVATE
    src/console_screen_posix.cpp
    src/glyph_atlas_stb.cpp
  )
endif()
target_include_directories(newbasic-screen PUBLIC include)
target_include_directories(newbasic-screen PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/third_party"
)
target_compile_definitions(newbasic-screen PRIVATE
  NEWBASIC_FONT_PATH="${CMAKE_SOURCE_DIR}/third_party/fonts/DejaVuSansMono.ttf"
)
target_link_libraries(newbasic-screen PUBLIC newbasic-core)
newbasic_target_warnings(newbasic-screen)
