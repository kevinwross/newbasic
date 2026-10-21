# newbasic — dual-platform lwbasic port plan

**Goal:** turn the Windows-only `lwbasic` interpreter into a project in
`~/repos/newbasic` that configures, builds, tests and runs on both **Windows
(MSVC)** and **Linux (GCC/Clang)**, with the GW-BASIC behaviour and the golden
fixtures unchanged.

**Approach:** one source tree with narrow platform seams. Do **not** fork the
interpreter into "lwbasic-win" and "lwbasic-linux". The VM, lexer, front end,
FSTOK codec, debugger and DAP protocol are already portable; only the host
services, console, font rasteriser, window backend and DAP socket are
Windows-bound.

**Recommended scope:** console support first (gets the interpreter running on
Linux quickly), then the SDL2 windowed backend for graphics parity, then CI and
packaging. Keep the Windows D3D11 backend as-is; add SDL2 only for Linux in v1.

## At a glance

| Phase | Outcome | Main risk |
|---|---|---|
| 0 | Seed `newbasic`, protect fixture bytes, record provenance | Git EOL conversion |
| 1 | CMake configures on Windows and Linux; screen/window libs split | SDL2 dependency discovery |
| 2 | Portable clock, shell, printer, sound; `SHELL` suspension hooks | Terminal/child-process interaction |
| 3 | POSIX termios/ANSI console; Linux REPL usable | Raw-mode restore, key decoding |
| 4 | Portable glyph atlas (GDI on Windows, stb on Linux) | Font quality/licensing |
| 5 | `WindowScreen` factory; SDL2 window on Linux | Input/readback parity |
| 6 | POSIX DAP TCP transport | Socket lifetime, `SIGPIPE` |
| 7 | DOS path/semantics shims on Linux | Case sensitivity, `NAME`/`RMDIR` |
| 8 | CI matrix, tests, docs, packaging | CI graphics jobs |

Critical path: **0 → 1 → 2 → 3** gets a working Linux interpreter; **4 → 5**
adds graphics; **6 → 8** makes it maintainable and releasable.

### Implementation status (2026-10-21)

Phases 0–8 are implemented in this repository (optional ALSA output and
packaging rules remain future work):

- Platform-aware CMake with separate `lwbasic-screen` and `lwbasic-window`
  targets, and a console-only configuration that needs no window library.
- Portable host split (`host.cpp`, `host_win32.cpp`, `host_posix.cpp`), POSIX
  `SHELL`, timed silent tone output and printer/environment handling.
- Win32 console retained; POSIX termios/ANSI console with break handling,
  resize handling, UTF-8 output and `SHELL` suspend/resume.
