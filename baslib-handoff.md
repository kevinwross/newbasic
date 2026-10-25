# baslib session handoff

**Session:** `baslib`  
**Date:** 2026-10-25 (America/Phoenix)  
**Repository:** `C:\Users\kevin\repos\newbasic`  
**Branch:** `main`  
**Purpose:** preserve the baslib work and current repository state so another session can continue on another computer.

> **Push rule:** another session owns the GitHub push. This session did **not** push.
> Do not force-push.

---

## 1. Current repository state

At the start of this handoff:

| Item | Value |
|---|---|
| Local branch | `main` |
| Local HEAD | `4d58222` (`Add stng session handoff and bigpush push ledger`) |
| `origin/main` | `d66efed` (`Add blUpdateAll for dirty-subtree repainting`) |
| Local commits ahead | 2 (`cd35000`, `4d58222`) |
| Working tree | clean except untracked `baslib/merged.bas` |

The two local commits ahead are from other sessions:

- `cd35000` — `Add DRAW examples for shapes, fills and patterns`
- `4d58222` — `Add stng session handoff and bigpush push ledger`

This session adds one local handoff commit containing `baslib-handoff.md` and the
`bigpush.md` update. The push session should push all local commits, not just
this one.

---

## 2. baslib work completed

baslib is now **version 0.3** with a fully parameterized public API.

Relevant commits already on `origin/main`:

| Commit | Subject |
|---|---|
| `b36137f` | `Replace baslib request globals with parameterized SUB/FUNCTION API` |
| `514ce87` | `Rename baslib variables to descriptive names` |
| `d66efed` | `Add blUpdateAll for dirty-subtree repainting` |

### 2.1 Public API

No `blApi...`, `blReq...`, or `blRet...` staging globals remain. Public calls are
`SUB`/`FUNCTION` procedures with parameters. `blStatus%` is the only shared
result global. Event fields are read through `blEvent...%()` functions.

Main calls:

```basic
CALL blInit(Width%, Height%)
Handle% = blCreate%(Parent%, Class%, Kind%, PosX%, PosY%, Width%, Height%, Text$, Action%, Flags%, TagId%)
CALL blDestroy(Handle%)
CALL blSetBounds(Handle%, PosX%, PosY%, Width%, Height%)
CALL blSetLayout(Handle%, Mode%, PadSize%, GapSize%)
CALL blSetText(Handle%, Text$)
Text$ = blGetText$(Handle%)
CALL blSetValue(Handle%, Value#)
Value# = blGetValue#(Handle%)
CALL blSetAction(Handle%, Action%)
CALL blSetActionLine(ActionId%, Line&)
CALL blSetClickMask(Handle%, Mask%)
CALL blInvalidate(Handle%)
CALL blLayoutAll
CALL blPaintAll
CALL blUpdate(Handle%)
CALL blUpdateAll
CALL blAutoSize(Handle%)
CALL blRegisterClass(Class%, Line&, Name$)
```

Event/test calls:

```basic
EventTyp% = blPollInput%()
EventTyp% = blNextEvent%()
CtlH% = blEventCtl%()
ActionId% = blEventAction%()
PosX% = blEventX%()
PosY% = blEventY%()
Button% = blEventButton%()
Wheel% = blEventWheel%()
Count% = blEventCount%()
HitH% = blFindAt%(PosX%, PosY%)
HitH% = blTestClickAt%(PosX%, PosY%, Button%)
CALL blTestMouseMove(PosX%, PosY%)
CALL blTestMouseButton(Button%, Down%, PosX%, PosY%)
CALL blTestWheel(PosX%, PosY%, Delta%)
```

### 2.2 Variable naming

Procedure parameters and locals were renamed to descriptive 4–10 character
names: `Handle%`, `Width%`, `Height%`, `PosX%`, `PosY%`, `PadSize%`, `GapSize%`,
`ClassId%`, `Parent%`, `PadSize%`, `TextCol%`, `LabelText$`, `SourceIn$`,
`SourceOut$`, and so on.

`Input$` could not be used because it collides with the `INPUT` keyword, hence
`SourceIn$`/`SourceOut$`.

### 2.3 Painting and dirty tracking

- `blPaintAll` performs a full tree paint and clears all dirty flags.
- `blUpdate(Handle%)` paints that control and descendants unconditionally. It
  does **not** clear dirty flags.
- `blUpdateAll` walks the tree, finds the first dirty control, repaints that
  whole subtree (parent background plus descendants), and clears the dirty
  flags. This is **dirty-subtree** painting, not per-pixel damage tracking.
