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
| `baslib: init, constants, and labels` | M1 constants, init, API labels |
| `baslib: create, layout, paint, hit-test, and click event` | M3/M5/M6/M7/M8 |
| `baslib: nested layout uses per-call locals` | M5/M6 (nested-panel regression) |
| `baslib: 255-control ceiling` | M3 |
| `baslib: action-line dispatch reaches host code` | M4/M9 |
| `baslib: destroy, re-init, layout, and host sentinel` | M3/M5/M10 |
| `baslib: root uses the current canvas size` | M1/M10 |
| `baslib: v0.2 value controls` | M11 |
| `baslib: slider drag updates its value` | M11 |
| `baslib: dynamic class registration` | M11 |
| `baslib: paint preserves the host viewport` | M6/M10 |
| `baslib: demo.bas compiles` | M9 |
| `baslib: demo-controls.bas compiles` | M9/M11 |
| `baslib: labels survive RENUM` | M10 |
| `baslib: ERL reports a 500000+ library line` | M10/B10-09 |
| `baslib: namespace, suffix, and API-label audit` | M10 |
| `baslib: source stays inside its line block` | M1/M10 |

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
