# Debugging newbasic from VS Code on Linux

**Status:** verified on Linux (Debian 13, arm64) on 2026-10-22 at `d7536e1`.
The adapter path below was updated for the `bin/Release` layout introduced after
that commit.
**Audience:** AI coding agents (and humans) asked to make VS Code debugging work
on a Linux machine for this repository.
**Scope:** build the interpreter and DAP adapter, install the VS Code
extension, configure the workspace, run a debug session, and verify
breakpoints. This is a runbook, not a design document.

The same extension source and the same platform-neutral VSIX are used on
Windows; only the `newbasic-dap` executable is platform-specific. See
`debug-on-windows.md` for the Windows runbook.

---

## 1. What success looks like

A successful run produces all of the following:

- `bin/Release/newbasic-dap` exists and is executable.
- `build-sdl/tests/newbasic-tests` passes (CTest: 7 of 7, 0 failures).
- `code --list-extensions --show-versions` contains
  `newbasic.newbasic-debug@0.1.0`.
- **Output → newbasic** logs the adapter path, working directory and launch.
- F5 on `fixtures/m0/hello.bas` opens the `newbasic` window, prints
  `HELLO FROM NEWBASIC` and `5  10`, and terminates.
- A gutter breakpoint on `fixtures/m0/hello.bas` **file line 2** (the
  `20 PRINT 5, 10` statement) stops the session, and F10 steps to file line 3
  (`30 END`).
- Optionally, F5 on `samples/RingWorld.bas` shows `SCREEN 720` graphics while
  the session runs.

Exact test counts can change. Zero failures is the acceptance criterion.

---

## 2. Ground rules for AI agents

1. **Do not commit build output or the VSIX.** `build/`, `build-sdl/` and
   `*.vsix` are generated. Keep them out of git.
2. **Do not edit the C++ source or CMake files** to make debugging work. The
   debugger is an extension plus an existing adapter executable.
3. **Do not put a Linux-only path in `newbasic.adapterPath`** unless the build
   directory is outside the documented ones. The checked-in workspace settings
   leave it empty so one settings file works on Linux and Windows.
4. **Reload VS Code after installing the extension or changing its
   `package.json`.** The already-running window keeps the old extension code and
   the old debug contributions.
5. **Do not use a bare extension symlink as proof of installation.**
   `code --list-extensions` does not reliably list symlinked development
   extensions. Build and install the VSIX, or use the VSIX supplied by the
   human.
6. **Report evidence.** Include the adapter path, the Output → newbasic lines,
   the debug window result, and the breakpoint stop. Do not claim success from a
   build alone.
7. **Keep `debug.allowBreakpointsEverywhere` in the workspace settings.** It is
   the fallback for `.bas` files whose language id is not the one the debugger
   contributes.

---

## 3. Verified environment

| Item | Verified value |
|---|---|
| OS | Debian 13 (trixie), arm64 |
| Desktop | labwc/Wayland (`WAYLAND_DISPLAY=wayland-0`) |
| VS Code | 1.142.0, Linux arm64 |
| `code` CLI | `/usr/bin/code` |
| CMake | 3.31.6 |
| Compiler | g++ 14.2.0 |
| SDL2 | 2.32.4 (`libsdl2-dev`) |
| Node/npm/npx | Node 24.22.1, npm 11.19.1 |
| Build directory | `build-sdl` (Release, SDL window backend) |

Other versions are expected to work if VS Code is at least 1.60, CMake at least
3.20, and a C++17 compiler is available.

---

## 4. Prerequisites

Start from the repository root with the current tree:

```bash
git pull --ff-only
git status --short
```

The steps below assume the current `editors/vscode` sources (version 0.1.0,
with platform-aware adapter lookup and post-substitution validation). If local
extension changes are present, do not discard them; install the VSIX built from
that tree.

