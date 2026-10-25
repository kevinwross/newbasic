> **2026-10-24:** Tokenized and protected program-file support was removed. `SAVE`/`LOAD`/`MERGE` are plain-text only. Historical notes below are retained for provenance.

# newbasic — Project Plan

> **Provenance note.** This is the upstream **lwbasic** implementation plan, retained
> for reference. The product and path names below have been updated to **newbasic**;
> the upstream project is https://github.com/kevinwross/lwbasic.

A terminal BASIC interpreter targeting the **GW-BASIC dialect**, with an
in-process debugger library, a Debug Adapter Protocol (DAP) server, and a
display abstraction designed to grow into a graphics engine later.
**Windows-only** (Windows 10 1809+; both legacy conhost and Windows
Terminal as hosts).

Status legend: ✅ done · 🚧 in progress · ⏳ planned

---

## Decisions (agreed)

| Area | Decision |
|---|---|
| Platform | Windows only (Win32 console APIs; conhost and Windows Terminal); no Linux/macOS targets |
| Language / toolchain | C++ (C++17 or later) built with **MSVC** via Visual Studio; CMake as the build frontend |
| Dialect | GW-BASIC 3.23 behavior; "like" fidelity — host `double` math with GW-BASIC-style `PRINT` formatting. Floating-point arithmetic and math functions use the host C++ runtime (MSVC CRT) **as-is — no emulation** of GW-BASIC floating-point semantics or quirks; only display formatting emulates GW-BASIC |
| Core | Bytecode VM (line-numbered) |
| Debugger | In-process **library API** + **DAP server** adapter |
| v1 coverage | Everything text-mode; **sound is implemented** (M7: `BEEP`/`SOUND`/`PLAY` through `Host::PlayTone`, silent in tests); graphics statements parse but report "not implemented" |
| Display | Full-screen **ScreenDevice abstraction** (80×25 text grid + cursor); **Win32 console API** backend now (`WriteConsoleW`, `SetConsoleMode`, `ReadConsoleW`), GPU window later (Win32 + D3D11) |
| Encoding | Console output in UTF-16 (`WriteConsoleW`); **CP437→Unicode glyph mapping** for GW-BASIC characters (no `SetConsoleOutputCP`); plain-text `.BAS` import/export preserves CP437 byte values, accepting LF and CRLF |
| CLI | `newbasic.exe prog.bas` runner + direct-mode REPL (no full-screen editor) |
| Binary files | Tokenized `.BAS` read **and** write (incl. protected decode) |
| `SAVE` output | `SAVE` / `SAVE "name.bas"` writes a **plain-text** `.BAS` (numbered lines, CRLF, CP437 bytes preserved) so files stay editable in a simple text editor; `SAVE BIN "name.bas"` writes the tokenized GW-BASIC binary. `LOAD`/`RUN` auto-detect text vs. tokenized by the leading version byte |
| DOS commands | GW-BASIC file I/O via native Win32/C++ `std::filesystem`; `SHELL` launches `cmd.exe /c …` as a child process (`CreateProcessW`), VM suspended while it runs |
| Testing | **Catch2 v3** unit + **RapidCheck** property tests, vendored under `third_party/` (no submodules/network); **spec-driven** (User's Guide) + **frozen golden fixtures** (hand-verified); GitHub Actions CI on `windows-latest` — no external processes in CI |
| Time-travel | Deferred; VM designed for cheap snapshots, rewind implemented later |
| Layout | CMake build (Visual Studio generator), binary name `newbasic.exe` |
| VCS | Git repo, remote `https://github.com/kevinwross/lwbasic.git`; CI via GitHub Actions |
| Reference | GW-BASIC User's Guide + User's Reference (MS 1986/87) text dump: full file at `GWBASIC.txt` and chunked verbatim copies under `docs/gw-basic/` are **committed** — Microsoft's revision places GW-BASIC and derived works under MIT, so they are no longer copyright-encumbered; summary index at `docs/README.md` |

**Intentional divergences from GW-BASIC** (documented alongside fixtures; tests
assert the divergent behavior, not the User's Guide):

- `SAVE` writes a **plain-text** `.BAS` by default instead of a tokenized
  binary (see the `SAVE` output decision above). Tokenized output remains
  available via `SAVE BIN`, and protected encoding stays a codec-level option
  in `newbasic-fstok` (API/CLI), not BASIC syntax.
- **Randomness**: `RND` uses the host generator (`std::mt19937`), not
  GW-BASIC's LCG, so the value *sequence* differs; it is still reproducible
  for a given `RANDOMIZE` seed inside newbasic. `RANDOMIZE` without an
  expression seeds from the host clock instead of prompting interactively.
- **Not-yet-implemented statements** from later milestones (arrays/`DIM`,
  `DEF FN`, `DATA`/`READ`, file I/O, `PRINT USING`/`WRITE`, graphics, sound)
  parse and raise "Advanced Feature" at run time naming the statement, rather
  than being syntax errors, so the parser stays dialect-complete.
- **Error display** is `?<message>[ in <line>]` (the guide does not specify
  the exact format; inferred from GW-BASIC behavior).
- Single-precision values are stored as host `double` and only *displayed*
  with 7 significant digits (per the "no floating-point emulation" decision).

---

## 1. Architecture / crate layout

```
newbasic/
  CMakeLists.txt            # build (CMake + MSVC)
  src/
    newbasic-core/            # values, lexer, parser, bytecode compiler, VM, errors
    newbasic-screen/          # ScreenDevice impls: Win32 console + headless
    newbasic-fstok/           # tokenized .BAS codec (read/write, protected)
    newbasic-debug/           # debugger library: breakpoints, stepping, eval, stack, snapshots
    newbasic-dap/             # Debug Adapter Protocol server over newbasic-debug
    newbasic-cli/             # `newbasic.exe` binary: file runner + direct-mode REPL
  third_party/               # vendored Catch2 v3 + RapidCheck (no submodules)
  fixtures/                  # *.bas + expected output
  tests/                     # Catch2 unit/property + golden fixture tests (CTest)
  docs/                      # chunked GW-BASIC reference (MIT) + format notes
  .github/workflows/         # CI (windows-latest)
  .clang-format              # pinned-format config (CI check)
```

Key seams:

- **`Screen` interface** (in `newbasic-core::screen`) — text grid, cursor,
  `CLS`, `LOCATE`, `COLOR`, key input, line input. The single point where
  text, graphics, and input attach. `newbasic-screen` provides
  `ConsoleScreen` (Win32 console) and `HeadlessScreen` (deterministic grid
  capture for tests). `ConsoleScreen`:
  - Full-screen mode via `SetConsoleScreenBufferSize`/`SetConsoleWindowInfo`
    (buffer ≥ 80×25, clamped to the actual console size), cursor shown only
    at the active cell (`SetConsoleCursorPosition`, `SetConsoleCursorInfo`).
  - Key input via `ReadConsoleW` in cooked/raw mode as needed; arrow keys
    mapped to GW-BASIC `INKEY$` arrow codes; `Ctrl+C` maps to the
    GW-BASIC interrupt (break/`STOP` semantics) rather than killing the
    process, using `SetConsoleCtrlHandler`.
  - Output via `WriteConsoleW`; CP437 source bytes translated through a
    static 437→Unicode code-point table (box-drawing, accented letters).
  - Resize handling: watch for buffer-size changes between paints; clamp the
    grid and re-flow rather than corrupting the screen.
- **`Host` interface** (planned) — clock, randomness, key input,
  filesystem, child processes. Real impl for the CLI; scripted/virtual impl
  for tests. `SHELL` routes through the Host so tests can script `cmd.exe`
  invocations.

---

## 2. Front-end (`newbasic-core`)

- Physical lines → line number + statements; raw source retained for `LIST`.
  Accepts LF and CRLF in plain-text `.BAS` input.
- Lexer: keywords, operators, identifiers with `%! #&$` suffixes, numeric
  literals (`&H`/`&O`, `E`/`D` exponents), strings, `REM`/`'`, `DATA` raw.
- Statement grammar: `LET`, `PRINT`, `INPUT`/`LINE INPUT`, `IF/THEN/ELSE`,
  `FOR/NEXT`, `WHILE/WEND`, `GOTO`/`GOSUB`/`RETURN`/`ON … GOTO|GOSUB`,
  `END`/`STOP`/`REM`, `DIM`/`OPTION BASE`, `DATA`/`READ`/`RESTORE`,
  `ON ERROR`/`RESUME`, event trapping, file I/O + DOS commands, screen ops.
- Program store: ordered map line-number → raw source; direct-mode statements
  execute immediately in the same VM.

## 3. Bytecode VM (`newbasic-core`)

- Stack-based instruction set (`Instr`), with a **line table** (line number →
  pc range) that powers breakpoints, source stepping, and error reporting.
- Stacks kept in `std::vector`s so VM state is cheap to snapshot (prerequisite for
  time-travel debugging).
- Values: `Int(int16_t)`, `Long(int32_t)`, `Sgl(float)`, `Dbl(double)`, `Str(std::string)`; math is `double`.
- **Number formatting** module implementing GW-BASIC-style `PRINT` output.
- Error model: GW-BASIC error-code table + messages.

## 4. Tokenized `.BAS` codec (`newbasic-fstok`)

Decode/encode the binary format (next-line offsets, token bytes, hex/octal,
numeric encodings) and the protected decode/encode algorithm. The tokenized
format is byte-exact and platform-neutral; no CRLF concerns apply to it.

The **text `.BAS` format** (the `SAVE` default) is the raw-source line store
written in ascending line order, one `<line number><tab><raw source>` per
line, with CRLF line endings and CP437 source bytes preserved. `LOAD`/`RUN`
auto-detect the format: a leading version byte (`0x14` for GW-BASIC 3.23)
selects the tokenized codec; anything else is parsed as plain text.

## 5. Debugger (`newbasic-debug` + `newbasic-dap`)

Library API: breakpoints (line + conditional), stepping (into/over/out,
instruction-level), variable/array inspection, expression evaluation, call
stack (GOSUB + FOR frames), snapshots and time travel (`stepBack`,
`reverseContinue`). Memory read/patch and bytecode disassembly are listed here
but **not exposed**; memory is consistent with the M13 decision to have no
machine model.
`newbasic-dap` maps these onto the DAP subset used by VS Code / Neovim / Emacs.
It is a **separate executable speaking DAP over stdio** — it is not built into
`newbasic.exe`, and there is no named-pipe or TCP transport, so VS Code needs an
extension or a TCP bridge while Neovim's `nvim-dap` can spawn it directly.
See `docs/m5-dap-debugger.md` for a verified session and an editor config.

---

## 6. Testing strategy

All verification is **self-contained**: no external interpreters, emulator
processes, or containers are run in CI.

1. **Unit tests** — lexer, parser, formatter, VM ops, codec round-trips.
2. **Property-based tests** — VM arithmetic identities, `FORMAT$`/number
   formatting invariants, `.BAS` codec decode→encode byte-exact round-trips,
   line-store ordering invariants.
3. **Spec-driven tests** — each statement and example from the GW-BASIC
   User's Guide turned into a test with expected behavior; intentional
   divergences (e.g., `SAVE` writing plain text) are specified in this plan
   and tested against it, not the User's Guide.
4. **Frozen golden fixtures** — `fixtures/**/*.bas` + committed expected
   output, deterministic `Host` (scripted console + filesystem + `SHELL`
   responses). Expected outputs are **verified once by hand** against the
   User's Guide / hand-computed values; CI then diffs only against the
   committed files, so the corpus doubles as a stable, dependency-free
   conformance set.
5. **CI** (GitHub Actions, `windows-latest`) — `clang-format` (standalone,
   version-pinned) check, MSVC **`/W4 /WX`** warnings-as-errors, CTest run.
6. *(later)* Fuzzing of the lexer/parser and the `.BAS` decoder via
   Visual Studio native fuzzing (or AFL++).

---

## 7. Milestones

| # | Deliverable | Status |
|---|---|---|
| M0 | Workspace skeleton, `Screen` interface + Win32 console/headless backends, `newbasic.exe file.bas` runs trivial `PRINT`, CI | ✅ |
| M1 | Lexer/parser/line store, direct-mode REPL, `LIST`/`NEW`/`RUN`, expressions, core control flow | ✅ |
| M2 | Types, arrays (`DIM`/`OPTION BASE`/`DEF*`), strings, `DEF FN`, `DATA`/`READ`/`RESTORE` | ✅ |
| M3 | File I/O (native; `SAVE` defaults to plain text, `SAVE BIN` tokenized) + `SHELL` via `cmd.exe`, error handling (`ON ERROR`/`RESUME`), event trapping | ✅ |
| M4 | Tokenized `.BAS` read + write (incl. protected) | ✅ (protected implemented; byte-fidelity to Microsoft files unproven — see `docs/m4-protected-files.md`) |
| M5 | `newbasic-debug` library + `newbasic-dap` | ✅ (the debugger and the DAP protocol headers now live in their own `newbasic-debug` target, as this row always intended) |
| M6 | Golden corpus expansion, spec-driven coverage, conformance hardening | ✅ (corpus is a living set; add pairs as needed) |
| M7 | Win32 + D3D11 screen backend, sound (`Beep`/WinMM), time-travel debugging | ✅ (window implementation verified headlessly; visual check is the user's) |
| M8 | Text-mode language completeness: `PRINT USING`, `WIDTH`, `VIEW PRINT`, `CLS` variants | ✅ (using fields `**`/`$$`/`**$`/`^^^^`/`_` implemented and tested; the `KEY` family is deliberately unsupported — see below) |
| M9 | Random-access files: `FIELD`, `LSET`/`RSET`, `GET #`/`PUT #`, `LOF`/`LOC`, `MKI$`/`MKS$`/`MKD$`, `CVI`/`CVS`/`CVD` | ✅ (FIELD widths must be constants; `GET` on an unwritten record zero-fills) |
| M10 | Program editing and chaining: `AUTO`/`EDIT`/`RENUM`/`MERGE`/`CHAIN`/`COMMON`/`CONT`/`TRON`/`TROFF`, and in-program `RUN`/`LIST`/`NEW`/`SAVE`/`LOAD` | ✅ (`AUTO`, `MERGE`, `RENUM` and `LLIST` are implemented: `RENUM` rewrites references after ELSE/GOTO/GOSUB/THEN/ON…GOTO/ON…GOSUB/RESTORE/RESUME, skipping strings and comments, refusing reordering and reporting undefined references. `EDIT` reports Illegal direct by design — a screen editor was deliberately not attempted. In-program `SAVE`/`LOAD` are a superset of GW-BASIC's direct-mode rule) |
| M11 | Event trapping beyond the timer: `ON PLAY(n)`, `ON COM(n)`, `ON STRIG(n)`, note queue | ✅ (`ON TIMER` works; the `KEY`/`ON KEY` family is deliberately unsupported, and `ON PLAY`/`COM`/`STRIG`/`PEN` and `PLAY ON/OFF/STOP` report Advanced Feature — no note queue, COM port, joystick or light pen) |
| M12 | Graphics (`SCREEN` modes, `PSET`/`LINE`/`CIRCLE`/`PAINT`/`DRAW`, `POINT`/`VIEW`/`WINDOW`/`PALETTE`/`PALETTE USING`, graphics `GET`/`PUT`) | ✅ except window presentation: canvas, `SCREEN 0/1/2/9`, `PSET`/`PRESET`/`POINT`, `LINE` (boxes, `BF`, omitted first point, `STEP`, style masks), `CIRCLE`, `PAINT`, `DRAW` (full GML), `VIEW`, `WINDOW`, `PMAP`, `PALETTE`, `PALETTE USING`, `COLOR ,n`, `GET`/`PUT` images with five actions, `PCOPY` pages. Verified by tests plus GPU pixel readback, and confirmed by eye on the maintainer's machine (see §D) |
| M13 | Machine-level statements (`PEEK`/`POKE`/`INP`/`OUT`/`WAIT`/`USR`/`VARPTR`/`FRE`) | ✅ decision taken: **refuse clearly** rather than invent a 64K memory/port model that corresponds to nothing on the host. Statements already reported Advanced Feature; the functions (`PEEK`/`FRE`/`VARPTR`/`INP`/`POINT`/`STICK`/`STRIG`/`USR`) were syntax errors and now report Advanced Feature naming the function. `CALL` is no longer machine-level; it calls a BASIC `SUB` (see §C). |

**Status detail (kept here so the state survives between sessions).**

- **M3** — error handling (`ON ERROR GOTO`/`RESUME` at statement granularity),
  plain-text program files, `SHELL`, sequential file I/O both directions
  (`OPEN`/`CLOSE`/`PRINT #`/`WRITE #`/`INPUT #`/`LINE INPUT #`/`EOF`) and the DOS
  commands (`FILES`/`KILL`/`NAME`/`MKDIR`/`CHDIR`/`RMDIR`) are done and tested.
  **Timer events** (`ON TIMER(n) GOSUB`, `TIMER ON|OFF|STOP`) are done and
  tested. Root cause of the long detour, kept because it is a useful lesson:

  > The event poll recorded its own progress flag *inside the poll*
  > (`if (!in_event_) main_step_done_ = true;`), and that flag is already true
  > immediately after a handler's `RETURN`. So the guard was satisfied by the
  > poll instead of by program progress: fire → handler → `RETURN` → poll sees
  > the flag → fires again, and the interrupted statement never ran.

  That one defect produced three identical unbounded loops (rounds 8, 10, 40)
  and survived two plausible-sounding "fixes", because both were reasoned from
  intent ("this should guarantee progress") rather than from reading the code.
  The fix records progress where an instruction actually executes —
  `if (!event_instruction) main_step_done_ = true;` in `Vm::Run`'s loop — so an
  event needs at least one real instruction in between and firing is bounded by
  the program's own length. Verified by a scripted-clock test measuring
  `ticks=24` with `TICK` on screen, not by inspection.

  Two things made the difference and are worth repeating: `Vm::TimerDebug()`
  made the poll's runtime state visible, and the decisive experiment printed
  values *from the poll site* rather than from a path that never reached it.
- **M4** — text format (property-tested), format auto-detection (`0xFF`
  tokenized, `0xFE` protected), the token table, the MBF numeric codec, the
  decoder, the encoder, `LOAD` for all three formats, and `SAVE "file",B` /
  `SAVE "file",P` are done and tested (including RapidCheck properties). The
  protected format is implemented (Kocher transform, `fstok/protect.h`); its
  byte-level fidelity against Microsoft-authored files is unproven because no
  such file is available — see `docs/m4-protected-files.md`.
- **M5** — the in-process debugger (breakpoints incl. conditional, stepping,
  variables, call frames, arrays, snapshots) and the DAP server (framing, JSON,
  dispatcher, session loop, `newbasic-dap.exe`) are done and tested, including a
  CTest that exercises the executable's stdin/stdout boundary. The plan places
  the debugger in its own `newbasic-debug` library; it currently lives in
  `newbasic-core` (recorded in `debugger.h`) and moves when convenient.


---

## 7a. Open items

Nothing *broken* is outstanding. Closed items worth knowing are listed below, and
**section 7b records the unfinished work** (deliberate refusals, engineering debt and
decisions only the maintainer can make).

Recently closed, worth knowing:

- **`Step()` executed past a halt (fixed).** Found by converting the timer
  diagnostic into assertions: after `END`, calling `Step()` again resumed at the
  instruction *after* the halt, so a debugger single-stepping past the end ran
  whatever code physically followed the main program — in the test case, the
  `ON TIMER` handler body, which printed `TICK` and looked deceptively like
  "`ON TIMER` runs its handler at registration". `Vm::Step()` now latches a
  `halted_` state and keeps returning false; a *trapped* error does not latch
  (it returns false but `Run()` resumes at the handler); `Run()` and `SetPc()`
  re-arm, and the flag is part of the debugger snapshot so a rewind resumes
  correctly.
- **M3 timer notes** — condensed to the root cause and its lesson (above).
- **Diagnostic scaffolding** — kept deliberately (`Vm::TimerDebug()`), and the
  `[diag]` test is now a real assertion test rather than an always-passing
  print. It is what caught the `Step()` bug.
- **Reference material** — Microsoft's revision puts GW-BASIC and derived works
  under MIT, so `GWBASIC.txt` and `docs/gw-basic/` are committed instead of
  ignored.

Protected-file encryption remains out of scope as a conformance requirement
(`SAVE ,B` and `,P` stay as implemented).

## 7b. Unfinished work (as of M11)

Nothing here produces a silently wrong result: every refused statement names its
reason at run time. The list exists so the boundary is written down rather than
remembered.

### A. Statements that report Advanced Feature, and what each needs

| Statement | Why it is refused | What implementing it needs |
|---|---|---|
| `RENUM` | ~~Line references must be rewritten~~ **done**: a direct-mode command now, rewriting references after ELSE/GOTO/GOSUB/THEN/ON…GOTO/ON…GOSUB/RESTORE/RESUME, skipping strings and comments, refusing reordering, and reporting undefined references | — |
| `MERGE` | ~~Direct-mode only, and the REPL lacked it~~ **done**: merges a file's lines into the store, replacing matching numbers | — |
| `LLIST` | ~~No printer~~ **done**: `Host::WriteToPrinter` is a printer sink (a file in `SystemHost`, a recorder in `ScriptedHost`), and both `LLIST` and `LPRINT` use it, including `LPRINT USING` | — |
| `AUTO` | ~~Direct-mode editor that the REPL lacked~~ **done**: numbered entry with start/increment, ended by an empty line or a line carrying its own number | — |
| `EDIT` | **Deferred by the maintainer, 2026-10-17: not now, but worth doing later.** `EDIT n` is the doorway into GW-BASIC's screen editor: it puts a stored line back on screen, and the editor provides cursor movement, Backspace/Del/Ins, and single-letter dot commands (`nSPACE`, `nD`, `nC`, `nI`, `nX`, `H`, `nS`, `nK`, `L`, `A`, `E`, `Q`). `EDIT` reporting Illegal direct is correct — GW-BASIC also refuses it in a program — what is missing is the direct-mode editor. | Two stages, each landed and tested on its own. **First** the direct cursor-editing core: a seeded `EditLine` variant over `KeySource` (which already exists and is scriptable, so it is testable headlessly), giving cursor movement, delete, insert mode and Enter/Esc over an existing line. **Second** the dot commands as a state machine on top, with `ScriptedKeys` driving each command as an assertion. Order matters: the cursor core is what makes the editor useful; `EDIT` is only its entry point. |
| `ON COM(n)`, `ON STRIG(n)`, `ON PEN`, `PLAY ON/OFF/STOP` | **Deferred by the maintainer, 2026-10-17: not now, but may be done later.** All four are event traps whose event source does not exist here: a COM port, a joystick, a light pen, and a note queue. The trap machinery itself works — `ON TIMER` proves that — so what is missing is a device, not plumbing. | A device seam in `Host` (the existing precedent is keys, mouse clicks, tones and printer output), plus a scripted implementation so tests can drive it. Then `STRIG`/`STICK` latch-vs-current reads, `PEN(n)`, and the `PLAY` note queue on top of `PlayTone`, each routed through the existing trap dispatch. A refusal is honest here: a stub that always reports "not pressed" would silently mislead a program. |
| `PRINT USING` `**`, `$$`, `**$`, `^^^^` | ~~Rules documented but examples OCR-damaged~~ **done**: implemented from the rules, with each damaged example's interpretation recorded in `tests/test_using.cpp` (asterisk fill covers the sign column and padding; a negative with `$$` puts the minus before the dollar; the exponent is always signed with two digits) | — |
| `CHAIN MERGE`, `CHAIN …,ALL`, `COMMON` of arrays | **Deferred by the maintainer, 2026-10-17: not now, but may be done later.** `COMMON` passes named scalars across `CHAIN` today. `COMMON D()` is reported as Advanced Feature — "COMMON of arrays" — so it is refused, not silently dropped (I claimed otherwise from reading the chain carrier without checking the compiler, which already rejected it). `CHAIN …,ALL` would pass everything and is the cheapest of the three; `CHAIN MERGE` (with `DELETE range`) is a larger piece because it changes what "load" means. | Let a `COMMON` entry be an array: snapshot its dimensions, `OPTION BASE` and element data before loading, and re-create it afterwards, letting the carried shape win if the chained program also `DIM`s it. Tests are cheap and headless: two temp programs, the first filling `D(1..3)` and chaining, the second printing them. The refusal is already tested, so lifting it is a matter of carrying data, not adding diagnostics. |
| `CLEAR` as a program statement | ~~Direct mode only~~ **done**: the compiler emits `Op::Clear` and `Vm` runs `ClearState()` — variables, arrays, stacks, open files, the DATA pointer and print state — then continues at the next instruction. The argument form is refused as Advanced Feature. Tested: variables, arrays and strings are emptied mid-program, and an array written after CLEAR does not touch the old one | — |
| `SWAP` of array elements | ~~Found 2026-10-17 by testing~~ **done**: both sides may be scalars or array elements, in any combination; subscript counts travel with the instruction, both slots are resolved before either value moves, and the value is converted for its destination. Tested: two elements, a scalar and an element, 2D elements with expression subscripts, and the plain scalar form. | — |
| `LLIST` with a range | **Still open**, found 2026-10-17 by testing: `LLIST 20-30` prints the whole program. The machine side now filters on `a`/`b`, but the compiler still skips the argument list, and my edits to that handler would not match. | Parse `[[first][-[last]]]` in the `LLIST` branch of the statement dispatcher (six-space indent, just above the `LPRINT` branch) and put the bounds in `in.a`/`in.b`; the `Op::LList` side is ready. Then reinstate the range test in `test_printer.cpp`. |
| `PEEK`/`POKE`/`INP`/`OUT`/`WAIT`/`USR`/`VARPTR`/`FRE` | See M13: a scope decision, not an oversight. `CALL` is no longer in this list: it now calls a BASIC `SUB`. | ~~Decision~~ **done**: refused clearly (no machine model); the functions used to be syntax errors and now name themselves |

### B. Engineering debt

- ~~`kUnimplementedStatements` lists implemented statements~~ **done**: pruned to the
  genuinely unimplemented set, and `tests/test_inventory.cpp` now checks both
  directions (implemented statements must not report Advanced Feature; the stub list
  must match the compiler's table), so it cannot rot unnoticed again.
- ~~The `[.probe]` tests print classifications~~ **done**: replaced by the asserting
  inventory test above; the probes are deleted.
- ~~Temp-file isolation only in some tests~~ **done**: `lwtest::TempName` gives every
  filesystem test a per-process name, so a leftover file cannot change a run.
- ~~CI runs the test binary directly rather than `tools/verify.ps1 -NoRetry`~~ **done**:
  CI now runs the wrapper in strict mode, so a first-attempt failure is treated as
  real and every run's log is captured.
- ~~No `--audio-check` flag~~ **done**: `--audio-check` plays a short arpeggio and exits, so the
  `BEEP` path can be checked by ear without a program file.
- ~~`LLIST` needs a printer~~ **done**: `Host::WriteToPrinter` gives the dialect a
  printer sink (a file in `SystemHost`, a recorder in `ScriptedHost`), and both
  `LLIST` and `LPRINT` use it, including `LPRINT USING`.
- ~~The `newbasic-debug` library split is deferred~~ **done**: the debugger and the DAP
  protocol headers (`debugger.h`, `dap/*.h`) moved into their own `newbasic-debug`
  target with `git mv`, so history follows. `newbasic-core` no longer mentions either;
  `newbasic-dap` and the tests link the new library. This works because the headers
  moved too - the circular dependency was the whole reason it had been deferred.
- No `docs/` note for M8–M11; their caveats live in the milestone rows above.

### C. Documented divergences (decisions, not gaps)

- `RND` uses `mt19937`, so sequences differ from GW-BASIC's generator; they are
  reproducible per `RANDOMIZE` seed.
- Sound: `MB` behaves as `MF`; `PLAY`'s `Xstring;` is unsupported; tones are capped
  at 5 s.
- **The `KEY` family is deliberately unsupported.** `KEY n, string`, `KEY ON/OFF/LIST`, `KEY(n) ON/OFF/STOP`, and `ON KEY(n)` all report Advanced Feature. Newbasic does not expose function-key macros or function-key event traps; `INKEY$` remains the supported keyboard-polling function. This avoids a partial soft-key implementation that could not distinguish F1–F10 reliably.
- `INKEY$` does not synthesise the two-character escaped code for a function key, because function keys are not exposed by the input layer.
- `WIDTH` wraps text at the new width but does not resize the physical console.
- The default `VIEW PRINT` window is the whole grid rather than lines 1–24, because newbasic has no function-key bar.
- `FIELD` widths must be constants; `GET` on a never-written record zero-fills.
- **`SCREEN` accepts every reference mode, 0 to 13**, plus the extension mode 720. Modes 4, 5, 6, 10 and 11 used to be refused for want of a canvas size; each now maps to one that already existed (4/5 to 320x200, 6 to 640x200, 10 to the mode 9 canvas, 11 to the mode 12 canvas), and 3 is a text mode, so it stays text. Tested at each mode far corner, which is what proves the size.
  depends on them.
- **Protected-file and tokenized byte fidelity: deferred by the maintainer, 2026-10-17.** `SAVE ,P` and `SAVE,B` are implemented and round-trip through this interpreter, but byte-for-byte agreement with Microsoft-produced files cannot be checked without samples, and the maintainer has none to test with. Nothing is known to be wrong; the claim is simply unproven.
- **Procedures are a newbasic extension (2026-10-24).** `SUB`/`FUNCTION` add
  per-call parameters and `LOCAL` variables, recursion, `EXIT`, and `CALL`
  as a BASIC procedure call. `CALL` is no longer the GW-BASIC machine-language
  call; `USR` remains refused. V1 requires procedure `END` lines, keeps
  handlers in main, and does not implement `BYREF`, array parameters, or local
  arrays. See [`enhancedsubs.md`](../enhancedsubs.md).

### D. Awaiting the maintainer

- ~~The D3D11 window has never been checked by eye~~ **done**: see the note below.
- **RESOLVED — the windowed backend works.** The maintainer ran `--gpu-check` and then
  `--gpu docs/verify-graphics.bas` on their own machine on 2026-10-17 and both rendered
  correctly: the check printed `PASS` with a red dot on black, and the demo showed the
  frame, green circle, magenta diagonal, red filled and outlined boxes, both `PSET` dots
  and the `DRAW` square, with the circle visibly round. The blank white window seen in
  earlier rounds was therefore this agent session's compositor, not the code.
  `D3D11Screen::ReadBackCanvasPixel` and `--gpu-check` remain in place so a regression in
  the pixels is caught without a screenshot.
- **Extension mode.** `SCREEN 720` (1280x720, 256-colour programmable palette, 160x45
  text grid) is **not** a GW-BASIC mode; it is an addition, recorded here so it is not
  mistaken for dialect behaviour. `PALETTE` in that mode takes 256 attributes and an
  18-bit colour (six bits per channel), which is the VGA form the dialect used.
- **Interpreter speed measured, and it is fine.** `--bench` reports, headless:

  | Workload | Rate |
  |---|---|
  | integer `FOR` loop | 3.7M iterations/s |
  | float expressions | 1.1M iterations/s |
  | Mandelbrot inner loop | ~38k cells/s (~900k inner iterations/s) |
  | `PSET` | 1.7M/s |

  That cancels the suggested variable-interning work. Measurements then showed the cost
  was not presentation either: with drawing removed the Mandelbrot still spent 44 of its
  53 seconds, and the culprit was `PumpMessages` being called on every instruction, so
  `PeekMessage` ran roughly once per VM instruction. Pumping is now throttled to 16 ms,
  the same way presents are throttled to 32 ms, and the same plot takes 10.1 s. Closing
  the window still ends the run, because 16 ms is far below what a person notices.
- The window is now sized to the content (640x400 at 96 DPI) with `--gpu-scale N` to
  choose larger, which fixed the maintainer's report that it was oversized for 320x200
  graphics.
- **`EOF(n)` works** and always did: it was in the refusal list by mistake, so a bare `EOF`
  statement reported Advanced Feature even though `EOF(1)` in an expression resolved normally. It is
  now out of the list, and tested: false before reading a file, true after its last line, Bad file
  number for an unopened handle, and reading past the end raises Input past end (62), which is the
  error EOF exists to avoid.
- **Deliberate refusals** remain for `PEN`, `CHDIR`, `COM`, `ENVIRON`,
  `FILES`, `INP`, `IOCTL`, `KILL`, `LINE` styling expressions that are not constant,
  `MKDIR`, `MOTOR`, `NAME`, `OUT`, `POKE`, `RMDIR`, `STICK`, `STRIG`, `USR`, `VARPTR`,
  `WAIT`, and the machine-level functions (M13 decision). `CALL` is no longer a
  refusal; it now calls a BASIC `SUB`.
- **Protected-file fidelity: deferred** by the maintainer (no samples to test with); see section C.
- **M12 graphics**: decided **in**, and complete — phases 1–2, `PALETTE`, the D3D11
  presentation and phase 3 (`VIEW`/`WINDOW`/`PMAP`/`PCOPY`, graphics `GET`/`PUT`) are all done,
  as the milestone table records.
- **M13 (machine-level statements)**: decided **refuse**; see the milestone table.

Nothing is awaiting the maintainer: the three items that were (protected and tokenized byte fidelity, and the `KEY` defaults) are deferred above, and the screen modes are implemented.

## 8. Risks

- **Fidelity without an automated oracle** — golden expectations are
  hand-verified; keep the corpus to hand-computable, fidelity-stable
  behavior and document known divergences alongside the fixtures.
- **`PRINT` formatting quirks** — isolated in one module with a test suite.
- **Conhost quirks** — code page, VT state, resize, focus loss, and
  `Ctrl+C` behavior differ between conhost and Windows Terminal; mitigate
  with the Win32-API backend, `HeadlessScreen`-based tests, and a documented
  manual smoke matrix on both hosts.
- **Event trapping / input nondeterminism** — virtualize via `Host`.
- **DAP scope creep** — land a documented subset first.
- **D3D initialization failures (M7)** — headless/CI machines may lack GPU
  drivers; the D3D11 backend must degrade to a software rasterizer
  (`WARP`) or a headless path in tests.

## 9. Assumptions

- MIT license (compatible with referencing the open-sourced GW-BASIC sources).
- MSVC toolchain (Visual Studio 2022 or later; developed on VS 2026 / MSVC 14.51,
  C++17); no ncurses or other terminal libraries — terminal I/O via the Win32
  console API. Local CMake is the VS-bundled one (not on PATH).
- Windows 10 1809 or later for the console APIs and CI.
- Build prerequisites on Windows: the Visual Studio C++ workload (MSVC
  compiler **and** the STL headers/CRT libraries) plus a Windows 11 SDK.
  CMake's VS auto-detection relies on `vswhere` reporting
  `Microsoft.VisualStudio.Component.VC.Tools.x86.x64`; VS 2026's installer
  did not report it until the C++ workload was repaired, so pass an explicit
  generator (`-G "Visual Studio 18 2026" -A x64`) when in doubt.
- Linting is pinned to clang-format 19 (CI action version `19`; locally
  `pip install clang-format==19.1.0` puts a matching binary on disk), so
  local checks and CI agree.
- Binary name is `newbasic.exe`.

## Backlog

Recorded 2026-10-17, found while writing `samples/FamousBirthdays.bas`. Each is small and none is
in the dialect's spirit to leave broken.

- **Done: `READ` fills array elements.** Each target compiles to its own instruction (`Op::Read`
  for a scalar, `Op::ReadArray` for an element), the subscript expressions are already on the
  stack, and the machine pops them and takes the next DATA item in order. Mixed lists, string
  arrays, computed subscripts, Out of DATA and subscript out of range are all tested.
- **Done: a reserved word used as a name says which word.** `DIM PRINT(3)` now reports that `PRINT`
  is a reserved word rather than a bare syntax error, at the three places a name is parsed. Keywords
  are whole words: `TOTAL` and `FNA` are names, while exact `TO`/`FN` remain reserved.
- **`DIM` lists work; my note here was wrong.** `DIM A(10), B(10)` compiles and both arrays take
  writes. The line that misled me began with `FN$`, which was reserved under the old prefix rule;
  whole-word lexing now treats `FN$` as a name, but `FN$(...)` keeps its user-function meaning.

Two smaller notes from the same program, worth a look when the above are done:

- Confirmed, not assumed: `PRINT USING` and `TAB` both work, and the database reports could use
  worth confirming both work so samples can use them.
- ~~The record picker and the form are shared by add, edit and delete, but only add was exercised~~
  **verified 2026-10-17**: the maintainer exercised edit and delete by hand in
  `samples/FamousBirthdays.bas` and both behave correctly.
