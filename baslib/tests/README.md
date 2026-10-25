# baslib tests

The executable baslib harness is integrated into the repository's Catch2 test
binary rather than a separate executable. It loads `baslib/baslib.bas`, merges
it with a host fragment, and runs the result on a `HeadlessScreen`.

## Commands

```text
# Windows
cmake --build out --config Release --target newbasic-tests
out/tests/Release/newbasic-tests.exe "[baslib]"

# Linux / WSL
cmake --build build-wsl --target newbasic-tests -j4
build-wsl/tests/newbasic-tests "[baslib]"
```

The full suite remains:

```text
out/tests/Release/newbasic-tests.exe
build-wsl/tests/newbasic-tests
```

CLI fixtures merge each `fixtures/*.bas` with `baslib.bas`, run it with
`--gpu-shot`, and compare `result.txt` against `expected/`:

```powershell
.\baslib\tests\run_cli.ps1
.\baslib\tests\run_cli.ps1 -Newbasic .\out-clean\src\newbasic-cli\Release\newbasic.exe
```

```bash
bash baslib/tests/run_cli.sh
NEWBASIC=/path/to/newbasic bash baslib/tests/run_cli.sh
```

## Coverage map

| Test | Covers |
|---|---|
| `baslib: init, constants, and public procedures` | constants, init, parameterized API |
| `baslib: create, layout, paint, hit-test, and click event` | core API |
| `baslib: bounds, text, action, visibility, and enabled setters` | setters |
| `baslib: nested layout uses per-call locals` | nested-panel regression |
| `baslib: 255-control ceiling` | handle table |
| `baslib: action-line dispatch reaches host code` | action lines |
| `baslib: destroy, re-init, layout, and host STACK` | destroy and re-init |
| `baslib: root uses the current canvas size` | root sizing |
| `baslib: value controls use parameterized getters and setters` | values |
| `baslib: slider drag updates its value` | pointer values |
| `baslib: hover, press, release, and wheel events` | event queue |
| `baslib: dynamic class registration` | metadata |
| `baslib: blUpdateAll repaints only dirty subtrees` | dirty-subtree painting |
| `baslib: paint preserves the host viewport` | viewport preservation |
| `baslib: demo.bas compiles` | demo |
| `baslib: demo-controls.bas compiles` | controls demo |
| `baslib: samplehost.bas compiles` | full host sample |
| `baslib: public procedure names survive RENUM` | RENUM |
| `baslib: ERL reports a 500000+ library line` | ERL |
| `baslib: namespace, suffix, and no request-global audit` | API/style audit |
| `baslib: source stays inside its line block` | line block |

Tests are headless and deterministic: no window, GPU, timers, or real mouse are
required. `RunHost` in `tests/test_baslib.cpp` is the shared fixture loader.

CLI fixtures:

| Fixture | Covers |
|---|---|
| `api_smoke` | init, create, paint, find, click, event |
| `layout_paint` | panel layout, child placement, pixel probe |
| `stack_sentinel` | host `STACK` survives init/paint |

## Manual demos

Basic click/action demo:

```text
LOAD "baslib/demo.bas"
MERGE "baslib/baslib.bas"
RUN
```

All-control gallery:

```text
LOAD "baslib/demo-controls.bas"
MERGE "baslib/baslib.bas"
RUN
```

Both demos are host fragments with lines below 500000; merging `baslib.bas`
adds the library lines at 500000+. Use `Esc` to exit.

The gallery shows a panel, button, labels, click target, slider, gauge, and
dial. Click the slider or dial at a position to change its value; the readout
updates. The gauge is read-only. The button and invisible click target write to
the readout and demonstrate action dispatch.