Install the compiler, CMake, SDL2 and VS Code. On Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev
```

Node/npm are only needed to package the extension. If the VSIX is copied from
another machine, they are not required.

Check the tools:

```bash
cmake --version
g++ --version
pkg-config --modversion sdl2
code --version
```

Check that VS Code has a display. Both are acceptable:

```bash
echo "DISPLAY=$DISPLAY"
echo "WAYLAND_DISPLAY=$WAYLAND_DISPLAY"
```

If neither is set, a windowed debug session cannot show its output. Use
`newbasic.showWindow = false` for headless debugging (section 10).

---

## 5. Build the interpreter and adapter

From the repository root:

```bash
cmake -S . -B build-sdl \
  -DCMAKE_BUILD_TYPE=Release \
  -DNEWBASIC_WINDOW_SDL=ON
cmake --build build-sdl --parallel
```

`NEWBASIC_WINDOW_SDL=ON` makes a missing SDL2 a hard configure error instead of
silently producing a console-only build. `AUTO` is also valid once SDL2 is
installed.

Required artifact:

```bash
test -x bin/Release/newbasic-dap && echo "adapter OK"
```

Run the process-level DAP smoke test:

```bash
cmake \
  -DDAP_EXE="$PWD/bin/Release/newbasic-dap" \
  -DTMP_DIR=/tmp/newbasic-dap-smoke \
  -P tests/dap_smoke.cmake
```

Expected final line:

```text
newbasic-dap smoke test passed
```

Run the test suite:

```bash
ctest --test-dir build-sdl --timeout 180 --output-on-failure
```

Expected: `100% tests passed, 0 tests failed out of 7`.

A console-only `build/` is optional. If you make one, use
`-DNEWBASIC_WINDOW_SDL=OFF`; the extension searches `build/` too.

---

## 6. Package and install the extension

The extension source is `editors/vscode`; it is plain JavaScript and has no
native dependencies. The VSIX is platform-neutral and can also be installed on
Windows.

```bash
cd editors/vscode
npx --yes @vscode/vsce package \
  --out "$HOME/newbasic-debug-0.1.0.vsix" \
  --allow-missing-repository
