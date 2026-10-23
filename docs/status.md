# Upstream status snapshot — 2026-10-17

> **Port note.** This is the Windows snapshot inherited from upstream `newbasic`.
> For the dual-platform state of `newbasic`, see `README.md` and
> `docs/portability.md`. On Linux the full default suite passes, the hidden
> audio timing check passes silently, and the SDL2 window backend passes the
> GPU check headlessly with `SDL_VIDEODRIVER=dummy`.
>
> **Historical note.** The `KEY`/`KEY(n)`/`ON KEY(n)` family described below
> was subsequently removed; those forms now report Advanced Feature. `INKEY$`
> remains supported.

**Everything is in git; nothing lives only on the old machine.** Clone, build, run the
tests, and you are exactly where the previous session stopped.

- Branch: `main`, commit **`49efba2436b95dbe0643e3140412d2c4abbc0b6a`** ("A better voice: ADSR,
  vibrato and two detuned oscillators"), clean tree, pushed.
- Suite: **31104 assertions in 363 test cases**, all passing; `ctest` 2/2.

## Build from scratch

Requires Visual Studio with the C++ workload and the Windows 11 SDK; CMake comes with Visual
Studio. Third-party dependencies (Catch2, RapidCheck) are vendored in `third_party/`, so no
network access is needed.

    cmake -S . -B out -G "Visual Studio 18 2026" -A x64
    cmake --build out --config Release --parallel

If the generator name is not available on the new machine, run `cmake --help` and use the
Visual Studio generator it lists.

## Verify

    ctest --test-dir out --build-config Release --output-on-failure
    out\tests\Release\newbasic-tests.exe            # the Catch2 suite
    tools\verify.ps1 -NoRetry                      # what CI runs: build, kill strays, ctest

## Try it

    out\src\newbasic-cli\Release\newbasic.exe samples\Beethoven.bas
    out\src\newbasic-cli\Release\newbasic.exe samples\FamousBirthdays.bas
    out\src\newbasic-cli\Release\newbasic.exe samples\playme.bas

`--gpu-shot FILE` writes the screen to a PPM; `--gpu-pixel X,Y` reads one pixel back from the
window itself, which is the only way to check what the display actually did (the dump is written
by separate code and has disagreed with the window twice). `--keys` and `--clicks` script input.

## One thing in flight

**Sound playback timing.** The voice was replaced on this date: `RenderTone` in `sound.h` now
renders two detuned sine oscillators, a second harmonic, a vibrato that fades in after 120 ms and
an ADSR envelope, played through waveOut instead of `Beep`. It is tested by shape (sample count,
envelope, zero crossings) and sounded much better to the maintainer by ear.

What is unresolved: playback may be slow. Measured here, `samples/playme.bas` took about 31
seconds for roughly ten seconds of music, where the earlier renderer took 14. The maintainer did
not report dragging tempo when listening, so this may be an artifact of this environment rather
than a real fault. **Measure it before changing anything.** If it is real, the cause is the
completion wait in `SystemHost::PlayTone` (`src/newbasic-core/src/host.cpp`): replacing

    while ((header.dwFlags & WHDR_DONE) == 0)
      Sleep(1);

with `Sleep(static_cast<DWORD>(milliseconds));` should fix it, because the samples are ours and
their length is known exactly. Attempts to do this on 2026-10-17 failed to land cleanly and were
reverted, so treat it as unwritten.

## Outstanding, in the order I would take them

1. **`LLIST` with a range** — `LLIST 20-30` prints the whole program. The machine side already
   filters on `in.a`/`in.b`; the compiler's `LLIST` branch (in the statement dispatcher, six-space
   indent, just above the `LPRINT` branch) still skips its argument list. Parse
   `[[first][-[last]]]` there, then reinstate the range test in `test_printer.cpp`.
2. **`ON ERROR GOTO 0` inside a handler** — untested. `Op::OnError` clears `handling_error_`, so it
   should work; "should" is how `EOF` was reported wrongly. Test it.
