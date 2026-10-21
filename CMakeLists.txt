cmake_minimum_required(VERSION 3.20)
project(lwbasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # /W4 globally; /WX (warnings-as-errors) is applied per lwbasic_* target
  # and to tests, not to vendored third-party code (plan section 6).
  add_compile_options(/W4 /permissive- /utf-8)
endif()

add_subdirectory(third_party/catch2)
add_subdirectory(src/lwbasic-core)
add_subdirectory(src/lwbasic-debug)
add_subdirectory(src/lwbasic-screen)
add_subdirectory(src/lwbasic-cli)
add_subdirectory(src/lwbasic-dap)

# RapidCheck is vendored as sources, built with our own target instead of
# upstream's CMake (which forces /WX globally under MSVC and pulls in
# extras). Header usage: link rapidcheck-vendored, include <rapidcheck/...>.
file(GLOB_RECURSE RC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/src/*.cpp")
add_library(rapidcheck-vendored STATIC ${RC_SOURCES})
target_include_directories(rapidcheck-vendored PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/include"
  # Catch2 integration header (RC_CHECK/RC_ASSERT/RC_GEN): extras/catch is
  # header-only and lives outside the main include tree.
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/extras/catch/include")
if(MSVC)
  target_compile_options(rapidcheck-vendored PRIVATE
    /bigobj /wd4503 /wd4267 /wd4244 /wd4100 /wd4365)
endif()

enable_testing()
add_subdirectory(tests)
