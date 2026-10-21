# newbasic

A dual-platform fork of **lwbasic**, a terminal interpreter for the **GW-BASIC
3.23 dialect**, written in C++17 and buildable on Windows and Linux.

Program files load and run, the direct-mode REPL accepts statements, and an
in-process debugger is exposed through a Debug Adapter Protocol server so an
editor can drive it.

This repository was seeded from
[kevinwross/lwbasic](https://github.com/kevinwross/lwbasic) at commit
`925fde3`; see `UPSTREAM.md`. The dialect specification and upstream
implementation plan are retained as `docs/newbasic-spec-plan.md`. The
porting plan is `plan.md`.

## Status

| Milestone | State |
|---|---|
| M0 — skeleton, `Screen` abstraction, CI | ✅ |
| M1 — lexer, expressions, control flow, REPL | ✅ |
| M2 — types, arrays, `DEF FN`, `DATA`/`READ`/`RESTORE` | ✅ |
| M3 — error handling, program files, `SHELL`, sequential file I/O, DOS commands, timer events | ✅ |
| M4 — tokenized `.BAS` read/write incl. protected files | ✅ (byte-fidelity to Microsoft files unproven) |
| M5 — debugger library + `newbasic-dap` | ✅ |
| M6 — golden corpus, spec-driven conformance | ✅ |
| M7 — D3D11 window backend, sound, time-travel debugging | ✅ on Windows |
| Linux port — console, POSIX host, stb glyph atlas, SDL2 window, DAP TCP | ✅ |

All 365 upstream test cases pass on Linux, plus one portability regression case
(366 total, 31,124 assertions). The Windows D3D11 backend is retained; Linux
uses SDL2 when it is available and falls back to a terminal-only build
otherwise. The Windows targets are cross-built in CI and were compiled here
with LLVM-MinGW, but the MSVC job is the authoritative Windows check.

## Build

### Windows

Requires Visual Studio (C++ workload) and the Windows 11 SDK. CMake comes with
Visual Studio.

```
cmake -S . -B out -A x64
cmake --build out --config Release --parallel
```

### Linux

Requires CMake 3.20+, a C++17 compiler, and (for the windowed backend) the SDL2
development package. Debian/Ubuntu:

```
sudo apt-get install cmake g++ libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

For a dependency-free console-only build:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNEWBASIC_WINDOW_SDL=OFF
cmake --build build --parallel
```

The Linux glyph atlas uses the bundled DejaVu Sans Mono font in
`third_party/fonts/`. Set `NEWBASIC_FONT` to override it.

## Test

```
ctest --test-dir build --timeout 180 --output-on-failure
```

or, on either platform, the hygiene wrapper:

```
pwsh tools/verify.ps1              # Windows: build, test, retry once
./tools/verify.sh                  # Linux: build and test
```

Some checks are deliberately **not** part of the default run because they make
sound or need a GPU/display. Run them by hand:

```
build/tests/newbasic-tests "[.audio]" -s      # real host audio timing
build/tests/newbasic-tests "[.gpu]" -s        # window backend
SDL_VIDEODRIVER=dummy build/tests/newbasic-tests "[.gpu]"  # headless SDL smoke
```

## Run

```
newbasic prog.bas          # run a program in a window when a backend is built
newbasic                   # direct-mode REPL in a window
newbasic --cli prog.bas    # run in the terminal instead
newbasic --gpu-scale 2     # bigger window (16x32 cells instead of 8x16)
newbasic samples/NeonHorizon.bas   # SCREEN 720 demos: synthwave sunset
newbasic samples/RingWorld.bas     #   ringed gas giant with three palettes
newbasic samples/Fireworks.bas     #   particle fireworks, click to burst
newbasic samples/Spirograph.bas    #   layered rosettes with a rotating hue wheel
```

On Linux, if SDL2 is not available or no display is present, `newbasic` falls
back to the terminal. In a console-only build, use `SCREEN` graphics with
`--gpu-shot FILE` to capture a PPM without a window.

## Layout

```
src/newbasic-core     VM, front end, values, screen grid, debugger, DAP protocol,
                     portable host services (clock, shell, printer, sound)
src/newbasic-screen   console / headless backends, glyph atlas
src/newbasic-window   D3D11 (Windows) and SDL2 (Linux) window backends + factory
src/newbasic-cli      newbasic.exe / newbasic
src/newbasic-dap      newbasic-dap.exe / newbasic-dap (DAP server over stdio or TCP)
tests                unit, property, conformance and integration tests
fixtures             *.bas + *.expected pairs run by the golden-corpus test
docs                 format notes, design notes, status snapshot, portability notes
third_party          vendored Catch2, RapidCheck, stb_truetype and DejaVu font
```

## Platform notes

See `docs/portability.md` for the host mapping and documented divergences:
`SHELL` uses `cmd.exe` on Windows and `$SHELL` on POSIX; DOS path separators are
translated on POSIX; audio is silent-but-correctly-timed unless a platform audio
backend is added; and the window backends are not pixel-identical.

## Notes

- **Reference material.** GW-BASIC is MIT-licensed following Microsoft's
  revision; `GWBASIC.txt` and `docs/gw-basic/` (the chunked User's Guide and
  Reference) are therefore committed and serve as the conformance source.
- **Format facts were sourced, not guessed** — token values, the MBF numeric
  encoding, the protected-file transform and the tokenized layout all record
  their provenance under `docs/`.
- **Arithmetic uses host floating point** by project decision; only `PRINT`
  display formatting emulates GW-BASIC (7 significant digits for single
  precision, exponent boundaries, cramming at column 80).