3. **The VS Code extension has never been run by the agent.** `editors/vscode` is complete and its
   launch shape was verified through the adapter, but pressing F5 is the only real test. See
   `docs/m5-dap-debugger.md`.
4. **No way to capture a corpus baseline** — `fixtures/**/*.bas` are compared against `.expected`,
   but nothing generates one, so a golden fixture must be hand-written and guessed. A `--capture`
   mode would make them cheap. A `.keys` sidecar already lets fixtures script input.
5. **Test tags are inconsistent** — only `[m10]`/`[m11]` use milestone tags, so coverage cannot be
   audited by milestone.
6. **Docs exist for M4, M5, M7, M12 only** — M8 (text-mode completeness), M9 (random-access files),
   M11 (event trapping) and M13 (the refusal decision) are documented only in `plan.md` and code.

## Deferred and refused

Not gaps, decided deliberately, all recorded in `plan.md`:

- **Deferred by the maintainer:** `EDIT` (the screen editor), the hardware event traps
  (`ON COM`/`ON STRIG`/`ON PEN`/`PLAY ON/OFF/STOP`), the `CHAIN` forms (`MERGE`, `ALL`, `COMMON` of
  arrays), `KEY` F1–F10 defaults, and protected/tokenized byte fidelity.
- **Refused by policy (M13):** 20 machine-level and filesystem statements, listed in `plan.md`
  section A. Each names itself in an "Advanced Feature" error.

## Where the knowledge lives

- `plan.md` — milestones, gaps, deferred and refused items, documented divergences. The single
  source of truth for what is left.
- `docs/` — design notes per milestone, and `docs/m5-dap-debugger.md` for the debugger.
- `samples/` — six programs that exercise the dialect, all runnable by hand.
- `fixtures/` — golden programs with `.expected` output, and optional `.keys` input scripts.

---

# Historical snapshot — 2026-10-16

The following was written at an earlier session's shutdown. It is kept for history and is no
longer current: the D3D11 backend exists, audio no longer uses `Beep`, and M7 is complete.
# Status snapshot — 2026-10-16 (session shutdown)