- GDI atlas on Windows; `stb_truetype` + bundled DejaVu Sans Mono on Linux.
- `WindowScreen` factory; D3D11 retained on Windows; SDL2 backend on Linux.
- POSIX DAP socket transport; portable CLI (`sscanf`, factory, generic text).
- DOS path translation (`\` -> `/` on POSIX), deterministic `FILES` ordering
  and DOS-compatible `NAME`/`KILL`/`RMDIR` type checks are implemented.

Verification on this Linux host: the full upstream suite passes (365 test
cases, 31,110 assertions), the hidden `[.audio]` timing test passes, and the
hidden `[.gpu]` test passes with SDL2 under `SDL_VIDEODRIVER=dummy`.
`LWBASIC_WERROR=ON` builds the first-party targets cleanly. The Windows side
was cross-compiled and linked with LLVM-MinGW (D3D11 backend, `lwbasic.exe`,
`lwbasic-dap.exe` and `lwbasic-tests.exe`); the MSVC CI job still needs to run
on Windows itself.

---

## 1. What the Linux compile probe already showed

A probe against upstream commit `925fde3` with GCC 15.2 on Linux:

| Area | Result |
|---|---|
| All 13 `lwbasic-core` `.cpp` files except `host.cpp` | compile clean |
| `src/lwbasic-core/src/host.cpp` | fails: `#include <windows.h>`; uses `localtime_s`, `CreateProcessW`, `waveOut`, `Beep`, `Sleep`, `_dupenv_s` |
| `src/lwbasic-screen/src/headless_screen.cpp` | compiles clean |
| `src/lwbasic-cli/main.cpp` | fails: `sscanf_s` is MSVC-only (also includes `d3d11_screen.h` and casts to `D3D11Screen`) |
| `src/lwbasic-dap/main.cpp` | fails: `#include <winsock2.h>` |
| `src/lwbasic-screen/src/console_screen.cpp` | fails: Win32 console API |
| `src/lwbasic-screen/src/glyph_atlas.cpp` | fails: GDI (`CreateDIBSection`, `CreateFontW`, `TextOutW`, …) |
| `src/lwbasic-screen/src/d3d11_screen.cpp` | Windows/D3D11 by definition |

So the port is roughly **one core file, the three screen backends, one CLI
file, one DAP file and the build system**. That is a good sign: the
interpreter proper does not need a rewrite.

### Complete Windows-bound inventory

| File | Windows dependency | Replacement / treatment |
|---|---|---|
| `CMakeLists.txt` | MSVC-only flags | compiler-specific blocks; platform subdirectories |
| `src/lwbasic-core/CMakeLists.txt` | links `winmm` | `winmm` on Windows only; `Threads::Threads` and `m` on POSIX |
| `src/lwbasic-screen/CMakeLists.txt` | always builds `d3d11_screen.cpp`; D3D libs under `if(MSVC)` | split into `lwbasic-screen` and `lwbasic-window`; select sources/libraries by platform |
| `src/lwbasic-dap/CMakeLists.txt` | links `ws2_32` | `ws2_32` on Windows only |
| `src/lwbasic-core/src/host.cpp` | `windows.h`, time, shell, sound, environment | split into portable + Win32 + POSIX units |
| `src/lwbasic-screen/include/lwbasic/screen/console_screen.h` | opaque `HANDLE`, Win32 state | pimpl; platform-neutral header |
| `src/lwbasic-screen/src/console_screen.cpp` | Win32 console API | keep as `console_screen_win32.cpp`; add `console_screen_posix.cpp` |
| `src/lwbasic-screen/src/glyph_atlas.cpp` | GDI font rasteriser | extract common glyph work; GDI on Windows, `stb_truetype` on Linux |
| `src/lwbasic-screen/src/d3d11_screen.cpp` | Win32 + D3D11 | Windows-only; add `sdl_screen.cpp` for Linux |
| `src/lwbasic-screen/include/lwbasic/screen/d3d11_screen.h` | D3D-specific public type used by CLI/tests | introduce `WindowScreen` interface + factory |
| `src/lwbasic-cli/main.cpp` | `sscanf_s`, `D3D11Screen` casts, Win32 wording | portable parsing, factory, generic messages |
| `src/lwbasic-dap/main.cpp` | Winsock types and lifecycle | portable socket wrapper (`socket_win32.cpp` / `socket_posix.cpp`) |
| `tests/test_text_render.cpp` | includes `d3d11_screen.h`, `[.gpu]` test | use factory; skip cleanly when no window backend |
| `tests/test_sound.cpp` | `SystemHost` real audio check | no source change; POSIX host must block for the tone duration |
| `tools/verify.ps1` | PowerShell/Windows paths | add `tools/verify.sh` |
| `.github/workflows/ci.yml` | `windows-latest` only | OS/compiler matrix |
| `README.md`, `docs/status.md`, `plan.md` | Windows-only instructions/claims | update with a platform matrix and divergences |

Four non-obvious behavioural hotspots found in the core:

- `src/lwbasic-core/src/repl.cpp` implements `NAME old AS new` with
  `std::filesystem::rename`. POSIX `rename` **silently overwrites** an
  existing destination, which DOS `NAME` must not do. Add an explicit
  no-replace shim and a test for the existing-destination case; confirm the
  current Windows behaviour at the same time instead of assuming it.
- `KILL` and `RMDIR` both use `std::filesystem::remove`, which removes either a
  file or an empty directory on both platforms. DOS `KILL` should reject a
  directory and DOS `RMDIR` should reject a file; add explicit type checks so
  the commands keep their documented semantics on both platforms.
- `FILES` iterates a directory in unspecified order. Windows and Linux will
  disagree unless the list is sorted.
- `SHELL` runs while the console may be in raw/no-echo alternate-screen mode.
  On POSIX the child shell needs a cooked terminal, so the screen needs
  suspend/resume hooks around the child process.

---

## 2. Principles

1. **Keep the dialect and the byte formats identical.** No change to lexer,
   VM, tokenized `.BAS` codec, protected files, number formatting or golden
   expectations. Platform code sits behind `Host`, `Screen`, the window
   factory and the socket wrapper.
2. **Keep upstream names.** Project name can be `newbasic`, but keep the
   `lwbasic::` namespace, target names and binary names (`lwbasic`,
   `lwbasic-dap`, `lwbasic-tests`) so upstream fixes can be merged with a
   small diff and existing docs/scripts keep working.
3. **Platform selection belongs in CMake, not in `#ifdef` forests.** Prefer
   separate translation units (`*_win32.cpp`, `*_posix.cpp`) over preprocessor
   branches inside shared logic.
4. **Degrade, do not abort.** If SDL2, a font, a GPU or an audio device is
   unavailable, fall back to the console / built-in data / silent timed tone,
   with one clear message.
5. **Everything testable stays testable.** The console and window backends
   should remain thin; all grid, palette, atlas geometry and composition logic
   stays in platform-neutral code.
6. **Pin line endings and bytes.** Add `.gitattributes` before copying
   fixtures, or Git on Windows can rewrite CRLF/CP437 fixture files.

---

## 3. Target layout

```
newbasic/
  CMakeLists.txt
  CMakePresets.json
  .gitattributes
  README.md
  plan.md                        # this document
  docs/
    portability.md               # behaviour matrix + divergences
    ...                          # upstream docs retained
  src/
    lwbasic-core/
      include/lwbasic/...        # unchanged public API
      src/
        host.cpp                 # portable: clock/date/time/printer
        host_win32.cpp           # SHELL + PlayTone
        host_posix.cpp           # SHELL + PlayTone
        host_path.cpp            # DOS path -> host path helper
        ...                      # unchanged
    lwbasic-screen/                # console + headless + atlas (no window libs)
      include/lwbasic/screen/
        console_screen.h         # pimpl, platform-neutral
        glyph_atlas.h            # unchanged interface
        ...
      src/
        console_screen_win32.cpp
        console_screen_posix.cpp
        glyph_atlas.cpp          # shared orchestration + procedural glyphs
        glyph_atlas_gdi.cpp      # Windows
        glyph_atlas_stb.cpp      # Linux (stb_truetype + bundled font)
        headless_screen.cpp
        text_render.cpp
    lwbasic-window/                # always built; may contain zero backends
      include/lwbasic/window/
        window_screen.h          # new: windowed-backend interface
        screen_factory.h         # new: CreateWindowScreen()
        d3d11_screen.h           # moved from lwbasic-screen
        sdl_screen.h             # new
      src/
        d3d11_screen.cpp         # Windows only
        sdl_screen.cpp           # Linux (optionally Windows)
        screen_factory.cpp
    lwbasic-cli/main.cpp
    lwbasic-dap/
      main.cpp
      socket_win32.cpp
      socket_posix.cpp
      socket.h
  third_party/
    stb/stb_truetype.h           # public-domain/MIT single header
    fonts/DejaVuSansMono.ttf     # permissive licence + LICENSE file
  tools/
    verify.ps1
    verify.sh                    # new
  tests/                         # unchanged, plus portability fixes
```

A possible later simplification is to use SDL2 on Windows as well and retire
D3D11, or to make D3D11 and SDL2 selectable at runtime. **Do not do that in
v1** — it adds risk to the known-good Windows path for no porting benefit.

---

## 4. Work breakdown

### Phase 0 — Seed the new project

**Tasks**

1. Copy the upstream tree without build output or Git metadata:
   ```sh
   rsync -a --exclude '.git' --exclude 'out' \
     ~/repos/lwbasic/ ~/repos/newbasic/
   ```
2. Record provenance (upstream URL + commit `925fde3`) in `README.md` or
   `UPSTREAM.md`.
3. Add `.gitattributes` **before the first commit**:
   ```gitattributes
   * -text
   *.cpp text
   *.h text
   *.md text
   *.cmake text
   CMakeLists.txt text
   CMakePresets.json text
   *.yml text
   *.ps1 text
   *.sh text
   ```
   The blanket `* -text` protects fixtures, samples, `.BAS`, `.expected`,
   `.dat`, `.keys`, `GWBASIC.txt` and binary references from EOL conversion.
   Mark source/doc files explicitly as text so editors still normalise them.
   Verify with `git check-attr -a fixtures/m6/strings.expected`.
4. `git init`, initial commit, optionally add `upstream` as a read-only remote.
5. Keep the licence (`MIT`, upstream contributors) and add third-party font /
   stb notices when they are added.

**Exit criteria:** tree committed; `git ls-files --eol` shows fixtures as
`-text`/no conversion; no `out/` or `.git/` copied in.

---

### Phase 1 — Platform-aware build system

**Files:** `CMakeLists.txt`, `src/*/CMakeLists.txt`, `tests/CMakeLists.txt`,
new `CMakePresets.json`.

**Tasks**

- Replace global `if(MSVC)` warning setup with per-compiler blocks:
  ```cmake
  if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
  ```
  Keep `/WX` scoped to first-party targets, as upstream does. Do not turn on
  `-Werror` for vendored Catch2/RapidCheck; add targeted `-Wno-*` only after
  seeing real warnings.
- Add platform definitions once, e.g. `LWBASIC_PLATFORM_WINDOWS` /
  `LWBASIC_PLATFORM_POSIX`, or prefer the standard `_WIN32`/`__unix__` macros
  and use CMake only for source selection.
- Add options:
  ```cmake
  option(LWBASIC_WINDOW_D3D11 "Build the Win32/D3D11 window backend" ${WIN32})
  option(LWBASIC_WINDOW_SDL   "Build the SDL2 window backend" ${UNIX})
  set(LWBASIC_ALSA "AUTO" CACHE STRING "POSIX tone output: AUTO, ON or OFF")
  option(LWBASIC_BUILD_TESTS  "Build the test suite" ON)
  option(LWBASIC_WERROR       "Treat first-party warnings as errors" OFF)
  ```
  Defaults must produce a working build on each OS once the documented
  prerequisites are installed. In particular, Linux defaults need the SDL2
  development package; `-DLWBASIC_WINDOW_SDL=OFF` is the supported
  no-extra-dependencies build.
- Core target:
  - Windows: `target_link_libraries(lwbasic-core PUBLIC winmm)`.
  - POSIX: `find_package(Threads REQUIRED)` and link `Threads::Threads`;
    link `m` for `std::pow`/`std::sin` on platforms that need it.
- **Split the screen library** so the DAP server and console-only builds do
  not inherit SDL2 or D3D link dependencies:
  - `lwbasic-screen` — console, headless, glyph atlas, text helpers. No window
    libraries.
  - `lwbasic-window` — always exists (even with zero backends), contains the
    `WindowScreen` interface, the factory and whichever backends are enabled;
    links `lwbasic-screen`.
  - `lwbasic-cli` and `lwbasic-tests` link `lwbasic-window`;
    `lwbasic-dap` links only `lwbasic-screen`.
  This matters for static libraries: a `PRIVATE` SDL2 dependency on a static
  `lwbasic-screen` would still propagate to every executable that links it.
- Screen target: select sources:
  ```cmake
  set(LWBASIC_SCREEN_SOURCES
    src/headless_screen.cpp
    src/glyph_atlas.cpp)
  if(WIN32)
    list(APPEND LWBASIC_SCREEN_SOURCES
      src/console_screen_win32.cpp
      src/glyph_atlas_gdi.cpp)
  else()
    list(APPEND LWBASIC_SCREEN_SOURCES
      src/console_screen_posix.cpp
      src/glyph_atlas_stb.cpp)
  endif()
  add_library(lwbasic-screen STATIC ${LWBASIC_SCREEN_SOURCES})
  ```
- Window target: select zero or one backend:
  ```cmake
  set(LWBASIC_WINDOW_SOURCES src/screen_factory.cpp)
  if(LWBASIC_WINDOW_D3D11)
    list(APPEND LWBASIC_WINDOW_SOURCES src/d3d11_screen.cpp)
  endif()
  if(LWBASIC_WINDOW_SDL)
    list(APPEND LWBASIC_WINDOW_SOURCES src/sdl_screen.cpp)
  endif()
  add_library(lwbasic-window STATIC ${LWBASIC_WINDOW_SOURCES})
  target_include_directories(lwbasic-window PUBLIC include)
  target_link_libraries(lwbasic-window PUBLIC lwbasic-screen)
  if(LWBASIC_WINDOW_D3D11)
    target_link_libraries(lwbasic-window PRIVATE d3d11 dxgi d3dcompiler)
  endif()
  if(LWBASIC_WINDOW_SDL)
    target_link_libraries(lwbasic-window PRIVATE ${SDL2_TARGET})
  endif()
  ```
  Pass `LWBASIC_WINDOW_D3D11` / `LWBASIC_WINDOW_SDL` as compile definitions so
  `screen_factory.cpp` and `--help` can report the backend. Add
  `add_subdirectory(src/lwbasic-window)` after `lwbasic-screen` and before
  `lwbasic-cli`/tests.
- SDL2 discovery: try `find_package(SDL2 CONFIG QUIET)`, then
  `pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)`. Normalise the
  imported target into `SDL2_TARGET` (e.g. `SDL2::SDL2` or
  `PkgConfig::SDL2`). If not found and `LWBASIC_WINDOW_SDL=ON`, fail with a
  package-install hint (`libsdl2-dev` on Debian/Ubuntu). If `OFF`, build a
  console-only binary.
- DAP target: link `ws2_32` only on Windows. Link `lwbasic-screen` only, not
  `lwbasic-window`.
- Add `CMakePresets.json` with `windows-msvc`, `linux-gcc`, `linux-clang`,
  `linux-console-only`, `linux-asan`.
- Keep test registration and `LWBASIC_FIXTURES_DIR` working on both.

**Exit criteria**

```sh
# Linux
cmake -S . -B build -G Ninja && cmake --build build
# Windows
cmake -S . -B out -A x64 && cmake --build out --config Release
```
The configure step must succeed on both. Before the source port lands, a
temporary `-DLWBASIC_WINDOW_SDL=OFF` build can prove the core/headless path.

---

### Phase 2 — Portable `Host` services

**Files:** `src/lwbasic-core/src/host.cpp`,
`src/lwbasic-core/src/host_win32.cpp`,
`src/lwbasic-core/src/host_posix.cpp`,
`src/lwbasic-core/include/lwbasic/host.h` (probably unchanged),
`src/lwbasic-core/include/lwbasic/screen/screen.h` and `src/lwbasic-core/src/vm.cpp`
(for the `SHELL` suspend/resume hooks).

**Design**

Keep the `Host` interface exactly as-is. Move implementations:

```
host.cpp          SecondsSinceMidnight, DateString, TimeString,
                  WriteToPrinter, a private GetEnv() helper
host_win32.cpp    SystemHost::RunShell, SystemHost::PlayTone
host_posix.cpp    SystemHost::RunShell, SystemHost::PlayTone
```

Clock/date/time:

```cpp
std::tm LocalTime(std::time_t t) {
  std::tm out{};
#if defined(_WIN32)
  localtime_s(&out, &t);
#else
  localtime_r(&t, &out);
#endif
  return out;
}
```

Do not use `std::localtime` (not thread-safe, and the tests run in one process
but the DAP server may not).

Environment/printer:

- Replace `_dupenv_s` with a small `GetEnv(name)` helper. On Windows use
  `_dupenv_s` to keep the current warning-clean behaviour; on POSIX use
  `std::getenv`.
- Keep `LWBASIC_PRINTER` and the default `lwbasic.prn` file. Path semantics
  are the same.

`SHELL`:

- Windows: retain `CreateProcessW` + `cmd.exe /c` (or `cmd.exe` when the
  command is empty), `WaitForSingleObject`, `GetExitCodeProcess`.
- POSIX: use `fork`/`exec` or `std::system`, then decode the wait status:
  ```cpp
  const int status = std::system(line.c_str());
  if (status == -1) return -1;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return -1;
  ```
  Empty command: launch `$SHELL` (fallback `/bin/sh`) interactively. Because
  `std::system` always adds `sh -c`, handle the empty case with `execl` or
  `posix_spawn` directly.
- **Suspend the screen around the child.** Add to `Screen`:
  ```cpp
  virtual void SuspendForChildProcess() {}
  virtual void ResumeAfterChildProcess() {}
  ```
  Call them from `vm.cpp` around the `host_->RunShell(command)` call (currently
  near line 2900). POSIX `ConsoleScreen` restores termios and leaves the
  alternate screen; Windows console and windowed/SDL backends can no-op
  initially. Without this, an interactive shell on Linux inherits raw mode and
  is unusable.

Tone output:

- Windows: move the existing `waveOut`/`Beep` code verbatim.
- POSIX v1: **always block for the requested duration**, even when silent:
  ```cpp
  std::this_thread::sleep_for(std::chrono::milliseconds(capped_ms));
  ```
  That preserves `PLAY`/`SOUND`/`BEEP` timing and makes the `[.audio]` test
  pass.
- Optional audio (recommended, behind `LWBASIC_ALSA`): use `libasound`
  (`snd_pcm_open` / `snd_pcm_writei`) with the same `RenderTone` samples.
  Default `AUTO`: enable when `alsa/asoundlib.h` and `libasound` are found.
  If no device opens, sleep for the duration instead of failing.
- Alternative if ALSA is unwanted: route audio through SDL2 when the SDL
  backend is built. Do not make core link SDL2 just for sound.
- Keep `--audio-check` working; on a silent build it prints the notes and
  exits after the correct delay.

**Exit criteria**

- `test_statements`, `test_events`, `test_sound` (non-audio tags) pass on
  Linux.
- `[.audio]` "real host blocks for the tone duration" passes on Linux.
- `SHELL "echo hi"` returns 0 and prints on both; `SHELL` with no argument
  starts an interactive shell with a cooked terminal.
- `LWBASIC_PRINTER` and `lwbasic.prn` work on both.

---

### Phase 3 — POSIX console backend

**Files:** `console_screen.h`, `console_screen_win32.cpp`,
`console_screen_posix.cpp`.

**Header refactor**

Make the header platform-neutral with pimpl:

```cpp
class ConsoleScreen final : public Screen {
public:
  ConsoleScreen();
  ~ConsoleScreen() override;

  bool Init(std::atomic<bool>* break_flag);
  void Present() override;
  void OnGridChanged() override;
  void EnterFullScreen() override;
  void ExitFullScreen() override;
  std::wstring ReadLine() override;
  bool TryGetKey(wchar_t* key) override;
  void PumpMessages() override;
  void ShowCaret(bool visible);

  void SuspendForChildProcess() override;
  void ResumeAfterChildProcess() override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
```

Move the existing Win32 fields (`HANDLE`, saved buffer size, cursor state)
into `Impl` in `console_screen_win32.cpp`. Behaviour and the Ctrl+C handler
stay identical.

**POSIX implementation**

- `Init`: require `isatty(STDIN_FILENO)` and `isatty(STDOUT_FILENO)`;
  otherwise return false so the caller can report "no terminal". Read
  `TIOCGWINSZ` for the saved size.
- Full screen: save `termios`; enter raw mode (`cfmakeraw`, disable `ICRNL`,
  `IXON`, echo, canonical mode); enter the alternate screen
  (`\x1b[?1049h`), hide the cursor (`\x1b[?25l`), clear, home
  (`\x1b[H`). Clamp the grid to the terminal size exactly as the Win32
  backend does.
- `Present`: encode the `wchar_t` grid to UTF-8 and emit one frame. Use
  absolute cursor addressing per row (`\x1b[{row};1H`) or `\r\n` with raw
  output, and erase to end of line (`\x1b[K`) so a wider previous frame
  leaves no residue; map CGA attributes to ANSI SGR:
  - foreground 0..7 → `30..37`, 8..15 → `90..97`
  - background 0..7 → `40..47`, 8..15 → `100..107`
  - remember CGA is IRGB, ANSI is RGB: 1=blue, 2=green, 3=cyan, 4=red,
    5=magenta, 6=brown/yellow, 7=white.
  - coalesce attribute changes into runs, as the Win32 backend does, to keep
    output small.
  - restore the default attribute and position the cursor at the grid cursor.
- `OnGridChanged`: same clamp logic as Win32.
- `ExitFullScreen` / destructor: restore `termios`, show the cursor, leave the
  alternate screen (`\x1b[?1049l`). Make restore idempotent and call it from
  the destructor, `ExitFullScreen`, and an `atexit`/signal path.
- Input:
  - `ReadLine`: read raw bytes from `STDIN_FILENO`, decode UTF-8 to
    `wchar_t`, feed the existing `EditLine` via a `KeySource`. Handle `\r` and
    `\n` as Enter, `0x7F`/`0x08` as Backspace, `0x1B` as Escape.
  - Arrow keys and function keys arrive as CSI/SS3 sequences beginning with
    `0x1B`. The existing editor treats a bare Escape as "abandon line".
    To avoid arrows aborting input, peek with a short timeout (e.g. 20–30 ms)
    after `0x1B`: if more bytes follow, consume the sequence and ignore or map
    it; if not, treat it as Escape. Keep mapping limited to what GW-BASIC
    needs; document unsupported keys.
  - `TryGetKey`: `poll()` with a zero timeout, read one decoded key; return
    false when nothing is pending.
- Break handling: install a `SIGINT` handler with `sigaction`. The handler
  should only set a `volatile sig_atomic_t`; `PumpMessages()` (called by
  `Screen::PresentIfDirty`) copies that into the `std::atomic<bool>*` from
  `Init`. Do not call non-async-signal-safe functions from the handler.
  Handle `SIGWINCH` by re-reading `TIOCGWINSZ` and clamping the grid.
  For `SIGTERM`/`SIGHUP`, set a shutdown flag and use the destructor/normal
  exit path to restore termios; a best-effort `write()` of the alternate-
  screen/cursor restore sequences is acceptable from the handler, but do not
  rely on it for termios.
- `PumpMessages`: transfer the pending signal flag; no window events.
- `ReadLine` must restore cooked mode before returning? The design keeps raw
  mode for the whole session and lets the editor do line editing; keep that,
  but `SuspendForChildProcess` must restore cooked mode.
- `ShowCaret`: emit `\x1b[?25h`/`\x1b[?25l`.
- Wide-character conversion: write explicit `AppendUtf8(std::string*,
  wchar_t)` helpers. Do not rely on `std::wstring_convert` (deprecated in
  C++17 and removed in later standards) or on `wchar_t` being 16 bits.
  Most CP437 mappings are BMP; reject or substitute surrogates.

**Windows regression check:** the Win32 backend must still pass its current
behaviour (full-screen buffer, Ctrl+C, raw input, caret). Keep its code
semantically identical while moving it into `Impl`.

**Exit criteria**

- `lwbasic` starts a REPL in an xterm/konsole/GNOME Terminal; typing, Enter,
  Backspace and Escape work; `SYSTEM` restores the terminal.
- `PRINT`, `INPUT`, `LINE INPUT`, `LOCATE`, `COLOR`, `CLS`, `WIDTH`, `VIEW
  PRINT` and `INKEY$` behave.
- `Ctrl+C` during `RUN` prints `Break in <line>` instead of killing the
  process.
- Closing the terminal or pressing Ctrl+D ends the session cleanly.
- `SHELL "echo hi"` works and interactive `SHELL` leaves the terminal usable.
- No terminal corruption after a normal exit; verify with `stty sane` not
  needed.

---

### Phase 4 — Portable glyph atlas

**Files:** `glyph_atlas.h` (interface mostly unchanged),
`glyph_atlas.cpp` (shared), `glyph_atlas_gdi.cpp` (Windows),
`glyph_atlas_stb.cpp` (Linux), `third_party/stb/stb_truetype.h`,
`third_party/fonts/`.

**Why not just keep GDI logic under `#ifdef`:** GDI is Win32-only. The atlas
is also used by `WriteViewPpm` and the window renderer, so Linux needs a real
implementation, not a stub, or tests and screenshots lose text.

**Design**

- Move the font-independent parts into `glyph_atlas.cpp`:
  - `GlyphAtlas` accessors (already in the header).
  - `DrawProceduralGlyphs` (CP437 shade/block glyphs 0xB0–0xDF).
  - `ThickenGlyphs`.
  - A single `RenderGlyphAtlas` entry point that dispatches to the platform
    rasteriser, then applies the shared post-processing.
- Windows: keep the GDI implementation from upstream unchanged (same
  candidates: Terminal/Consolas/Lucida Console/Courier New, same
  shrink-to-fit, same thresholding). This preserves the current look and
  screenshots.
- Linux: `stb_truetype` + a bundled TrueType monospace font.
  - Vendor `stb_truetype.h` with its licence header.
  - Bundle a permissively licensed font. Recommended: **DejaVu Sans Mono**
    (Bitstream Vera / public-domain additions; include its licence file) or
    **Liberation Mono** (SIL OFL 1.1). DejaVu is widely available and has
    good CP437 coverage.
  - To avoid runtime path problems, embed the font at build time. CMake can
    generate a header from the `.ttf` with `file(READ ... HEX)` and a small
    `string(REGEX REPLACE ...)` conversion — no `xxd` dependency. Alternative:
    load from `share/newbasic/fonts/` and fall back to common system paths.
    Embedding is recommended for deterministic tests and single-file
    deployment.
  - Reproduce the GDI behaviour with stb:
    - `stbtt_InitFont`.
    - For a requested cell `gw x gh`, choose a pixel height and shrink until
      the advance of `W` fits in `gw` (mirroring the GDI loop).
    - Rasterise each CP437 glyph via `Cp437ToUnicode` (already in
      `screen.cpp`) → `stbtt_GetCodepointBitmap`.
    - Centre vertically in the cell using ascent/descent/line gap.
    - For `gh <= 16`, threshold coverage at 96 (bitmap-font look); for larger
      cells keep antialiased coverage.
    - Run procedural glyphs and thickening afterwards, exactly as Windows.
  - Set `atlas.ok = true` when the font loads and at least one glyph draws.
  - If the font fails to load, fall back to a small built-in 8x16 CP437
    bitmap (optional but cheap insurance) so `atlas.ok` is not false in a
    minimal container.
- Update the `glyph_atlas.h` comment: it no longer promises GDI.

**Alternative considered:** a pure 8x16 bitmap font with nearest-neighbour
scaling. It is dependency-free and deterministic, but upstream deliberately
moved away from chunky scaled bitmap glyphs for `--gpu-scale > 1`. Use it only
as the emergency fallback.

**Exit criteria**

- `tests/test_text_render.cpp` `[render]` cases pass on Linux: all 256 glyphs
  rasterise, `'A'`/`'Z'` have ink, space does not, block glyphs are heavier
  than letters, shade glyphs are between.
- `WriteViewPpm` (`--gpu-shot`) produces a legible text screenshot on Linux.
- Windows rendering is pixel-comparable to the current output at 8x16 and
  `--gpu-scale 2` (compare a few frames or screenshot hashes).

---

### Phase 5 — Window backend abstraction + SDL2 on Linux

**Files:** new `src/lwbasic-window/include/lwbasic/window/window_screen.h`,
`screen_factory.h`, `src/lwbasic-window/src/screen_factory.cpp`,
`sdl_screen.h/.cpp`; move `d3d11_screen.h/.cpp` from `lwbasic-screen` to
`lwbasic-window`; changes to `lwbasic-cli/main.cpp`,
`tests/test_text_render.cpp`, `src/lwbasic-screen/CMakeLists.txt` and the new
`src/lwbasic-window/CMakeLists.txt`.

**Problem**

`lwbasic-cli/main.cpp` and `tests/test_text_render.cpp` include
`d3d11_screen.h`, call `D3D11Screen::Create`, and `static_cast` to
`D3D11Screen` for `InjectKeys`, `InjectClick` and `ReadBackCanvasPixel`.
Linux must not compile that header's implementation, and a second backend
must satisfy the same CLI features.

**Interface**

```cpp
// window_screen.h
namespace lwbasic::screen {

class WindowScreen : public Screen {
public:
  // Scripted input, used by --keys / --clicks and the [.gpu] tests.
  virtual void InjectClick(int canvas_x, int canvas_y, int button) = 0;
  virtual void InjectKeys(const std::wstring& keys) = 0;

  // Reads a pixel the backend actually presented, by canvas coordinate.
  virtual bool ReadBackCanvasPixel(int canvas_x, int canvas_y,
                                   std::uint8_t* rgba) = 0;
};

// Returns nullptr and fills `error` when no window/device is available.
std::unique_ptr<WindowScreen> CreateWindowScreen(std::string* error,
                                                 int scale = 1);

// True when a window backend was compiled in.
bool WindowBackendAvailable();

// Name for diagnostics: "D3D11", "SDL2", or "".
const char* WindowBackendName();

} // namespace lwbasic::screen
```

- `D3D11Screen` derives from `WindowScreen`; its `Create` can remain for
  internal use or be removed once the factory is the only caller.
- `SdlScreen` implements the same interface.
- `screen_factory.cpp` contains the compile-time selection:
  ```cpp
  std::unique_ptr<WindowScreen> CreateWindowScreen(std::string* error, int scale) {
  #if defined(LWBASIC_WINDOW_D3D11)
    return D3D11Screen::Create(error, scale);
  #elif defined(LWBASIC_WINDOW_SDL)
    return SdlScreen::Create(error, scale);
  #else
    if (error) *error = "no window backend in this build";
    return nullptr;
  #endif
  }
  ```
- CLI `ChooseScreen` returns a `WindowScreen*`; `ProbeWindowPixel`,
  `ApplyScriptedClicks`, `InjectKeys` and `--gpu-check` use the interface.
  Remove the `is_console` static-cast special case or keep it only as a
  diagnostic.
- Tests use `CreateWindowScreen`; when it returns null, `WARN` + `SKIP` the
  `[.gpu]` case instead of failing. Tag backend-specific assertions if
  pixel-exact results differ. Update `tests/CMakeLists.txt` to link
  `lwbasic-window` additively (it already links `lwbasic-screen`), so the
  `[.gpu]` case can see the factory on both platforms.

**SDL2 backend design**

Use SDL2 window + renderer; keep composition close to the existing D3D path.

- `Create`:
  - `SDL_Init(SDL_INIT_VIDEO)`; on failure fill `error` and return null.
  - Create a window sized `Width() * glyph_width * scale` by
    `Height() * glyph_height * scale` (`640x400` at scale 1).
  - Create an `SDL_Renderer` (hardware, fall back to software).
  - Set `SDL_HINT_RENDER_SCALE_QUALITY=0` for crisp nearest-neighbour
    scaling; use `SDL_RenderSetLogicalSize(640, 400)` so `--gpu-scale` only
    changes the window size, not the CPU composition size.
  - Build the atlas via `RenderGlyphAtlas(8, 16)`; keep it as a texture
    (single-channel or RGBA).
- Frame composition:
  - Reuse the palette/canvas logic from `WriteViewPpm` and `text_render.h`.
  - Compose at the logical text-grid resolution: fill cell backgrounds from
    the CGA palette, blit glyph coverage from the atlas, overlay the canvas
    for graphics modes with the same 4:3 / 16:9 aspect handling as D3D11.
  - Upload one RGBA (or BGRA) streaming texture and render it. This is
    simple, testable and fast enough at 640x400: ~1 MB per frame, 30 fps.
  - Alternative: two textures (atlas + canvas) with `SDL_RenderCopy`; only
    do this if profiling shows CPU composition is a bottleneck. `--bench`
    and a frame-time log (the `LWBASIC_TIMING` equivalent) should decide.
- `OnGridChanged`: resize the window for `SCREEN 720` (160x45) and other
  modes; keep the client area equal to cells × cell size × scale.
- `Present`: render, then `SDL_RenderPresent`.
- `PumpMessages`:
  - `SDL_PumpEvents`; `SDL_PollEvent` for `SDL_QUIT`,
    `SDL_WINDOWEVENT_CLOSE`, `SDL_KEYDOWN`, `SDL_TEXTINPUT`,
    `SDL_MOUSEBUTTONDOWN`.
  - Translate UTF-8 `SDL_TEXTINPUT` to `wchar_t` via a UTF-8 decoder.
  - Map Enter/Return, Backspace, Escape, Tab and printable keys into the
    same input queues as D3D11. Keep `Ctrl+C` → break semantics
    (`TakeBreak`) if the D3D backend supports it; otherwise document.
  - Map mouse coordinates from window/logical space to canvas coordinates
    (inverse of the D3D mapping, including letterbox offsets).
- `ReadLine`: use the queued key source, feeding `EditLine` exactly like the
  console backend.
- `TryGetKey`: non-blocking pop from the queue.
- `InjectKeys` / `InjectClick`: push into the same queues.
- `ReadBackCanvasPixel`: `SDL_RenderReadPixels` on the render target (or
  read from the last composed frame if the renderer cannot read back), and
  map using the same letterboxing as the D3D implementation. `--gpu-pixel`
  should continue to mean "what was actually presented".
- `Closed`: `SDL_QUIT`/window-close sets a flag; `InputExhausted` returns it.
- Cleanup: destroy textures, renderer, window; `SDL_Quit` in the destructor.
- Headless/SSH: if `SDL_Init` fails (no `DISPLAY`/`WAYLAND_DISPLAY`), return
  null so `ChooseScreen` falls back to the console with a clear message.
- DPI: request `SDL_WINDOW_ALLOW_HIGHDPI` and use `SDL_GetRendererOutputSize`
  for readback/window math; do not assume logical pixels equal device pixels.

**CLI changes**

- Replace `sscanf_s` with `std::sscanf` (both call sites).
- Update help text:
  - `--gpu` → "Render in a window (D3D11 on Windows, SDL2 on Linux). This is
    the default when a window backend is built."
  - `--cli` → "Use the terminal instead of the window."
  - `--gpu-pixel` → "read a pixel from the presented window".
  - Remove "D3D11" from user-facing errors; use `WindowBackendName()`.
- Change "lwbasic requires a console (conhost or Windows Terminal)." to a
  generic "lwbasic requires a terminal." message.
- Default `g_use_gpu` should be `WindowBackendAvailable()`; on Linux without
  SDL2 it is false, so the console is used without an error.
- Keep `--gpu-scale`, `--keys`, `--clicks`, `--gpu-shot`, `--gpu-pixel`,
  `--gpu-check`, `--audio-check` and `--bench` working. `--gpu-check` should
  name the backend in its output and still PASS/FAIL on the same known frame.
- `WriteViewPpm` stays as-is after the atlas becomes portable; it is the
  headless screenshot path and should not require a window backend.

**Exit criteria**

- On Linux with SDL2:
  - `lwbasic samples/Beethoven.bas` opens a window and plays.
  - `lwbasic samples/Mandelbrot1200.bas` displays graphics at a reasonable
    frame rate; `--gpu-shot` writes a correct PPM.
  - `lwbasic --gpu-check` reports PASS.
  - `--keys` and `--clicks` drive a scripted session.
- On Linux without SDL2 (`-DLWBASIC_WINDOW_SDL=OFF`):
  - the binary builds and runs console-only; `--gpu` prints a clear
    "no window backend in this build" message and falls back.
- On Windows: D3D11 remains the default, and all existing behaviour,
  screenshots and `[.gpu]` tests still pass.

---

### Phase 6 — DAP socket portability

**Files:** `src/lwbasic-dap/main.cpp`, new `socket.h`,
`socket_win32.cpp`, `socket_posix.cpp`, `src/lwbasic-dap/CMakeLists.txt`.

**Tasks**

- Introduce a tiny socket abstraction used only by the DAP executable:
  ```cpp
  class Socket {
  public:
    static Socket ListenLoopback(int port, std::string* error);
    Socket Accept(std::string* error);
    std::ptrdiff_t Recv(char* data, std::size_t size);
    std::ptrdiff_t Send(const char* data, std::size_t size);
    void Close();
    int BoundPort() const;
  };
  ```
  Or a `SocketBuf` that owns a platform handle and implements
  `std::streambuf`, preserving the current design.
- Windows unit: `WSAStartup`/`WSACleanup`, `SOCKET`, `closesocket`,
  `::recv`/`::send`, `winsock2.h` with `WIN32_LEAN_AND_MEAN`/`NOMINMAX`
  before project headers (keep the existing include-order comment).
- POSIX unit:
  - `<sys/socket.h>`, `<netinet/in.h>`, `<unistd.h>`, `<arpa/inet.h>`.
  - No `WSAStartup`; `close` instead of `closesocket`.
  - Use `send(..., MSG_NOSIGNAL)` or ignore `SIGPIPE` so a disconnected
    editor does not kill the server.
  - Retry `accept`/`recv` on `EINTR`.
- Bind to loopback only, as today. `--port 0` still reports the actual port
  on stderr.
- Keep the stdio transport unchanged; it is already portable.
- `ConsoleScreen` is constructed but never `Init`ed in DAP; the POSIX
  implementation must be safe in that state (no terminal access in the
  constructor/destructor).
- Update CMake link list: `ws2_32` on Windows only; POSIX needs no extra
  library.

**Exit criteria**

- `dap-smoke` (stdio) passes on Linux and Windows.
- `lwbasic-dap --port 0` prints a port; a scripted client can connect,
  initialize, set a breakpoint, continue and disconnect.
- Disconnecting the client does not terminate the server; a second
  connection works.
- No `SIGPIPE` crash when the client closes mid-response.

---

### Phase 7 — Core filesystem/path semantics on Linux

**Files:** `src/lwbasic-core/src/repl.cpp`, `program_files.cpp`,
`vm.cpp`, new `host_path.h/.cpp`.

**Tasks**

- Centralise DOS path handling:
  ```cpp
  std::string HostPath(const std::string& dos_path);
  ```
  - Windows: return unchanged.
  - POSIX: translate `\` to `/`; leave drive letters (`C:`) documented as
    unsupported, or map `C:`/`C:\` to `/` only if a path mapping option is
    added later.
  - Apply it to `OPEN`, `CLOSE`, `SAVE`, `LOAD`, `CHAIN`, `MERGE`, `KILL`,
    `NAME`, `MKDIR`, `CHDIR`, `RMDIR`, `FILES`, and `RUN`/`LOAD` file
    arguments. Do **not** rewrite strings inside `SHELL`; the shell should
    receive the user's command as written.
- Preserve DOS semantics where they matter:
  - `NAME old AS new` must fail if `new` exists on POSIX. Implement
    `RenameNoReplace` (check target, then `rename`; handle the race with
    `link`/`unlink` or `renameat2(RENAME_NOREPLACE)` where available).
  - `RMDIR` must fail for a non-directory and for a non-empty directory.
  - `KILL` must fail for a directory (DOS `KILL` deletes files only).
- `FILES`: sort entries for deterministic output on both platforms. Match
  DOS's 8.3 display? The current implementation prints long names; keep
  that and document, or add a `FILES` formatting note. Do not change the
  golden expectations unless a fixture depends on order.
- Case sensitivity: DOS is case-insensitive, Linux is not. Decide:
  - v1: document the divergence and require exact case on Linux; or
  - add `OpenInputFile`/`OpenOutputFile` helpers that try exact case, then
    scan the directory for a case-insensitive match. This is friendlier for
    `.BAS` programs written on Windows. Recommend the helper for input
    (`LOAD`, `OPEN ... FOR INPUT`, `CHAIN`, `MERGE`) and exact case for
    output.
- CRLF: `LineStore::ParseTextBas` already accepts LF and CRLF; confirm with
  tests. Do not add global text-mode translation.
- `std::filesystem::current_path`/`directory_iterator` work on both. Use
  `std::error_code` overloads, as upstream already does.
- `SHELL` and DOS internal commands: document that `SHELL "DIR"` works on
  Windows but not Linux (use `ls`), because the child is the platform shell.

**Exit criteria**

- `test_files`, `test_fileio`, `test_random_files`, `test_program_control`,
  `test_chaining`, `test_renum` pass on Linux.
- A program using `OPEN "DATA\FILE.DAT"` works on Linux via path
  translation.
- `NAME` refuses to overwrite on Linux, matching Windows/DOS.
- `FILES` output order is stable.

---

### Phase 8 — Tests, CI, documentation, packaging

**Tests**

- Add / update platform guards:
  - `tests/test_text_render.cpp`: replace `D3D11Screen` with
    `CreateWindowScreen`; `[.gpu]` uses `SKIP` when no backend. Add a
    backend-agnostic check that the factory name is stable.
  - `tests/test_sound.cpp`: no source change expected; confirm the
    `[.audio]` duration test passes on POSIX (silent build still sleeps).
  - `tests/test_repl.cpp`, `test_graphics.cpp`, `test_renum.cpp`,
    `test_coverage.cpp`: the hidden `[.gpu]`/`[.audio]` tags are already
    Windows-friendly; no changes unless include hygiene is needed.
- Include hygiene sweep: GCC 15 with `-Wall -Wextra` will surface missing
  `<cstdint>`, `<cstdio>`, `<array>`, `<memory>`, `<algorithm>`, `<cmath>`,
  `<utility>` includes. Fix them in first-party files; add targeted
  suppressions for vendored code only.
- Keep all golden fixtures and expected output byte-identical. If a test
  depends on host behaviour (paths, directory order), fix the production
  code, not the fixture.
- Add CTest working-directory hygiene if tests write files: run each test
  in the build tree (as now) and avoid parallel tests sharing fixed names.
  If any test uses `lwbasic.prn` or `accounting.dat` in the CWD, use unique
  temp paths under `CMAKE_CURRENT_BINARY_DIR`.
- `dap-smoke` already uses `$<TARGET_FILE>` and a `TMP_DIR`; verify on
  Linux.

**CI**

Replace the single Windows job with a matrix:

```yaml
jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: pip install "clang-format==19.1.0"
      - run: |
          files=$(find src tests -name '*.cpp' -o -name '*.h')
          clang-format --dry-run --Werror $files
  windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake -S . -B out -A x64
      - shell: pwsh
        run: ./tools/verify.ps1 -NoRetry
  linux:
    strategy:
      matrix:
        compiler: [gcc, clang]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get update && sudo apt-get install -y ninja-build libsdl2-dev
      - run: cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=${{ matrix.compiler }}
      - run: cmake --build build --parallel
      - run: ctest --test-dir build --timeout 120 --output-on-failure
  linux-console-only:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake -S . -B build -G Ninja -DLWBASIC_WINDOW_SDL=OFF
      - run: cmake --build build --parallel
      - run: ctest --test-dir build --timeout 120 --output-on-failure
```

Notes:

- Keep `[.gpu]` out of the default CTest run (the leading dot already does
  this in Catch2). Add an optional `xvfb-run` job for SDL GPU checks once
  they are stable; Windows WARP covers headless there.
- Add `tools/verify.sh` mirroring `verify.ps1`: build, run CTest with a
  timeout, capture logs under `build/test-logs/`, optionally retry once.
- Pin the clang-format version to the existing `.clang-format` intent.
- Consider a `linux-asan-ubsan` preset; run it manually or as a non-blocking
  job initially.

**Documentation**

- `README.md`: platform matrix, prerequisites, build/test/run commands for
  Windows and Linux, backend fallback, and the fact that the D3D11 backend is
  Windows-only while SDL2 is the Linux window.
- `docs/portability.md`: table of host mappings and documented divergences:
  - `SHELL` uses `cmd.exe` vs `$SHELL`/`/bin/sh`.
  - DOS paths: `\` translated to `/` on POSIX; drive letters unsupported.
  - Case-insensitive filenames: exact case by default, or helper fallback.
  - Console: Win32 console API vs ANSI/termios; colour/keys may differ.
  - Audio: WinMM/waveOut vs ALSA or silent timed tone.
  - Fonts: GDI system font vs bundled DejaVu Sans Mono.
  - Window: D3D11 vs SDL2; pixel-exact output not guaranteed.
  - `NAME`/`RMDIR` semantics are kept DOS-compatible on POSIX.
- Update `docs/status.md` with a dated dual-platform snapshot.
- Add `docs/building-linux.md` if the README gets long.
- Keep the upstream specification plan as `docs/lwbasic-spec-plan.md`; this
  file is the port plan. Link both from `README.md`.

**Packaging (optional but recommended)**

- Add `install()` rules: binaries to `bin/`, docs/fixtures to
  `share/newbasic/`, font data embedded or installed under
  `share/newbasic/fonts/`.
- Linux: provide a `.desktop` file only if a windowed mode is the default;
  not required for v1.
- Windows: keep the existing VS/CMake flow; no installer required.

**Exit criteria**

- CI green on Windows MSVC, Linux GCC and Linux Clang.
- Console-only Linux build green.
- `clang-format` clean.
- README commands work from a clean clone on both OSes.
- Documented divergences reviewed against the acceptance matrix.

---

## 5. Suggested PR sequence

Keep each PR small and reviewable; the Windows build should stay green at
every step.

1. **Seed + `.gitattributes` + CMake platform blocks** (no behaviour change
   on Windows).
2. **Host split**: portable clock/printer, Win32/POSIX process and audio.
3. **Console pimpl + POSIX console**: Linux REPL works; Windows unchanged.
4. **Glyph atlas split**: Windows keeps GDI; Linux uses stb + bundled font.
5. **WindowScreen interface + factory**: D3D11 adapted, CLI/tests use the
   interface. No Linux window yet.
6. **SDL2 backend**: Linux window, graphics, input, screenshot, `--gpu-check`.
7. **DAP socket split**: POSIX TCP transport; stdio already portable.
8. **Filesystem semantics**: path translation, `NAME`/`RMDIR`/`FILES`
   parity.
9. **Include hygiene + test fixes + Linux CI**: full matrix green.
10. **Docs, presets, verify.sh, packaging**: release-ready.

---

## 6. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Git rewrites CRLF/CP437 fixtures | Golden tests fail or silently change bytes | `.gitattributes` with `* -text` before first commit; verify `git ls-files --eol` |
| POSIX terminal left in raw mode after crash | User's shell is unusable | Restore in destructor, `ExitFullScreen`, `atexit`, and `SIGINT`/`SIGTERM` handlers; test by killing during a run |
| `SHELL` inherits raw mode / alternate screen | Interactive shell unusable on Linux | Add `SuspendForChildProcess`/`ResumeAfterChildProcess` to `Screen`; call around `RunShell` |
| `wchar_t` is 32-bit on Linux, 16-bit on Windows | Encoding bugs, wrong CP437 mappings | Explicit UTF-8/UTF-16 conversion helpers; avoid `wcs*` and `std::wstring_convert`; keep CP437 tables in BMP |
| SDL2 not installed on target distro | No windowed mode | `LWBASIC_WINDOW_SDL=OFF` builds console-only; clear configure error with package hint; runtime fallback |
| Bundled font licensing / size | Legal or repo-size concern | Use DejaVu (permissive) or Liberation (OFL); include licence; embed compressed or install as data; ~300 KB is acceptable |
| GDI vs stb rendering differs | Windows/Linux screenshots differ; tests brittle | Make tests assert structural properties (ink presence/count), not pixel-exact cross-platform hashes; compare within each platform |
| ANSI colour/key differences between terminals | Visual/input parity gaps | Map CGA→ANSI carefully; document; keep Win32 backend on Windows; test in xterm + GNOME Terminal + Windows Terminal |
| `NAME`/`RMDIR`/`FILES` semantics differ | Tests fail or data loss on Linux | Explicit DOS-compatible shims; tests for overwrite/remove ordering |
| SDL CPU composition too slow | Graphics programs stutter | Compose at 640x400 logical size, upload one texture; measure; move to two-texture GPU path only if needed |
| GPU tests unavailable in CI | Regressions in window path | Keep `[.gpu]` hidden; add `xvfb-run` job later; keep `--gpu-check` as a manual smoke |
| Upstream drift | Merge conflicts | Confine changes to platform files and small seams; keep upstream names; record the upstream commit |
| MSVC-only code in shared headers | Linux build breaks unexpectedly | Compile Linux in CI on every PR; grep CI for `windows.h` outside `*_win32.cpp`/`d3d11*` |

---

## 7. Alternatives considered

| Option | Why not (for v1) |
|---|---|
| **Console-only Linux, no window** | Easiest, but `SCREEN`/`PSET`/`LINE`/`CIRCLE`/`DRAW` and the Mandelbrot samples have no display; it weakens "runs on Linux" to "interprets on Linux". Use as a fallback build, not the only target. |
| **ncurses/PDCurses console** | Adds a dependency and a second abstraction; termios + ANSI is enough for an 80x25 grid with 16 colours, and keeps the backend close to the existing Win32 code. |
| **Raw X11 + OpenGL window** | More code and more platform-specific surface than SDL2; no Wayland story without extra work. SDL2 handles X11/Wayland/Windows. |
| **GLFW + OpenGL** | Viable, but SDL2 also provides input, UTF-8 text input, mouse, timing and (optionally) audio in one dependency; GLFW would need separate text-input handling. |
| **FreeType + fontconfig** | Better system-font integration, but adds two dependencies and runtime variability. stb_truetype + a bundled font is deterministic and dependency-free. |
| **Replace D3D11 with SDL2 on Windows too** | Reduces backend count long-term but risks the known-good Windows rendering and screenshot tests. Keep D3D11 in v1; revisit after Linux is green. |
| **Two source trees** | Duplicates the VM and guarantees drift. One tree with seams is the whole point. |

---

## 8. Acceptance test matrix

Run these before calling the port done. "Linux console" means
`-DLWBASIC_WINDOW_SDL=OFF` or no display; "Linux window" means SDL2 with a
display.

| Scenario | Windows | Linux console | Linux window |
|---|---|---|---|
| Configure + build from clean clone | ✅ | ✅ | ✅ |
| `ctest` default suite | ✅ | ✅ | ✅ |
| `--bench` completes all four cases | ✅ | ✅ | ✅ |
| `fixtures/m0/hello.bas` output matches `.expected` | ✅ | ✅ | ✅ |
| `fixtures/m6/strings.bas` (CP437) matches | ✅ | ✅ | ✅ |
| Direct-mode REPL: type, Enter, Backspace, Escape | ✅ | ✅ | ✅ |
| `INPUT` / `LINE INPUT` | ✅ | ✅ | ✅ |
| `Ctrl+C` during `RUN` → `Break in N` | ✅ | ✅ | ✅ |
| `SYSTEM` exits and restores the terminal | ✅ | ✅ | ✅ |
| `SHELL "echo hi"`; interactive `SHELL` | ✅ | ✅ | ✅ |
| `BEEP` / `SOUND` / `PLAY` timing | ✅ | ✅ (silent or ALSA) | ✅ (silent or ALSA) |
| `samples/Beethoven.bas` | ✅ | n/a (audio optional) | ✅ |
| `samples/Mandelbrot1200.bas` graphics | ✅ | ❌ (expected) | ✅ |
| `--gpu-shot` PPM is legible | ✅ | ✅ (headless dump) | ✅ |
| `--gpu-check` PASS | ✅ | n/a | ✅ |
| `--gpu-pixel` reads presented pixel | ✅ | n/a | ✅ |
| `--keys` / `--clicks` scripted session | ✅ | n/a | ✅ |
| `LOAD`/`SAVE`/`CHAIN` with `\` paths | ✅ | ✅ (translated) | ✅ |
| `NAME old AS new` refuses overwrite | ✅ | ✅ | ✅ |
| `FILES` stable order | ✅ | ✅ | ✅ |
| `OPEN` random files | ✅ | ✅ | ✅ |
| `dap-smoke` stdio | ✅ | ✅ | ✅ |
| `lwbasic-dap --port 0` TCP connect | ✅ | ✅ | ✅ |
| `clang-format --dry-run --Werror` | ✅ | ✅ | ✅ |

---

## 9. Decisions to confirm

These are the choices that materially change the work. The plan above assumes
the recommended answer for each.

1. **Is a GUI window required on Linux for v1?**
   Recommended: yes, via SDL2, but land the console path first so the
   interpreter is usable early.
2. **Is adding SDL2 and a bundled font acceptable?**
   Recommended: yes. SDL2 is packaged on all major distros; the font is
   needed for graphics screenshots and `--gpu-shot`. If not, the fallback is
   console-only + a built-in bitmap font.
3. **Should the new project merge upstream `lwbasic` changes?**
   Recommended: keep upstream names and the `upstream` remote; confine edits
   to platform files. If this is a one-way fork, the naming constraints can
   be relaxed.
4. **Which Linux baseline?**
   Recommended: GCC 9+ and Clang 9+, C++17, 64-bit; test on Ubuntu LTS in CI.
   `std::filesystem` needs `-lstdc++fs` only on GCC 8, so either require GCC 9
   or add the conditional link.
5. **Is macOS in scope?**
   Recommended: no for v1, but the POSIX console + SDL2 backend should make
   it mostly work; leave it as a later target and avoid Windows- or
   Linux-specific assumptions in the seams.
6. **Audio on Linux: ALSA or silent?**
   Recommended: silent, correctly timed by default; enable ALSA via
   `LWBASIC_ALSA=AUTO` when headers/libs are present. Avoid making audio a
   hard build dependency.

---

## 10. Definition of done

- A clean clone builds and tests on Windows 11 with MSVC and on Linux with
  GCC and Clang, using only documented prerequisites.
- The full default CTest suite is green on all three configurations, and the
  console-only Linux configuration is green.
- The same `lwbasic` binary can run the REPL, the sample programs, the DAP
  server and the golden fixtures on both platforms.
- Windows D3D11 behaviour is unchanged; Linux has a working console and (with
  SDL2) a working windowed backend.
- Platform differences are listed in `docs/portability.md`, and no
  Windows-only header or library leaks into the Linux build.
- `clang-format` is clean and CI enforces the matrix.
