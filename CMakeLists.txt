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
  set(_newbasic_d3d11_default ON)
  set(_newbasic_sdl_default OFF)
else()
  set(_newbasic_d3d11_default OFF)
  set(_newbasic_sdl_default AUTO)
endif()
option(NEWBASIC_WINDOW_D3D11 "Build the Win32/D3D11 window backend" ${_newbasic_d3d11_default})
set(NEWBASIC_WINDOW_SDL "${_newbasic_sdl_default}" CACHE STRING
    "Build the SDL2 window backend: AUTO, ON or OFF")
option(NEWBASIC_BUILD_TESTS "Build the test suite" ON)
option(NEWBASIC_WERROR "Treat first-party warnings as errors" OFF)

if(WIN32)
  set(NEWBASIC_PLATFORM_WINDOWS ON)
else()
  set(NEWBASIC_PLATFORM_POSIX ON)
endif()

# SDL2 is optional: a console-only build is fully supported.
set(NEWBASIC_HAVE_SDL OFF)
if(NOT NEWBASIC_WINDOW_SDL STREQUAL "OFF")
  find_package(SDL2 CONFIG QUIET)
  if(SDL2_FOUND)
    if(TARGET SDL2::SDL2)
      set(SDL2_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
      set(SDL2_TARGET SDL2::SDL2-static)
    endif()
    set(NEWBASIC_HAVE_SDL ON)
  else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
      if(SDL2_FOUND)
        set(SDL2_TARGET PkgConfig::SDL2)
        set(NEWBASIC_HAVE_SDL ON)
      endif()
    endif()
  endif()
  if(NEWBASIC_WINDOW_SDL STREQUAL "ON" AND NOT NEWBASIC_HAVE_SDL)
    message(FATAL_ERROR
      "NEWBASIC_WINDOW_SDL=ON but SDL2 was not found. Install the SDL2 "
      "development package (libsdl2-dev on Debian/Ubuntu) or configure "
      "with -DNEWBASIC_WINDOW_SDL=OFF.")
  endif()
  if(NOT NEWBASIC_HAVE_SDL)
    message(STATUS "SDL2 not found; building without a window backend")
  endif()
endif()

# Apply warnings-as-errors only to first-party targets. Vendored Catch2 and
# RapidCheck keep their own warning posture.
function(newbasic_target_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /WX)
  elseif(NEWBASIC_WERROR)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

# Keep the user-facing executables in one predictable place. The generator
# expression makes multi-config generators produce bin/Release and bin/Debug
# directly instead of appending another configuration directory. Debug symbols
# stay with the rest of the build output; only the two executables belong in bin.
function(newbasic_set_binary_output target)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb/$<IF:$<CONFIG:Debug>,Debug,Release>")
endfunction()

add_subdirectory(third_party/catch2)
add_subdirectory(src/newbasic-core)
add_subdirectory(src/newbasic-debug)
add_subdirectory(src/newbasic-screen)
add_subdirectory(src/newbasic-window)
add_subdirectory(src/newbasic-cli)
add_subdirectory(src/newbasic-dap)

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

if(NEWBASIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