> **Update after shutdown (same day).** M6 is complete (golden-corpus runner,
> conformance batches for statements, display precision, PRINT separators/width
> and Appendix A error codes), and **M7's sound work is done and audibly
> verified by the user**: `BEEP`, `SOUND` and `PLAY` go through
> `Host::PlayTone`, the real host calls Win32 `Beep()`, and the tones were
> confirmed by ear. Remaining M7 work is the D3D11 screen backend and
> time-travel debugging (both deliberately deferred).
>
> Manual audio check (hidden from CI, so build machines stay silent):
> `out\tests\Release\newbasic-tests.exe "[.audio]" -s` — it plays the BEEP tone
> and a rising arpeggio, and asserts the call actually blocks (measured 251 ms
> for an 800 Hz/250 ms tone).
>
> Also fixed since this snapshot: `Vm::Step()` executed past a halt (a debugger
> stepping past `END` ran trailing code); it now latches a halted state that
> `Run()`, `SetPc()` and snapshot restore arm correctly.
>
> **CTest flakiness: solved — it was a real codec bug, not a stray process.**
> Capturing test output (see `tools/verify.ps1` below) finally caught it: a
> RapidCheck property failed with *"Falsifiable after 98 tests and 6 shrinks"*,
> which is why it came and went. The tokenized encoder's number scanner was
> wrong in two ways: it read `&H`/`&O` constants greedily, so a decoded `&H4F10`
> followed by `0.5` re-encoded as one longer constant, and it accepted letters
> in octal constants, so `&O3627` followed by `PRINT` swallowed the letters.
> Both corrupt program text and therefore the bytes. Fixed by limiting constants
> to 4 hex / 6 octal digits and validating digits per base; verified with 12
> fresh property seeds (previously roughly 60% failed) plus the shrunk
> counterexample pinned as a regression test. The earlier "stray process"
> explanation was wrong and is retracted here.
>
> Two hygiene problems found along the way were also fixed:
> `dap_smoke.cmake` wrote its temp request into the *source* tree (fails on a
> read-only checkout, can race), and failures were invisible unless
> `--output-on-failure` happened to be passed by hand. `tools/verify.ps1` now
> builds, kills stray test processes, always captures ctest output to
> `out/test-logs/`, and retries once so a glitch is reported as flaky rather
> than mistaken for a regression.
>
> **M7 is now implemented.** Sound (verified by ear), time travel (library +
> DAP `stepBack`/`reverseContinue`) and the D3D11 windowed backend (window,
> device with WARP fallback, shaders, glyph atlas, per-cell quads, resize, DPI,
> cursor, one-shot device recovery, `--gpu` flag, hidden `[.gpu]` check) are all
> in. The only outstanding item for M7 is human verification of what the window
> looks like - see `docs/m7-d3d11-backend.md`.
>
> **M8-M11 are now implemented.** Text-mode completeness (`PRINT USING` incl. the
> deliberate refusals for `**`/`$$`/`**$`/`^^^^`, `WIDTH`, `VIEW PRINT`, `CLS` variants,
> the `KEY` family), random-access files (`OPEN FOR RANDOM`, `FIELD`, `LSET`/`RSET`,
> `GET`/`PUT`, `LOF`/`LOC`, `MKI$`/`MKS$`/`MKD$`, `CVI`/`CVS`/`CVD`), program control
> and chaining (`RUN`, `TRON`/`TROFF`, `LIST`, `DELETE`, `NEW`, in-program `SAVE`/`LOAD`,
> `CHAIN`/`COMMON`), and `KEY(n)` event trapping. Statements that cannot be honest
> (`RENUM` without reference rewriting, `LLIST` without a printer, `ON COM`/`ON STRIG`/
> `ON PLAY` without hardware, the remaining `PRINT USING` field types) report Advanced
> Feature naming the reason instead of misbehaving. `plan.md`'s milestone table
> carries the per-milestone caveats.
>
> Current numbers: **6073 assertions in 251 test cases**, ctest 2/2, tree clean.
> M12 (graphics) and M13 (machine-level statements) are still awaiting a scope
> decision.

Recorded at the end of an autonomous working session so the next session starts
from verified facts rather than memory. Everything below was measured on this
date, not estimated.

## Milestones (see `plan.md` for the full table)

| # | Deliverable | Status |
|---|---|---|
| M0 | Skeleton, `Screen` + Win32/headless backends, `newbasic.exe prog.bas`, vendored Catch2 + RapidCheck, CI | ✅ |
| M1 | Lexer, expressions, line store, direct-mode REPL, control flow | ✅ |
| M2 | Types, arrays, `DEF FN`, `DATA`/`READ`/`RESTORE` | ✅ |
| M3 | Error handling, plain-text program files, `SHELL`, sequential file I/O, DOS commands, timer events | ✅ |
| M4 | Tokenized `.BAS` read/write, MBF codec, protected format, `SAVE ,B` / `,P` | ✅ |
| M5 | In-process debugger + `newbasic-dap` (framing, JSON, dispatcher, session, executable) | ✅ |
| M6 | Golden corpus + spec-driven conformance | ✅ (54 conformance assertions across 4 areas; corpus is a living set) |
| M7 | D3D11 backend, sound, time-travel debugging | ⏳ not started |

## Measured numbers

- **Tests: 5558 assertions in 164 test cases**, all passing.
- **CTest: 2/2 passed** — `newbasic-tests` (unit/property) and `dap-smoke` (runs
  the real `newbasic-dap.exe` with framed stdin and checks the framed output).
