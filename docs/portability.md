# Platform notes

`newbasic` keeps the GW-BASIC dialect, the tokenized/protected `.BAS` formats and
the golden fixtures identical on Windows and Linux. Only the host-facing pieces
differ. This page records the mapping and the known divergences.

## Host services

| Area | Windows | Linux/POSIX |
|---|---|---|
| Clock/date/time | `localtime_s` | `localtime_r` |
| `SHELL` | `CreateProcessW` + `cmd.exe /c` (or interactive `cmd.exe`) | `fork`/`execl` for an interactive `$SHELL`; `std::system` for commands (exit status decoded with `WIFEXITED`) |
| Tone output | WinMM `waveOut` with a `Beep` fallback | Blocks for the tone duration; silent by default. An optional ALSA sink can be added without changing the dialect. |
| Printer | `NEWBASIC_PRINTER` or `newbasic.prn` | Same |
| Filesystem | `std::filesystem` / `std::fstream` | Same, with DOS path translation (below) |

### `SHELL` and the terminal

The POSIX console enters raw mode and the alternate screen. Before starting a
child process it restores the saved termios state and leaves the alternate
screen, then re-enters raw mode afterwards (`Screen::SuspendForChildProcess` /
`ResumeAfterChildProcess`). This keeps an interactive `$SHELL` usable.

`SHELL "DIR"` is a DOS command and only works where `cmd.exe` exists. On Linux
use normal shell commands (`ls`, `cat`, ...). The exit status is the child's
status, or `128 + signal` when the child was killed by a signal.

A bare `SHELL` starts an interactive shell, so it is only allowed when both the
host and the screen can attach it to a terminal. Windowed and headless runs
report “SHELL without a command needs a terminal” instead of waiting forever for
invisible input.

## Paths and file names

Paths are normalized at the host boundary, not inside string literals or the
lexer: POSIX maps `\` to `/`, and Windows maps `/` to `\`. The conversion is
applied to the program-file argument, `OPEN`, `SAVE`, `LOAD`, `CHAIN`,
`MERGE`, the DOS commands, the printer path, `--gpu-shot` and
`NEWBASIC_FONT`. It is deliberately **not** applied to `SHELL` command lines,
because the shell has its own quoting and path rules.

Drive letters such as `C:` are not mapped on POSIX; use host paths for files
outside the working directory. A DOS path like `C:\DATA\FILE.DAT` becomes
`C:/DATA/FILE.DAT`, which is a relative path unless a directory named `C:`
exists.

DOS commands keep their DOS semantics where it matters:

- `NAME old AS new` refuses to overwrite an existing destination.
- `KILL` operates on files, not directories.
- `RMDIR` operates on directories, not files.
- `FILES` sorts its listing so output is deterministic across filesystems.

Filenames on Linux are case-sensitive. Programs written on Windows should use
the exact case, or the user can create a case-insensitive mount. This is a
documented divergence, not a hidden fallback.

Text `.BAS` files accept both LF and CRLF. The repository's `.gitattributes`
marks fixtures and samples with `-text` so their bytes (including CP437 and
CRLF) survive checkout.

## Console and keyboard

- Windows keeps the Win32 console API (`WriteConsoleW`, console buffer sizing,
  `ReadConsoleInputW`, `SetConsoleCtrlHandler`).
- POSIX uses `termios` plus ANSI escapes: alternate screen, cursor addressing,
  16-colour SGR and UTF-8 output.
- `Ctrl+C` maps to the GW-BASIC break (`Break in <line>`) on both. On POSIX the
  signal handler only sets a `sig_atomic_t`; `PumpMessages` forwards it to the
  VM's break flag.
- Backspace and DEL (0x7F) both edit input; POSIX also normalizes DEL to
  `CHR$(8)` so programs that test `INKEY$ = CHR$(8)` (including the accounting
  sample's custom field editor) work as they do on Windows.
- Arrow/function-key sequences are consumed rather than inserted into line
  input. Mapping them to `INKEY$` codes is not implemented; the original
  backend also left this for the interpreter's key-mapping milestone.
- `SIGWINCH` re-reads the terminal size and clamps the grid, mirroring the
  Win32 "console too small" behaviour.
- If stdout/stdin is not a terminal, the POSIX console backend declines to
  initialise. Use a real terminal or the windowed backend. `--gpu-shot` is the
  exception: it falls back to a headless screen so a PPM can be captured in CI
  or over SSH without a TTY.

## Glyphs and fonts

- Windows keeps the GDI rasteriser and its font candidates (Terminal,
  Consolas, Lucida Console, Courier New).
- Linux uses `stb_truetype` with the bundled
  `third_party/fonts/DejaVuSansMono.ttf` (Bitstream Vera/DejaVu licence in
  `LICENSE-DejaVu.txt`). Set `NEWBASIC_FONT` to override it, or install the font
  at a standard system path.
- Both paths share the procedural CP437 block/shade glyphs and the thickening
  pass. Cross-platform screenshots are structurally identical but not
  guaranteed pixel-identical.

## Window backends

| Platform | Backend | Notes |
|---|---|---|
| Windows | D3D11 | Existing backend, unchanged in behaviour |
| Linux | SDL2 | CPU composition into an ARGB8888 texture, presented through SDL2. Falls back to the console if SDL2 or a display is unavailable. |
| Neither | console | Fully supported; `--gpu-shot` still writes a PPM using the headless atlas/canvas. |

On Linux, `SDL_VIDEODRIVER=dummy` makes the SDL backend usable in a headless
smoke test. `--gpu-check`, `--gpu-pixel`, `--gpu-shot`, `--keys` and `--clicks`
work through the shared `WindowScreen` interface.

## Audio

The dialect's sound statements are synchronous. On POSIX the default host waits
for the requested duration and produces no sound, so `PLAY`/`SOUND`/`BEEP`
timing still matches. `--audio-check` therefore reports the planned notes and
waits without making noise unless an audio sink is added.

## Debug Adapter Protocol

The stdio transport is portable. The `--port` transport uses a small
platform-specific socket wrapper: Winsock on Windows, POSIX sockets on Linux.
The listener stays bound to loopback. POSIX sends with `MSG_NOSIGNAL` so a
disconnected editor cannot kill the server with `SIGPIPE`.

## Test matrix

| Scenario | Windows | Linux console | Linux SDL2 |
|---|---|---|---|
| Default CTest suite | ✅ | ✅ | ✅ |
| `[.audio]` timing | ✅ | ✅ (silent) | ✅ (silent) |
| `[.gpu]` | ✅ (D3D11/WARP) | skips | ✅ with a display or `SDL_VIDEODRIVER=dummy` |
| `dap-smoke` | ✅ | ✅ | ✅ |
| Golden fixtures | ✅ | ✅ | ✅ |
