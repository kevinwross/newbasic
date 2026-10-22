# The debugger and the DAP adapter (M5, M7)

Two pieces, deliberately separate:

- **`newbasic-debug`** is the debugger itself: a `Debugger` over a live `Vm`, with no protocol
  concerns, so it can be unit-tested against a headless screen.
- **`newbasic-dap`** is a Debug Adapter Protocol server that drives it. Its protocol layers
  (framing, JSON, dispatch, session) are unit-tested without stdio; the executable is only the
  process boundary, reading DAP requests on stdin and writing responses and events on stdout.

There is **no interactive debugger inside `newbasic.exe`** and no graphical UI. A debug session is a
DAP conversation with `newbasic-dap`.

## What it can do

| Area | Support |
|---|---|
| Breakpoints | Line breakpoints, and conditional ones whose condition is a BASIC expression evaluated in the stopped context with the VM state saved and restored around it |
| Stepping | `next`, `stepIn`, `stepOver` (step to the next source line), plus instruction-level stepping in the library |
| Pause | `pause` interrupts a running `continue` at the next instruction; a reader thread sets a flag that the debugger checks between instructions |
| Time travel | `stepBack` and `reverseContinue` rewind through snapshots recorded at line boundaries in a bounded ring (default 500 entries). Snapshots are opt-in with `--history`; without it the capability is not advertised. |
| Inspection | `stackTrace`, `scopes`, `variables`, `evaluate`, and `setVariable` for scalar Locals; the library also exposes call frames (GOSUB and FOR), arrays and array elements |
| Lifecycle | `initialize`, `launch` (with `programText` or `program`), `attach`, `setBreakpoints`, `configurationDone`, `continue`, `next`/`stepIn`/`stepOver`, `pause`, `stepBack`, `reverseContinue`, `disconnect` |

Variable names carry their type suffix, so a single-precision `A` appears as `A!`.

## A verified session

This ran successfully and stopped at editor line 3 (BASIC line 30) with `A!` = 1 and `B!` = 2:

```powershell
$reqs = @(
  '{"seq":1,"type":"request","command":"initialize"}',
  '{"seq":2,"type":"request","command":"launch","arguments":{"programText":"10 A = 1\n20 B = 2\n30 C = A + B\n40 END\n"}}',
  '{"seq":3,"type":"request","command":"setBreakpoints","arguments":{"breakpoints":[{"line":3}]}}',
  '{"seq":4,"type":"request","command":"configurationDone"}',
  '{"seq":5,"type":"request","command":"variables","arguments":{"variablesReference":1}}'
)
$sb = New-Object System.Text.StringBuilder
foreach ($r in $reqs) {
  $len = [System.Text.Encoding]::UTF8.GetByteCount($r)
  [void]$sb.Append("Content-Length: $len`r`n`r`n$r")
}
[System.IO.File]::WriteAllText("dap_in.txt", $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
Start-Process -FilePath "out\src\newbasic-dap\Release\newbasic-dap.exe" `
  -ArgumentList "--nowindow" `
  -RedirectStandardInput dap_in.txt -RedirectStandardOutput dap_out.txt -PassThru -Wait
Get-Content dap_out.txt
```

`--nowindow` keeps this scripted session headless; remove it to see the GUI output window.

`launch` accepts either a `program` path or inline `programText`. `configurationDone` (or
`continue`) is what starts execution; execution stops at the first breakpoint and a `stopped` event
is emitted.

DAP line numbers are physical editor lines, 1-based, as every DAP client expects. The adapter maps
them to and from BASIC line numbers using the loaded source: in the inline program above, editor
line 3 is BASIC line 30. Breakpoints on blank lines or lines without a BASIC number are reported as
unverified rather than silently landing on a different line.

## The output window

By default `newbasic-dap` opens the interpreter's GUI window (D3D11 on Windows, SDL2 on Linux) and
presents the program's screen as execution proceeds. That makes `PRINT`, `CLS`, `LOCATE`, `SCREEN`,
`PSET`, `LINE`, `CIRCLE`, `PAINT`, `SCREEN 720` and the rest visible while stepping or continuing;
the debugger calls `Screen::PresentIfDirty` after each step and forces a final frame at a stop. The
window is separate from VS Code, which remains the debug UI.