- **Code: 37 source files / 7866 lines** under `src/`;
  **26 test files / 3195 lines** under `tests/` (25 of them `test_*.cpp`).
- **Fixtures: 2** (`fixtures/m0/hello.bas`, `fixtures/m6/strings.bas`, each with
  its `.expected`) — the corpus runner pairs every `fixtures/**/*.bas` with a
  sibling `.expected`, so adding a case needs no test code.
- **Conformance coverage (M6):** statement behaviour, numeric display precision,
  PRINT separators and the 80-column width quirk, and the Appendix A error-code
  numbering.
- **Git: 53 commits**, `HEAD = 6aa66f8`, working tree clean, in sync with
  `origin/main`.

## How to build and verify

```
cmake -S . -B out -G "Visual Studio 18 2026" -A x64
cmake --build out --config Release --parallel
ctest --test-dir out --build-config Release --timeout 120 --output-on-failure
clang-format -i --style=file (all *.cpp/*.h in src, tests)   # pinned 19.1.0
```

MSVC `/W4 /WX` is applied to the project's own targets (not vendored code);
clang-format is pinned to 19.1.0 to match CI.

**Operational hazard:** a timed-out CTest leaves `newbasic-tests` running, and
subsequent runs then report spurious failures. Kill stray `newbasic-tests`
processes before re-running.

## Open items (also in `plan.md` §7a)

1. **Audit the M3 timer notes (TODO).** Timer events work — measured
   `ticks=24` through `Vm::Run`. The long "firing does not happen" analysis in
   `plan.md`'s M3 section is historical; condense it to the real root cause
   (the progress flag was set *inside* the poll, so a handler's `RETURN`
   re-satisfied the guard).
2. **Diagnostic scaffolding (TODO).** `Vm::TimerDebug()` and the `[diag]` test
   are deliberate debugging aids; decide whether to keep or delete.
3. **Reference-material licensing (resolved, decision left).** Microsoft's
   revision places GW-BASIC and derived works under MIT, so `GWBASIC.txt` and
   `docs/gw-basic/` are no longer copyright-encumbered. `.gitignore` still
   excludes them; committing them is now a repo-size preference.
4. **Protected-file encryption is out of scope** (user decision). `SAVE ,B` and
   `,P` stay; the Kocher transform remains implemented and round-trip tested
   but is not a conformance requirement.

## Known caveats and decisions worth remembering

- **Protected-file fidelity.** Our `ProtectTokenized`/`UnprotectTokenized` are
  proven mutual inverses, **not** byte-compatible with Microsoft-authored
  files: the index offset and trailer handling are inferred from the published
  description. See `docs/m4-protected-files.md`.
- **`SAVE` grammar is inverted** vs GW-BASIC: text is the default, `,B` asks
  for tokenized and `,P` for protected. If `SAVE BIN "file"` is preferred, it
  is a small change in `repl.cpp`.
- **Format facts were sourced, not guessed:** token values and the Kocher
  keys/transform from PC-BASIC (GPL-3.0, provenance recorded), MBF layout from
  the Microsoft Binary Format reference, envelope/line layout from the GW-BASIC
  tokenised-format description. See `docs/m4-tokenized-format-notes.md`.
- The interpeter uses **host floating point** for arithmetic by project
  decision; only PRINT display formatting emulates GW-BASIC.

## Where things live

- `plan.md` — spec, milestone table, status detail, open items.
- `docs/m4-tokenized-format-notes.md`, `docs/m4-protected-files.md` — format
  findings and provenance.
- `src/newbasic-core/include/newbasic/` — `fstok/` (token table, MBF, codec,
  encoder, protect), `dap/` (framing, json, handler, session), `debugger.h`,
  `repl.h`, `vm.h`, `screen/`.
- `src/newbasic-dap/` — the DAP server executable.
- `tests/` — 25 test sources, including `test_vm_print.cpp` which now holds the
  M6 golden-corpus runner and the conformance batch.

