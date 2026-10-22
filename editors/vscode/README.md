# newbasic debugger for VS Code

A thin extension that lets VS Code debug GW-BASIC programs with the `newbasic` interpreter. It adds
the `newbasic` debug type; when a session starts it runs `newbasic-dap` (`newbasic-dap.exe` on
Windows), which speaks the Debug Adapter Protocol over stdin/stdout. Breakpoints, stepping, pause,
variable inspection and reverse debugging (`stepBack`, `reverseContinue`) all come from the adapter -
see `docs/m5-dap-debugger.md`.

## Before you start

Build the interpreter so the adapter exists.

Linux (console-only builds work, but the debug window needs SDL2 and a display):

    cmake -S . -B build-sdl -DCMAKE_BUILD_TYPE=Release -DNEWBASIC_WINDOW_SDL=ON
    cmake --build build-sdl --parallel

Windows (Visual Studio generator):

    cmake -S . -B out -A x64
    cmake --build out --config Release --parallel

The extension searches the first workspace folder for the adapter, in this order:

1. `newbasic.adapterPath`, if set (supports `${workspaceFolder}`);
2. `build-sdl/src/newbasic-dap/newbasic-dap`;
3. `build/src/newbasic-dap/newbasic-dap`;
4. `out/src/newbasic-dap/Release/newbasic-dap.exe`;
5. `out/src/newbasic-dap/Debug/newbasic-dap.exe`.

The Linux binaries have no `.exe` suffix, and this repository's `.vscode/launch.json` is already
configured to run the current `.bas` file. The checked-in workspace settings leave
`newbasic.adapterPath` empty so the same settings file works on Linux and Windows; set it only if the
adapter lives outside these build directories. If it is set, it must match the current platform.

## Running it from source

No build step: the extension is plain JavaScript.

1. Open this folder (`editors/vscode`) in VS Code.
2. Press **F5**. A second VS Code window opens with the extension loaded.
3. In that window, open a `newbasic` project - the newbasic repo itself works - and add a
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

4. Open a `.bas` file, press F5, and the debugger starts.

Set a breakpoint by clicking in the gutter, then step with F10, inspect variables in the left panel,
and use the **Step Back** button to rewind through the recorded history.

## Installing it permanently

    npx @vscode/vsce package
    code --install-extension newbasic-debug-0.1.0.vsix

For development, VS Code also loads an extension straight from this checkout. Link it into the
extensions directory and restart VS Code:

    ln -s "$PWD/editors/vscode" ~/.vscode/extensions/newbasic.newbasic-debug-0.1.0

(The target name must be `publisher.name-version`; this extension's publisher is `newbasic`.)

## Breakpoints

`.bas` files are claimed by VS Code's built-in **Visual Basic** language (`vb`), so the debugger
declares breakpoint support for both `basic` and `vb`. Add a breakpoint by clicking the gutter to the
left of the line number, or right-click **the gutter** for the breakpoint menu (Add Breakpoint,
Conditional Breakpoint, Logpoint). Right-clicking the source text itself offers "Add Inline
Breakpoint" while a debug session is active; that is standard VS Code behaviour.

This workspace also sets `debug.allowBreakpointsEverywhere`, so breakpoints work even if a `.bas`
file is associated with a different language.

## If a session does not start

Open **Output → newbasic**. The extension logs the resolved adapter path, the
launch configuration and the working directory there, so a failed session is not
just an opaque "Invalid debug adapter" message.

Before starting, the extension checks that the program exists and that the
adapter is executable. If the active buffer is unsaved (or is an untitled
`${file}`), it launches that buffer as `programText` instead of the stale path
on disk; it also saves a dirty file before launching. A missing program shows a
clear error naming the path.

If the adapter is not executable, fix it with:

    chmod +x build-sdl/src/newbasic-dap/newbasic-dap

## Settings

| Setting | Meaning |
|---|---|
| `newbasic.adapterPath` | Path to `newbasic-dap` (`newbasic-dap.exe` on Windows). Empty means "search the documented build outputs". |
| `newbasic.showWindow` | Show the program's GUI window during debug sessions (default `true`). Set `false` to pass `--nowindow` and run headless. |
| `newbasic.history` | Record VM snapshots for `stepBack`/`reverseContinue` (default `false`). Snapshots cost a full VM copy per line; enable only when you need reverse debugging. |

## Program output

By default the debug session opens a separate GUI window and shows the program's text and graphics
there as execution proceeds. This is the same `Screen` the normal runner uses (SDL2 on Linux,
D3D11 on Windows), so `PRINT`, `CLS`, `LOCATE`, `SCREEN`, `PSET`, `LINE`, `CIRCLE`, `PAINT` and
`SCREEN 720` all appear in that window while stepping or continuing.

`PRINT` output is **not** forwarded to VS Code's Debug Console. Set `newbasic.showWindow` to `false`
to run headless; `INPUT`/`INKEY$` are not usable in that mode. On Linux a window requires SDL2 and a
display; if either is unavailable the adapter falls back to a headless screen.

The adapter runs with the active workspace folder as its working directory, so relative program paths
such as `samples\accounting.dat` resolve from the repository root, not from the adapter executable's
folder. When there is no active editor it uses the first workspace folder, then the adapter directory.

Rewinding with `stepBack`/`reverseContinue` restores variables and the program counter, but not the
screen contents, so the window keeps showing the later frame after a rewind. Reverse debugging is
opt-in: set `newbasic.history` to `true`, because snapshots cost a full VM copy per line.

## Editing variables

While stopped, double-click an existing scalar variable in the **Variables** view, type the new
value, and press Enter. Numeric values use the variable's type suffix; strings accept quoted or
unquoted text. Arrays are not expandable or editable.

## What it does not do

- **Pause cannot interrupt a blocking statement.** While a program is running the toolbar's main
  button becomes Pause, and clicking it stops at the next instruction. `INPUT`, `SHELL` and other
  synchronous host calls keep the adapter inside that instruction until they return.
- A program is a single thread, so there is one thread and one stack frame in the UI.
- Arrays appear as summary variables such as `ARRAY(0..20)  21 elements` and expand to show
  individual elements. Elements are read-only; only scalar Locals can be edited.
