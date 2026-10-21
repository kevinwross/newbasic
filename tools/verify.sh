#!/usr/bin/env bash
# Build and test newbasic on Linux (or any POSIX host).
#
# Usage:
#   ./tools/verify.sh                 # configure, build, run CTest
#   ./tools/verify.sh --no-sdl        # dependency-free console-only build
#   BUILD_DIR=out ./tools/verify.sh   # choose the build directory
#
# Extra CMake arguments can be passed after `--`.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-${root}/build}"
config="${CONFIG:-Release}"
sdl="ON"
extra=()

while (($# > 0)); do
  case "$1" in
    --no-sdl) sdl="OFF"; shift ;;
    --build-dir) build_dir="$2"; shift 2 ;;
    --config) config="$2"; shift 2 ;;
    --) shift; extra+=("$@"); break ;;
    *) extra+=("$1"); shift ;;
  esac
done

echo "== configure ($config, SDL=$sdl) =="
cmake -S "$root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE="$config" \
  -DNEWBASIC_WINDOW_SDL="$sdl" \
  "${extra[@]}"

echo "== build =="
cmake --build "$build_dir" --parallel

echo "== test =="
ctest --test-dir "$build_dir" --timeout 180 --output-on-failure

echo "PASS"
