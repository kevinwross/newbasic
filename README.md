# lwbasic

A terminal interpreter for the **GW-BASIC 3.23 dialect**, written in C++17 for
Windows. Program files load and run, the direct-mode REPL accepts statements,
and an in-process debugger is exposed through a Debug Adapter Protocol server so
an editor can drive it.

## Status

| Milestone | State |
|---|---|
| M0 — skeleton, `Screen` abstraction, CI | ✅ |
| M1 — lexer, expressions, control flow, REPL | ✅ |
| M2 — types, arrays, `DEF FN`, `DATA`/`READ`/`RESTORE` | ✅ |
| M3 — error handling, program files, `SHELL`, sequential file I/O, DOS commands, timer events | ✅ |
| M4 — tokenized `.BAS` read/write incl. protected files | ✅ (byte-fidelity to Microsoft files unproven) |
| M5 — debugger library + `lwbasic-dap` | ✅ |
| M6 — golden corpus, spec-driven conformance | ✅ |
| M7 — D3D11 window backend, sound, time-travel debugging | ✅ (window awaiting a visual check) |

`plan.md` is the specification and the source of truth for decisions, divergences
and open items; `docs/status.md` is a dated snapshot of measured state.

## Build

Requires Visual Studio (C++ workload) and the Windows 11 SDK. CMake comes with
Visual Studio.

```
cmake -S . -B out -G "Visual Studio 18 2026" -A x64
cmake --build out --config Release --parallel
```

## Test

```
ctest --test-dir out --build-config Release --timeout 120 --output-on-failure
```

or, with the hygiene this project needs (stray-process cleanup, captured logs, a
single retry to separate a glitch from a regression):

```
pwsh tools/verify.ps1              # build, test, retry once on failure
pwsh tools/verify.ps1 -NoRetry     # CI-style strictness
```

Some checks are deliberately **not** part of the default run because they make
sound or need a GPU. Run them by hand:

```
out\tests\Release\lwbasic-tests.exe "[.audio]" -s   # plays BEEP and an arpeggio
out\tests\Release\lwbasic-tests.exe "[.gpu]" -s     # opens the D3D11 window
```

## Run

```
lwbasic prog.bas          # run a program in a D3D11 window
lwbasic                   # direct-mode REPL in a window
lwbasic --cli prog.bas    # run in the console instead
lwbasic --gpu-scale 2     # bigger window (16x32 cells instead of 8x16)
```

## Layout

```
src/lwbasic-core     VM, front end, values, screen grid, debugger, DAP protocol
src/lwbasic-screen   console / D3D11 / headless backends, glyph atlas
src/lwbasic-cli      lwbasic.exe
src/lwbasic-dap      lwbasic-dap.exe (Debug Adapter Protocol server over stdio)
tests                unit, property, conformance and integration tests
fixtures             *.bas + *.expected pairs run by the golden-corpus test
docs                 format notes, design notes, status snapshot
third_party          vendored Catch2 v3 and RapidCheck
```

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