- Setters mark the changed control and its parent dirty. `blCreate`,
  `blDestroy`, `blSetBounds`, `blSetLayout`, `blShow`, `blHide`,
  `blSetEnabled`, `blSetText`, `blSetValue`, `blAutoSize`, `blInvalidate`, and
  slider pointer changes all participate.

`demo-controls.bas` does one `blPaintAll` at startup and then uses
`blUpdateAll` in its loop and action handlers.

### 2.4 Known caveat

`blUpdate(Handle%)` does not clear dirty state. A subsequent `blUpdateAll` will
therefore still repaint the dirty parent subtree. This is conservative and
safe, but a true one-off update that consumes dirty state needs either:

- a separate update call that repaints the parent background and target control,
  then clears the parent only if no other dirty children exist; or
- separate “self dirty” and “child dirty” state.

Also, the readout label in `demo-controls.bas` is transparent by default
(`blUser#(Handle%,18) = 1`). Painting only the label does not erase old text.
For a targeted label update, make it opaque (`blUser#(Handle%,18) = 0`) or
repaint its parent.

---

## 3. Verification

Last verified in this session:

- Full Catch2 suite: **516 test cases / 32,037 assertions passing**.
- CTest: **7/7 passing**.
- baslib tests: **21 test cases / 2,806 assertions passing**.
- baslib CLI fixtures: **3/3 passing** (`api_smoke`, `layout_paint`, `stack_sentinel`).

Commands:

```powershell
# Build tests
& 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' `
  --build out --config Release --target newbasic-tests --parallel

# Full suite
.\out\tests\Release\newbasic-tests.exe

# baslib suite
.\out\tests\Release\newbasic-tests.exe "[baslib]"

# CTest
& 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' `
  --test-dir out -C Release --output-on-failure

# baslib CLI fixtures
pwsh -NoProfile -ExecutionPolicy Bypass -File .\baslib\tests\run_cli.ps1
```

The CLI binary is at `bin\Release\newbasic.exe`; the old
`out\src\newbasic-cli\Release\newbasic.exe` path does not exist.

A `cmake --build out --clean-first` run was aborted earlier. The test target was
rebuilt afterward, but a clean full build may still be needed before release
packaging.

---

## 4. Uncommitted files

At handoff time the only working-tree entry was:

| File | State | Handling |
|---|---|---|
| `baslib/merged.bas` | untracked, generated merge artifact | Not committed; it is reproducible from `baslib/baslib.bas` plus a host and was deliberately left out |

This file was also noted by the previous `stng` session. Do not add it to the
push unless the baslib source/host pair is intentionally being regenerated and
committed.

---

## 5. Files changed by this session

This handoff commit adds:

- `baslib-handoff.md`
- `bigpush.md` (updated ledger entry)

Previously committed baslib files of interest:

- `baslib/baslib.bas` — v0.3 implementation
- `baslib/baslib-prog-ref.md` — v0.3 programmer reference
- `baslib/demo.bas`
- `baslib/demo-controls.bas`
- `baslib/samplehost.bas`
- `baslib/tests/README.md`
- `baslib/tests/fixtures/*.bas`
- `tests/test_baslib.cpp`
- `RefManual/*` procedure entries and index
- `enhancedsubs.md`
- `docs/newbasic-spec-plan.md`
- `docs/status.md`

---

## 6. Next actions

1. **Push session:** follow `bigpush.md`. Run `git log --oneline origin/main..main`,
   then `git push origin main`. Never force-push.
2. After push, update `bigpush.md` §1 to say nothing is pending and record the new
   `origin/main` hash.
3. If continuing baslib:
   - Consider adding a true one-off update API that clears dirty state safely.
   - Consider making the `demo-controls.bas` readout label opaque if switching
     that action from `blUpdateAll` to `blUpdate(ReadoutH%)`.
   - Optionally add `baslib/merged.bas` to `.gitignore` or a documented
     regeneration step.
4. Re-run the full suite and baslib CLI fixtures after any further changes.

---

## 7. Coordination notes

- Multiple sessions share this repo. Use `bigpush.md` as the push ledger.
- Do not overwrite another session’s `*-handoff.md` or `bigpush.md` sections.
- Current local commits from other sessions (`cd35000`, `4d58222`) must travel with
  this handoff commit.
- The baslib code commits `b36137f`, `514ce87`, and `d66efed` are already on
  `origin/main`; do not rewrite history around them.
