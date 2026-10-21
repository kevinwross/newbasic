add_library(lwbasic-core STATIC
  src/value.cpp
  src/number_format.cpp
  src/using.cpp
  src/program_files.cpp
  src/renum.cpp
  src/draw.cpp
  src/errors.cpp
  src/lexer.cpp
  src/host.cpp
  src/line_store.cpp
  src/screen.cpp
  src/frontend.cpp
  src/vm.cpp
  src/repl.cpp
)
target_include_directories(lwbasic-core PUBLIC include)
target_link_libraries(lwbasic-core PUBLIC winmm)
if(MSVC)
  target_compile_options(lwbasic-core PRIVATE /WX)
endif()
