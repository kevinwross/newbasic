# lwbasic debugger for VS Code

A thin extension that lets VS Code debug GW-BASIC programs with the `lwbasic` interpreter. It adds
the `lwbasic` debug type; when a session starts it runs `lwbasic-dap.exe`, which speaks the Debug
Adapter Protocol over stdin/stdout. Breakpoints, stepping, variable inspection and reverse debugging
(`stepBack`, `reverseContinue`) all come from the adapter - see `docs/m5-dap-debugger.md`.

## Before you start

Build the interpreter so the adapter exists:

    cmake --build out --config Release

The extension looks for `out/src/lwbasic-dap/Release/lwbasic-dap.exe` in the first workspace folder.
If your build lives elsewhere, set `lwbasic.adapterPath` to the full path of the executable.

## Running it from source

No build step: the extension is plain JavaScript.

1. Open this folder (`editors/vscode`) in VS Code.
2. Press **F5**. A second VS Code window opens with the extension loaded.
3. In that window, open an `lwbasic` project — the lwbasic repo itself works — and add a
   `.vscode/launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "lwbasic",
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
    code --install-extension lwbasic-debug-0.1.0.vsix

## Settings

| Setting | Meaning |
|---|---|
| `lwbasic.adapterPath` | Full path to `lwbasic-dap.exe`. Empty means "use the workspace's build output". |

## What it does not do

- **No pause button.** The adapter cannot be interrupted mid-run, so `continue` runs to the next
  breakpoint or the end. Set a breakpoint instead.
- A program is a single thread, so there is one thread and one stack frame in the UI.
- Arrays appear as a single variable per array; individual elements are not expanded in the
  variables view (they are available through the adapter's `variables` request for arrays).
