# newbasic debugger for VS Code

A thin extension that lets VS Code debug GW-BASIC programs with the `newbasic` interpreter. It adds
the `newbasic` debug type; when a session starts it runs `newbasic-dap.exe`, which speaks the Debug
Adapter Protocol over stdin/stdout. Breakpoints, stepping, pause, variable inspection and reverse
debugging (`stepBack`, `reverseContinue`) all come from the adapter - see `docs/m5-dap-debugger.md`.

## Before you start

Build the interpreter so the adapter exists:

    cmake --build out --config Release

The extension looks for `out/src/newbasic-dap/Release/newbasic-dap.exe` in the first workspace folder.
If your build lives elsewhere, set `newbasic.adapterPath` to the full path of the executable.

## Running it from source

No build step: the extension is plain JavaScript.

1. Open this folder (`editors/vscode`) in VS Code.
2. Press **F5**. A second VS Code window opens with the extension loaded.
3. In that window, open a `newbasic` project — the newbasic repo itself works — and add a
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

    npm install -g @vscode/vsce
    cd editors/vscode
    vsce package
    code --install-extension newbasic-debug-0.1.0.vsix

## Settings

| Setting | Meaning |
|---|---|
| `newbasic.adapterPath` | Full path to `newbasic-dap.exe`. Empty means "use the workspace's build output". |
| `newbasic.showWindow` | Show the program's GUI window during debug sessions (default `true`). Set `false` to pass `--nowindow` and run headless. |
| `newbasic.history` | Record VM snapshots for `stepBack`/`reverseContinue` (default `false`). Snapshots cost a full VM copy per line; enable only when you need reverse debugging. |

## Program output

By default the debug session opens a separate GUI window and shows the program's text and graphics
there as execution proceeds. This is the same `Screen` the normal runner uses (D3D11 on Windows,
SDL2 on Linux), so `PRINT`, `CLS`, `LOCATE`, `SCREEN`, `PSET`, `LINE`, `CIRCLE`, `PAINT` and
`SCREEN 720` all appear in that window while stepping or continuing.

`PRINT` output is **not** forwarded to VS Code's Debug Console. Set `newbasic.showWindow` to `false`
to run headless; `INPUT`/`INKEY$` are not usable in that mode. On Linux a window requires SDL2 and a
display; if either is unavailable the adapter falls back to a headless screen.

Rewinding with `stepBack`/`reverseContinue` restores variables and the program counter, but not the
screen contents, so the window keeps showing the later frame after a rewind. Reverse debugging is
opt-in: set `newbasic.history` to `true`, because snapshots cost a full VM copy per line.

## What it does not do

- **Pause cannot interrupt a blocking statement.** While a program is running the toolbar's main
  button becomes Pause, and clicking it stops at the next instruction. `INPUT`, `SHELL` and other
  synchronous host calls keep the adapter inside that instruction until they return.
- A program is a single thread, so there is one thread and one stack frame in the UI.
- Arrays appear as a single variable per array; individual elements are not expanded in the
  variables view (they are available through the adapter's `variables` request for arrays).