```

`npx` downloads `@vscode/vsce` on first use; that needs network. If network is
unavailable, copy a `newbasic-debug-0.1.0.vsix` produced elsewhere and skip the
package command.

Install and verify:

```bash
code --install-extension "$HOME/newbasic-debug-0.1.0.vsix" --force
code --list-extensions --show-versions | grep newbasic
```

Expected:

```text
newbasic.newbasic-debug@0.1.0
```

Do not commit the VSIX. Do not install a second copy under a different version;
remove stale copies if `code --list-extensions` shows duplicates.

---

## 7. Workspace configuration

The repository should contain these two files. Create them if they are missing;
otherwise verify their contents and do not overwrite unrelated settings.

`.vscode/launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "newbasic",
      "request": "launch",
      "name": "Run this program",
      "program": "${file}"
    }
  ]
}
```

`.vscode/settings.json`:

```json
{
  "debug.allowBreakpointsEverywhere": true
}
```

Deliberately **no** `newbasic.adapterPath`. The extension searches, in order:

1. `newbasic.adapterPath`, if set (supports `${workspaceFolder}`);
2. `bin/Release/newbasic-dap`;
3. `bin/Debug/newbasic-dap`;
4. `build-sdl/src/newbasic-dap/newbasic-dap` (legacy build tree);
5. `build/src/newbasic-dap/newbasic-dap` (legacy build tree);
6. `out/src/newbasic-dap/Release/newbasic-dap.exe` (legacy, Windows);
7. `out/src/newbasic-dap/Debug/newbasic-dap.exe` (legacy, Windows).

Only set `newbasic.adapterPath` if the adapter lives elsewhere, and make it a
platform-correct path. A Linux path in that setting will break the same
workspace on Windows.

---

## 8. Reload and run

1. Command Palette → **Developer: Reload Window**. This is required after
   installing or repackaging the extension.
2. Open `fixtures/m0/hello.bas`.
3. Press **F5** and choose **Run this program** if prompted.
4. Expect a separate window titled `newbasic` showing:

   ```text
   HELLO FROM NEWBASIC
   5  10
   ```

5. Open **Output → newbasic**. A healthy session contains lines like:

   ```text
   newbasic debugger on linux/arm64, VS Code 1.142.0
   registered debug configuration provider and adapter factory
   launch: type=newbasic program=/home/.../fixtures/m0/hello.bas cwd=/home/.../newbasic
   adapter: /home/.../bin/Release/newbasic-dap args=[] cwd=/home/.../newbasic
   ```

6. Set a breakpoint by clicking the **gutter** next to **file line 2**, the
   `20 PRINT 5, 10` statement (or right-click the gutter). The fixture has only
   three physical lines; BASIC line 20 is file line 2, and gutter row 20 does
   not exist. Press F5. The session should stop on BASIC line 20; F10 steps to
   BASIC line 30; the Variables view should show the program state. A hollow
   breakpoint circle means the line was not found in the loaded program.

PRINT output appears in the `newbasic` window, **not** in the VS Code Debug
Console. An empty Debug Console is expected.

For graphics, open `samples/RingWorld.bas`, press F5, and confirm the ringed
planet appears in the `newbasic` window.

---

## 9. Breakpoints

`.bas` files are claimed by VS Code's built-in **Visual Basic** language
(`vb`). The debugger contributes breakpoint support for both `basic` and `vb`,
and the workspace sets `debug.allowBreakpointsEverywhere` as a fallback.

**File line vs. BASIC line.** `fixtures/m0/hello.bas` is only three physical
lines; its BASIC line 20 is file line 2. A breakpoint on gutter row 20 is
outside the file and will not verify, so the program runs to completion without
stopping. Use file line 2 and confirm the breakpoint circle is solid.

- **Left-click the gutter** to toggle a breakpoint.
- **Right-click the gutter** for Add Breakpoint / Conditional Breakpoint /
  Logpoint.
- **Right-click the source text** offers *Add Inline Breakpoint* while a debug
  session is active. That is standard VS Code behavior; the full menu is on the
  gutter.

If the gutter menu is missing:

1. Reload the window; the old manifest may still be loaded.
2. Check `code --list-extensions --show-versions` for
   `newbasic.newbasic-debug@0.1.0`.
3. Check that the file language (status bar) is `Visual Basic` or that
   `debug.allowBreakpointsEverywhere` is true.
4. Inspect **Output → newbasic** for extension errors.

---

## 10. Headless debugging

If there is no display, or a window must not open, set:

```json
{
  "debug.allowBreakpointsEverywhere": true,
  "newbasic.showWindow": false
}
```

The adapter receives `--nowindow` and uses a headless screen. `PRINT` is still
not forwarded to the Debug Console, and `INPUT`/`INKEY$` cannot be used in this
mode. Use headless mode only for stepping and variable inspection.

For `Step Back`/`reverseContinue`, also set `"newbasic.history": true`.
This copies the VM at every line boundary and is off by default.

---

## 11. Troubleshooting

| Symptom | Check | Fix |
|---|---|---|
| `program not found: .../${file}` | Output shows a literal `${file}` | The extension is older than the post-substitution validation. Repackage/reinstall the current `editors/vscode`, then reload the window. |
| `Invalid debug adapter` | **Output → newbasic** and whether a `newbasic-dap` process exists | Usually a missing/non-executable adapter or a launch-time program error. The Output channel now names the path. |
| `newbasic-dap was not found` | Output lists the searched paths | Build `build-sdl/` or `build/`, or set `newbasic.adapterPath` to the real executable. |
| `Permission denied` starting the adapter | `ls -l bin/Release/newbasic-dap` | `chmod +x bin/Release/newbasic-dap`. |
| Session starts, no output window | SDL2/display available? | Install `libsdl2-dev`, check `DISPLAY`/`WAYLAND_DISPLAY`, or use `newbasic.showWindow: false`. PRINT is not in the Debug Console. |
| No breakpoint menu | Window not reloaded; language not `vb` | Reload, confirm extension version, keep `debug.allowBreakpointsEverywhere` true. |
| `code: command not found` | `command -v code` | Install the VS Code CLI or add it to `PATH`; use `/usr/bin/code` explicitly. |
| `npx` cannot download `vsce` | Network/proxy | Use a VSIX produced on another machine; do not vendor `node_modules`. |
| Adapter path has `.exe` on Linux | `newbasic.adapterPath` manually set | Remove the setting or point it at the Linux `newbasic-dap`. |
| Breakpoint circle is hollow; session starts and exits immediately | Gutter row is outside the file or not a loaded program line | Use the file line: `hello.bas` file line 2 is BASIC line 20. Confirm the adapter reports the breakpoint as `verified`. |

---

## 12. Evidence checklist for the agent

Report all of these:

- OS/arch, VS Code version, CMake/compiler/SDL2 versions.
- Configure/build commands and exit codes.
- `test -x` result for the adapter.
- DAP smoke output (`newbasic-dap smoke test passed`).
- CTest summary (7/7, 0 failures).
- `code --list-extensions --show-versions` line for `newbasic`.
- Output → newbasic lines showing the resolved adapter and cwd.
- Fixture result in the `newbasic` window.
- Breakpoint location (file line 2 = BASIC line 20 for the shipped fixture), the
  fact that the session stopped there, and the line F10 stepped to.

---

## 13. Verification record (2026-10-22)

This runbook was executed end to end at commit `d7536e1` on Debian 13 (trixie),
arm64, with VS Code 1.142.0, CMake 3.31.6, g++ 14.2.0, SDL2 2.32.4 and Node
24.22.1. No C++ sources or CMake files were changed for the verification.

| Check | Observed |
| --- | --- |
| Configure `build-sdl` with `NEWBASIC_WINDOW_SDL=ON` | success |
| `test -x build-sdl/src/newbasic-dap/newbasic-dap` (layout at that commit) | `adapter OK` |
| `tests/dap_smoke.cmake` | `newbasic-dap smoke test passed` |
| `ctest --test-dir build-sdl` | 7/7 passed, 0 failed |
| `build-sdl/tests/newbasic-tests` | 385 cases, 31,275 assertions |
| `code --list-extensions --show-versions` | `newbasic.newbasic-debug@0.1.0` |
| Breakpoint on file line 2 (BASIC line 20) | verified; stopped at line 20 |
| F10 / step over | stopped at BASIC line 30 (file line 3) |
| `newbasic` window | `HELLO FROM NEWBASIC`, then `5  10` |

At `d7536e1` the adapter lived under `build-sdl/src/newbasic-dap/`; current
builds put the same executable in `bin/Release/`.

The first attempt used gutter row 20 (the BASIC line number, not the file line).
The adapter returned `verified: false`, and the session ran to completion and
terminated immediately. Moving the breakpoint to file line 2 produced
`verified: true` and the expected stop. This is the most likely failure mode for
older notes that say `line 20`.

The Output → newbasic channel showed:

```text
newbasic debugger on linux/arm64, VS Code 1.142.0
registered debug configuration provider and adapter factory
launch: type=newbasic program=/home/.../fixtures/m0/hello.bas cwd=/home/.../newbasic
adapter: /home/.../newbasic/build-sdl/src/newbasic-dap/newbasic-dap args=[] cwd=/home/.../newbasic
```

### Scripted or headless verification

If no interactive input is available, these techniques drive the same installed
extension and adapter without modifying the repository:

- Load a temporary extension with `--extensionDevelopmentPath`; it can call
  `vscode.debug.startDebugging`, add a `vscode.SourceBreakpoint`, and run
  `workbench.action.debug.stepOver` (the F10 command).
- Register a `vscode.debug.registerDebugAdapterTrackerFactory('newbasic', ...)`
  tracker to record the `setBreakpoints` response (`verified: true`), `stopped`
  events, and the `stackTrace` line for each stop.
- Under Wayland the adapter window's app id is `newbasic-dap` and its title is
  `newbasic`; match the app id when focusing it for screenshots.
- To reveal Output → newbasic without a pointer, attempt a launch with a missing
  `program`. The extension logs `program not found` and calls
  `output.show(true)`, exposing the channel with the earlier session lines. That
  deliberate error is the final line, not part of a healthy session.