Pass `--nowindow` to use a headless screen instead. This is what CI and other non-interactive clients
should do; it also avoids opening a window when no display exists. `INPUT` and `INKEY$` need the
window, so they are not usable with `--nowindow`. If the window backend is missing or fails to
initialise, the adapter prints one line to stderr and falls back to headless rather than failing the
session.

The DAP Debug Console does not receive `PRINT` output; it belongs to the GUI window. Time travel
restores the VM state but not the screen, so after `stepBack`/`reverseContinue` the window still
shows the later frame.

## Driving it from an editor

**Neovim** with `nvim-dap` can spawn a stdio adapter directly:

```lua
local dap = require("dap")
dap.adapters.newbasic = {
  type = "executable",
  command = "C:/Users/kevin/repos/newbasic/out/src/newbasic-dap/Release/newbasic-dap.exe",
  args = {}, -- add "--nowindow" for a headless session
}
dap.configurations.basic = {
  {
    type = "newbasic",
    request = "launch",
    name = "Run program",
    program = "${file}",
  },
}
```

### The TCP transport

`newbasic-dap --port N [--nowindow]` listens on `127.0.0.1:N` instead of using stdio; `--port 0`
picks a free port and reports it on stderr, which makes the transport scriptable:

    newbasic-dap --port 0
    newbasic-dap: listening on 127.0.0.1:60554

The listener binds the loopback interface only, because a debug port lets whoever connects run and
inspect programs. Each connection gets a fresh session over the same machine, so a client can
reconnect after a reload. Verified over a real socket: connect, then `initialize`, `launch`,
`setBreakpoints`, `configurationDone` produced a `stopped` event at the requested line.

`nvim-dap` can attach to it as a server rather than spawning a process:

```lua
dap.adapters.newbasic = { type = "server", host = "127.0.0.1", port = 4711 }
```

**VS Code still needs an extension.** `--port` removes the *transport* obstacle - a client that
wants a debug server can now have one - but VS Code resolves a `"type"` in `launch.json` through an
installed extension, and `"debugServer"` is a property that extension has to honour. So plain
VS Code without any extension cannot attach to this adapter even over TCP; nvim-dap, vimspector and
Emacs dap-mode can, because they accept a host and port directly. Anyone wanting VS Code support
needs a small extension that registers the type and forwards to the port.

## Limitations

- **Pause latency.** `pause` stops at the next instruction. A blocking statement such as `INPUT` or
  `SHELL` keeps the adapter inside that instruction until it returns, so a pause cannot interrupt it.
- **Time travel is opt-in.** Start the adapter with `--history` (or set `newbasic.history` in the VS
  Code extension) to record snapshots. The default is off because a snapshot copies the entire VM at
  every line boundary.
- **Set Variable is scalar-only.** The Variables view can edit existing scalar Locals. Numbers are
  parsed according to the variable's type suffix (`%`, `&`, `!`, `#`); strings accept both quoted
  and unquoted text. Array elements are read-only.
- **Arrays are expandable in the DAP Variables view.** Each `DIM`'d array appears as one summary
  row showing its bounds and element count, and expands into numbered element rows such as
  `ACCOUNTNUMBER!(0)`. Elements are read-only. Very large arrays are capped at 1,000 displayed
  elements to keep the client responsive.
- **No disassembly or memory requests.** The plan lists both; neither is exposed. Memory is
  consistent with the M13 decision to have no machine model.
- **`terminate` is not supported** (`supportsTerminateRequest` is false); use `disconnect`.
- Thread id is always 1 and the stack frame id 0, since a BASIC program is a single thread.

## Status

All of the above is covered by `test_debugger.cpp` and the four `test_dap_*` unit tests, plus
`tests/dap_smoke.cmake`, which drives the real executable with framed requests so the process
boundary is covered in CI rather than only in-process.
