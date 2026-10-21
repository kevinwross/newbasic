cmake_minimum_required(VERSION 3.20)
project(newbasic VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # /W4 globally; /WX (warnings-as-errors) is applied per first-party target
  # and to tests, not to vendored third-party code.
  add_compile_options(/W4 /permissive- /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Window backend selection. D3D11 is the known-good Windows backend; SDL2 is
# the Linux backend. AUTO means "use it when the dependency is available".
if(WIN32)
  set(_lwbasic_d3d11_default ON)
  set(_lwbasic_sdl_default OFF)
else()
  set(_lwbasic_d3d11_default OFF)
  set(_lwbasic_sdl_default AUTO)
endif()
option(LWBASIC_WINDOW_D3D11 "Build the Win32/D3D11 window backend" ${_lwbasic_d3d11_default})
set(LWBASIC_WINDOW_SDL "${_lwbasic_sdl_default}" CACHE STRING
    "Build the SDL2 window backend: AUTO, ON or OFF")
option(LWBASIC_BUILD_TESTS "Build the test suite" ON)
option(LWBASIC_WERROR "Treat first-party warnings as errors" OFF)

if(WIN32)
  set(LWBASIC_PLATFORM_WINDOWS ON)
else()
  set(LWBASIC_PLATFORM_POSIX ON)
endif()

# SDL2 is optional: a console-only build is fully supported.
set(LWBASIC_HAVE_SDL OFF)
if(NOT LWBASIC_WINDOW_SDL STREQUAL "OFF")
  find_package(SDL2 CONFIG QUIET)
  if(SDL2_FOUND)
    if(TARGET SDL2::SDL2)
      set(SDL2_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
      set(SDL2_TARGET SDL2::SDL2-static)
    endif()
    set(LWBASIC_HAVE_SDL ON)
  else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
      if(SDL2_FOUND)
        set(SDL2_TARGET PkgConfig::SDL2)
        set(LWBASIC_HAVE_SDL ON)
      endif()
    endif()
  endif()
  if(LWBASIC_WINDOW_SDL STREQUAL "ON" AND NOT LWBASIC_HAVE_SDL)
    message(FATAL_ERROR
      "LWBASIC_WINDOW_SDL=ON but SDL2 was not found. Install the SDL2 "
      "development package (libsdl2-dev on Debian/Ubuntu) or configure "
      "with -DLWBASIC_WINDOW_SDL=OFF.")
  endif()
  if(NOT LWBASIC_HAVE_SDL)
    message(STATUS "SDL2 not found; building without a window backend")
  endif()
endif()

# Apply warnings-as-errors only to first-party targets. Vendored Catch2 and
# RapidCheck keep their own warning posture.
function(lwbasic_target_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /WX)
  elseif(LWBASIC_WERROR)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

add_subdirectory(third_party/catch2)
add_subdirectory(src/lwbasic-core)
add_subdirectory(src/lwbasic-debug)
add_subdirectory(src/lwbasic-screen)
add_subdirectory(src/lwbasic-window)
add_subdirectory(src/lwbasic-cli)
add_subdirectory(src/lwbasic-dap)

# RapidCheck is vendored as sources, built with our own target instead of
# upstream's CMake (which forces /WX globally under MSVC and pulls in
# extras). Header usage: link rapidcheck-vendored, include <rapidcheck/...>.
file(GLOB_RECURSE RC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/src/*.cpp")
add_library(rapidcheck-vendored STATIC ${RC_SOURCES})
target_include_directories(rapidcheck-vendored SYSTEM PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/include"
  # Catch2 integration header (RC_CHECK/RC_ASSERT/RC_GEN): extras/catch is
  # header-only and lives outside the main include tree.
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidcheck/extras/catch/include")
if(MSVC)
  target_compile_options(rapidcheck-vendored PRIVATE
    /bigobj /wd4503 /wd4267 /wd4244 /wd4100 /wd4365)
endif()

if(LWBASIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
