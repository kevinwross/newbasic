add_library(newbasic-core STATIC
  src/value.cpp
  src/number_format.cpp
  src/using.cpp
  src/program_files.cpp
  src/renum.cpp
  src/draw.cpp
  src/errors.cpp
  src/lexer.cpp
  src/host.cpp
  src/host_path.cpp
  src/line_store.cpp
  src/screen.cpp
  src/frontend.cpp
  src/vm.cpp
  src/repl.cpp
)
if(WIN32)
  target_sources(newbasic-core PRIVATE src/host_win32.cpp)
  target_link_libraries(newbasic-core PUBLIC winmm)
else()
  target_sources(newbasic-core PRIVATE src/host_posix.cpp)
  target_link_libraries(newbasic-core PUBLIC m)
endif()
# std::thread is used by the DAP session's pause reader. MSVC supplies it
# without extra libraries; MinGW and POSIX need the Threads target.
find_package(Threads REQUIRED)
target_link_libraries(newbasic-core PUBLIC Threads::Threads)
target_include_directories(newbasic-core PUBLIC include)
newbasic_target_warnings(newbasic-core)
