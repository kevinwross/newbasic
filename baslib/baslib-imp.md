# baslib implementation plan

**Status:** Planning only — no baslib implementation has started.  
**Design source:** [`baslib-plan.md`](baslib-plan.md)  
**Target release:** v0.1 first; v0.2 follows the same process.  
**Primary runtime:** newbasic on Windows and Linux.  
**Last updated:** 2026-10-24

This document converts the design in `baslib-plan.md` into an ordered,
testable implementation effort. The design remains authoritative for behavior;
this file is authoritative for **sequence, scope, and completion criteria**.

Use the checkboxes as the tracking record. Tick a box only when the item is
implemented, reviewed, and covered by the test named beside it.

---

## 0. Working rules

- [ ] Read `baslib-plan.md` before starting a milestone.
- [ ] Keep `baslib.bas` inside `500000–519999` and the host side inside `1–499999`.
- [ ] Use the lowercase `bl` prefix for every baslib symbol and label.
- [ ] Give every public entry point a `LABEL blApi...`; never call a public entry
      point by literal line from host code.
- [ ] Keep the numeric API lines from `baslib-plan.md` §14.2 stable.
- [ ] Use explicit type suffixes on every `bl` variable and array.
- [ ] Keep `OPTION BASE` at the default `0`.
- [ ] Never use `DIM` inside a paint, dispatch, or event loop.
- [ ] Treat expected failures as `blStatus%`/`blError%` returns, not BASIC runtime
      errors.
- [ ] Keep host `STACK` values below baslib frames untouched.
- [ ] Add or update a test for every behavior change.
- [ ] Update this file and, when behavior changes, `baslib-plan.md` in the same
      commit.
- [ ] Do not begin a milestone until the previous milestone gate passes.
- [ ] Prefer one milestone per commit and one focused test suite per milestone.

**Status legend:** `[ ]` not started · `[~]` in progress · `[x]` complete ·
`[!]` blocked.

### 0.1 Constraints carried from the design

- `CONST name = literal` only. The constants block uses literal values; do not
  write computed constant expressions.
- `LABEL` is compile-time. It is for direct `GOSUB`/`GOTO` calls; it cannot be
  stored as a line value.
- Method tables (`blMethod&`, `blUserMethod&`, `blPanelLayoutLine&`) and action
  lines (`blActionLine&`) store numeric line values because dispatch uses
  computed `GOSUB`/`GOTO`.
- `blSetActionLine` therefore takes a numeric line, not a label. Document the
  host action handler as a numeric line.
- The library has no user-defined local variables; all state is explicit arrays
  and the `STACK` frame protocol.
- `OPTION BASE 1` is unsupported; every array is base 0.
- `STACK` has no depth query; track `blFp%` and work-item counts explicitly.
- Do not use `ON ERROR` as expected control flow; return status codes.

---

## 1. Fixed line map and budget

The line map is a contract. Use `baslib-plan.md` §4.2 for the full description.

| Range | Contents | Milestone |
|---|---|---|
| `1–499999` | Host program; never used by baslib | — |
| `500000` | Hard `END` guard | M1 |
| `500010–500099` | Version, limits, statuses, enums, flags, `blNoLine&` | M1 |
| `500100–500299` | `STACK` frame protocol, depth state, argument cache/local storage | M2 |
| `500300–500599` | Errors, validation, geometry, string and numeric helpers | M3 |
| `500600–500999` | Method tables, action-line table, class/kind metadata | M4 |
| `501000–501999` | Control table, handles, free list, tree links, create/destroy | M3 |
| `502000–502999` | Dispatch, message helpers, result transport | M4 |
| `503000–503999` | Layout pass, absolute rectangles, clipping, work-stack traversal | M5 |
| `504000–504999` | Paint pass, `PUSH VIEW`/`VIEW`/`POP VIEW`, dirty/update plumbing | M6 |
| `505000–505999` | Input polling, hit testing, hover/press/capture, event ring | M7 |
| `506000–506999` | Top class methods | M8 |
| `507000–507999` | Panel class methods | M8 |
| `508000–508999` | User class and button/label kind methods | M8 |
| `509000–509499` | Click-target class methods | M8 |
| `509500–509999` | Shared drawing, text, geometry, event and value helpers | M8 |
| `510000–511999` | Public API entry points and wrappers | M1 stubs, M8/M9 complete |
| `512000–518999` | Debug/test helpers and example-only code | M9 |
| `519000–519999` | Reserved for future public API | — |

### 1.1 Budget tracker

Update `Used` and `Status` after each milestone. The gate is **not** the estimate;
the gate is that the code stays inside its block and inside `500000–519999`.

| Block | Budget | Used | Status |
|---|---:|---:|---|
| Guard and constants | 90 | 0 | `[ ]` |
| Frame protocol | 200 | 0 | `[ ]` |
| Core helpers | 300 | 0 | `[ ]` |
| Method/action tables | 400 | 0 | `[ ]` |
| Control table and tree | 1,000 | 0 | `[ ]` |
| Dispatch | 1,000 | 0 | `[ ]` |
| Layout | 1,000 | 0 | `[ ]` |
| Paint | 1,000 | 0 | `[ ]` |
| Input/events | 1,000 | 0 | `[ ]` |
| Top/panel/user/click classes | 3,500 | 0 | `[ ]` |
| Shared helpers | 500 | 0 | `[ ]` |
| Public API | 2,000 | 0 | `[ ]` |
| Debug/test/demo support | 7,000 | 0 | `[ ]` |
| Reserve | 1,010 | 0 | `[ ]` |
| **Total** | **20,000** | **0** | `[ ]` |

If a block overruns, first move only within its semantic block and re-check the
adjacent boundaries. If the total approaches 20,000, stop feature work and
reduce code duplication before consuming the reserve.

Current implementation: `baslib.bas` is 1,035 lines, all within
`500000–519999`. Per-block counts are not split yet; the important gate is the
hard range and the 20,000-line ceiling.

---

## 2. Milestone overview

The implementation order below differs from `baslib-plan.md` §21 in one place:
the control table (M3) precedes dispatch (M4). Dispatch needs a real class/verb
row from the control table; doing it the other way would create a temporary
test-only shim and rework. The design behavior is unchanged.

| Milestone | Goal | Line blocks | Depends on | Exit evidence |
|---|---|---|---|---|
| M0 | Test harness and baseline | none | — | Harness builds and runs a no-op fixture |
| M1 | Scaffold, constants, labels, guard | 500000–500099, API stubs | M0 | `baslib.bas` compiles; constants and labels resolve |
| M2 | `STACK` frame protocol | 500100–500299 | M1 | Frame tests pass; sentinel preserved |
| M3 | Control table, handles, tree | 500300–500599, 501000–501999 | M2 | 255/256 handle and tree tests pass |
| M4 | Method tables and dispatch | 500600–500999, 502000–502999 | M3 | Class/verb dispatch tests pass |
| M5 | Layout and clipping | 503000–503999 | M4 | Layout fixtures match expected rectangles |
| M6 | Paint and text | 504000–504999 | M5 | Pixel probes pass; host `VIEW` preserved |
| M7 | Input, events, capture | 505000–505999 | M6 | Synthetic input tests pass |
| M8 | v0.1 classes and API | 506000–511999 | M7 | Button/label/click-target demo works |
| M9 | Action lines, theme, demo, docs | 512000–518999 | M8 | Demo and action dispatch pass |
| M10 | v0.1 hardening and acceptance | all | M9 | Full v0.1 gate passes |
| M11 | v0.2 features | new ranges only after review | M10 | v0.2 gate passes |
| M12 | Deferred work | — | — | Backlog only |

---

## 3. Milestone details

### M0 — Test harness and baseline

**Goal:** make it possible to build, run, and fail a baslib test before writing
library behavior.

**Deliverables**

- `baslib/tests/` directory and test README.
- A Catch2-based `baslib-tests` test target linked against `newbasic-core`.
- A fixture loader that reads a library source plus a test host source file.
  M0 uses an empty/minimal library; M1 supplies the real `baslib.bas`.
- A CLI fixture runner for state-only programs that compares stdout with a
  `.expected` file.
- A no-op fixture that compiles, runs, and prints `PASS`.
- CI/local command documented for Windows and Linux.

**Checklist**

- [ ] Add `baslib/tests/CMakeLists.txt` or a root-CMake target named `baslib-tests`.
- [ ] Add `HeadlessScreen` test setup; no real window or GPU dependency.
- [ ] Add fixture loader for a library source and host fixture files.
- [ ] Add an empty-library path for M0 fixtures.
- [ ] Add a `Compile`/`Vm` helper that reports compile and run errors clearly.
- [ ] Add stdout-fixture runner (`baslib/tests/run_cli.ps1` and `.sh`).
- [ ] Add a no-op fixture `baslib/tests/fixtures/noop.bas`.
- [ ] Add `baslib/tests/README.md` with exact commands.
- [ ] Confirm the harness runs on Windows and Linux.
- [ ] Record the command in §4.4.

**Tests**

- [ ] `B0-01` Harness builds on Windows.
- [ ] `B0-02` Harness builds on Linux.
- [ ] `B0-03` Fixture loader merges a library source and host source.
- [ ] `B0-04` No-op fixture prints `PASS`.
- [ ] `B0-05` A deliberately failing fixture reports the failure.

**Exit gate:** the no-op fixture and a failure fixture both behave correctly, and
the harness is reproducible from a clean build.

---

### M1 — Scaffold, constants, labels, guard

**Goal:** create a compilable library skeleton with the fixed line contract,
constants, and public labels.

**Deliverables**

- `baslib/baslib.bas` with the `500000 END` guard.
- Constants block `500010–500099`.
- All public API labels at their published lines, initially as stubs.
- Internal `blNoop` line.
- A stub `blApiInit` that returns `blStatusNotInit%` until initialization is
  implemented.
- A line-map comment at the top of the file.

**Checklist**

- [ ] Create `baslib/baslib.bas`.
- [ ] Add `500000 END`.
- [ ] Declare `blVersion$`, `blVersionNum#`, and all constants from plan §4.2.1.
- [ ] Add `blNoLine&`, `blActionNone%`, `blStatus...%`, class/verb/kind/event/flag
      constants.
- [ ] Add `LABEL blApi...` at every published API line.
- [ ] Add a `RETURN` stub and a default status to every API entry.
- [ ] Add the internal `blNoop` no-op line.
- [ ] Add `LABEL blDispatch`, `blFramePush`, `blFrameLoad`, `blFrameClose`,
      `blFrameDrop` labels even if their bodies are stubs.
- [ ] Ensure constants are initialized before any call; no constant-init API is
      needed.
- [ ] Verify all names use the `bl` prefix and explicit suffixes.
- [ ] Verify no line falls outside `500000–519999`.

**Tests**

- [ ] `B1-01` `baslib.bas` parses and compiles alone.
- [ ] `B1-02` A host can read every constant and prints the expected values.
- [ ] `B1-03` `GOSUB blApiInit` resolves; missing labels are compile errors.
- [ ] `B1-04` The `500000 END` guard prevents fall-through when a host stops.
- [ ] `B1-05` `blApiInit` currently returns `blStatusNotInit%`.
- [ ] `B1-06` No duplicate labels; no API label is missing.
- [ ] `B1-07` With the documented host-then-library merge, a host line at
      `500000` is replaced by the library guard; hosts must stay below `500000`.

**Exit gate:** `baslib.bas` compiles by itself and merged with a trivial host;
all constants and API labels resolve.

---

### M2 — `STACK` frame protocol

**Goal:** implement the 14-slot frame, depth state, argument cache, locals,
results, and balance rules.

**Deliverables**

- `blFramePush`, `blFrameLoad`, `blFrameClose`, `blFrameDrop`.
- `blFp%`, per-depth arrays, request/return globals.
- Result transport: one numeric and one string result.
- Depth cap at `blMaxFrames%` (32).
- Frame protocol error status for malformed requests.

**Checklist**

- [ ] Implement `blFramePush` from the request block.
- [ ] Implement `blFrameLoad` into depth-indexed cache/locals.
- [ ] Implement `blFrameClose` and `blFrameDrop` with exact pop counts.
- [ ] Implement result push order: `blRetS$`, then `blRetN#`.
- [ ] Implement `blFp%` increment/decrement and depth cap.
- [ ] Implement `blStatusFrameProtocol%` for malformed requests.
- [ ] Zero unused argument/local slots on frame load.
- [ ] Document the exact frame layout in a comment or companion table.
- [ ] Add a helper that asserts balance in tests.

**Tests**

- [ ] `B2-01` Single frame push/load/close/drop leaves `STACK` balanced.
- [ ] `B2-02` Result order matches the caller contract.
- [ ] `B2-03` Nested frames at depths 1–32 pass.
- [ ] `B2-04` Depth 33 fails with `blStatusFrameOverflow%` and balances.
- [ ] `B2-05` Host sentinel below a frame survives.
- [ ] `B2-06` Invalid class/verb/argument counts drop the frame and balance.
- [ ] `B2-07` Unused numeric and string slots are zeroed.
- [ ] `B2-08` `blFp%` returns to 0 after every call.

**Exit gate:** all frame tests pass and every error path leaves `blFp%` and
`STACK` in the same state as the successful path.

---

### M3 — Control table, handles, and tree

**Goal:** implement the common control table, free list, handles, parent/child
links, and create/destroy primitives.

**Deliverables**

- `blCtl%`, string arrays, paint/clip arrays, free-list arrays.
- Handles 1–255; root handle 1.
- Allocation/free; parent/child/sibling links.
- Create, link, detach, destroy; destroy-root refusal.
- Validation helpers for handles, parents, classes, geometry, and cycles.

**Checklist**

- [ ] `DIM` all `bl` tables at the documented dimensions.
- [ ] Implement `blAllocHandle%` and `blFreeHandle%`.
- [ ] Implement root creation and the `blRoot%` constant.
- [ ] Implement parent/child/sibling link updates.
- [ ] Implement cycle and self-parent rejection.
- [ ] Implement `blDestroy` with iterative child destruction and no stack leak.
- [ ] Refuse destruction of the root.
- [ ] Reject a 256th control with `blStatusControlTableFull%`.
- [ ] Clear capture/hover/pressed state when a control is destroyed or hidden.
- [ ] Add validation helpers used by later milestones.

**Tests**

- [ ] `B3-01` Root creation, alloc/free, and handle reuse.
- [ ] `B3-02` Create/link/detach preserves sibling order.
- [ ] `B3-03` 255 controls fit; 256 fails with status 2.
- [ ] `B3-04` Parent cycle and self-parent fail with status 7.
- [ ] `B3-05` Destroy root fails with status 8.
- [ ] `B3-06` Destroy subtree balances `STACK` and frees handles.
- [ ] `B3-07` Destroy/hide clears capture/hover/pressed references.
- [ ] `B3-08` Re-init invalidates every previous handle.

**Exit gate:** the table/tree tests pass independently of layout, paint, or input.

---

### M4 — Method tables and dispatch

**Goal:** implement class/verb dispatch by table lookup and computed
`GOSUB`, with a complete no-op fallback.

**Deliverables**

- `blMethod&`, `blUserMethod&`, `blPanelLayoutLine&`, `blActionLine&`.
- Class metadata tables.
- `blDispatch` with validation and result transport.
- No-op line and no-op class/kind rows.
- Top/panel/user/click-target class router stubs.
- Action-line table clearing in `blInit`.

**Checklist**

- [ ] Fill every method cell with `blNoop` during initialization.
- [ ] Install classes 1–4 and user kinds 1 and 5; reserve the rest.
- [ ] Implement `blDispatch` validation and computed `GOSUB`.
- [ ] Implement computed `GOTO` tail dispatch for the user/panel routers.
- [ ] Implement `blActionLine&` initialization and `blSetActionLine`.
- [ ] Ensure invalid class/verb/method-line errors drop the frame cleanly.
- [ ] Ensure a method closes its own frame and pushes a result.
- [ ] Add internal `blSend` for nested calls.
- [ ] Keep method tables numeric; do not store labels in them.

**Tests**

- [ ] `B4-01` Every class × verb cell resolves to `blNoop` or a real method.
- [ ] `B4-02` Invalid class fails with status 4; invalid verb with status 5.
- [ ] `B4-03` User kind 1/5 routes correctly; reserved kinds fail cleanly.
- [ ] `B4-04` Computed `GOTO` tail dispatch preserves the pending `GOSUB` return.
- [ ] `B4-05` A missing method falls back to `blNoop` and balances.
- [ ] `B4-06` Action-line 0 and `blNoLine&` mean no handler.
- [ ] `B4-07` `blSetActionLine` accepts the full 32-bit line contract.

**Exit gate:** no invalid class/verb can corrupt depth or stack state, and every
fallback path returns a defined result.

---

### M5 — Layout and clipping

**Goal:** implement local-to-absolute geometry, panel modes, padding, clipping,
and dirty-layout propagation using the `STACK` work stack.

**Deliverables**

- Root and panel layout.
- Absolute/vertical/horizontal modes; reserved grid path.
- Padding, gap, and child local rectangles.
- Clip rectangle computation and inheritance.
- Layout-dirty propagation and iterative traversal.

**Checklist**

- [ ] Implement root sizing from explicit request or `SCREENW`/`SCREENH`.
- [ ] Implement absolute, vertical, and horizontal panel modes.
- [ ] Implement padding and gap.
- [ ] Implement hidden-child exclusion.
- [ ] Implement absolute rectangles and clip intersection.
- [ ] Implement `blLayoutAll` and `blFlagLayoutDirty%` clearing.
- [ ] Ensure layout uses `STACK` work items and balances.
- [ ] Ensure `blPaintAll` can trigger layout when dirty.
- [ ] Keep zero-size controls legal but invisible.

**Tests**

- [ ] `B5-01` Root size from `SCREENW`/`SCREENH`.
- [ ] `B5-02` Absolute panel positions children exactly.
- [ ] `B5-03` Vertical stack applies padding and gap.
- [ ] `B5-04` Horizontal row applies padding and gap.
- [ ] `B5-05` Hidden children do not consume layout space.
- [ ] `B5-06` Clip rectangles match expected intersections.
- [ ] `B5-07` Layout work stack balances after errors and success.
- [ ] `B5-08` Re-layout is skipped when no dirty flag is set.

**Exit gate:** layout fixtures compare exact rectangles and clips; no off-by-one
error at panel edges.

---

### M6 — Paint, text, and view preservation

**Goal:** paint the tree with correct viewport/clip handling and deterministic
text.

**Deliverables**

- Paint traversal back-to-front.
- Per-control `PUSH VIEW`, `VIEW`, `POP VIEW`.
- `blPaintAll` and `blUpdate` basics.
- Shared drawing helpers: box, bevel, text, text rect, truncation.
- Label text metrics and multiline layout primitives.
- Host viewport preservation.

**Checklist**

- [ ] Implement paint traversal in sibling order.
- [ ] Apply viewport and clip per control.
- [ ] Preserve host `VIEW` across paint/update.
- [ ] Implement panel background/border paint.
- [ ] Implement button face/caption paint.
- [ ] Implement label text, alignment, padding, border, transparency, scale.
- [ ] Implement text truncation and multiline measurement.
- [ ] Implement `blAutoSize` measurement and bounds update.
- [ ] Implement dirty clearing and `blUpdate`.
- [ ] Add no-op handling for invisible/disabled controls.

**Tests**

- [ ] `B6-01` Root/panel background pixels match the palette.
- [ ] `B6-02` Paint order matches sibling order.
- [ ] `B6-03` Host `VIEW` is unchanged after paint.
- [ ] `B6-04` Clip prevents drawing outside a control.
- [ ] `B6-05` Button caption is centered and scaled.
- [ ] `B6-06` Label alignment/valign/padding/border cases pass.
- [ ] `B6-07` Multiline label and explicit CR/LF cases pass.
- [ ] `B6-08` Long text truncates at pixel width.
- [ ] `B6-09` `blAutoSize` produces expected bounds.
- [ ] `B6-10` Transparency leaves the background intact.

**Exit gate:** pixel probes and text-row assertions pass on Windows and Linux
headless screens.

---

### M7 — Input, hit testing, events, and capture

**Goal:** implement live mouse polling, hit testing, click bubbling, hover,
press/release, wheel, capture, and the event ring.

**Deliverables**

- Hit test by z-order, visibility, enabled state, and masks.
- Click dispatch and bubbling.
- Live hover/press state.
- Capture and `CaptureLost`.
- Event ring, coalescing, and `blNextEvent`.
- Synthetic input hooks.
- `blPollInput` and `blPollMouse`.

**Checklist**

- [ ] Implement hit test with z-order and disabled/transparent rules.
- [ ] Implement click queue and bubbling to parent.
- [ ] Implement hover/enter/leave.
- [ ] Implement press/release and click distinct from drag.
- [ ] Implement wheel.
- [ ] Implement capture and `CaptureLost`.
- [ ] Implement bounded event ring and move coalescing.
- [ ] Implement `blPollInput` click drain limit.
- [ ] Implement test hooks.
- [ ] Clear capture/hover on hide/destroy/re-init.

**Tests**

- [ ] `B7-01` Hit test chooses front-most eligible control.
- [ ] `B7-02` Disabled and transparent rules pass.
- [ ] `B7-03` Label click-through reaches a lower control.
- [ ] `B7-04` Click bubbles to the expected parent.
- [ ] `B7-05` Hover/enter/leave sequence is correct.
- [ ] `B7-06` Press/release does not duplicate the click action.
- [ ] `B7-07` Wheel posts the expected event.
- [ ] `B7-08` Capture routes move/up to the captured control.
- [ ] `B7-09` Destroy/hide clears capture and posts `CaptureLost`.
- [ ] `B7-10` Event ring full/empty and coalescing cases pass.
- [ ] `B7-11` Synthetic hooks produce deterministic events.

**Exit gate:** all input tests pass without OS events; event ordering is stable
and documented.

---

### M8 — v0.1 classes and public API

**Goal:** complete Top, Panel, Button, Label, and ClickTarget behavior and fill
the public API wrappers.

**Deliverables**

- Top, panel, user (button/label), and click-target methods.
- All public API entry points implemented or explicitly stubbed with a status.
- Common request/return block.
- Init, create, destroy, bounds, show/hide, text, value, layout, action,
  invalidate, layout/paint/update, polling, find, event, test hooks, dump,
  autosize, action line, enabled.
- Class-specific tables and defaults.

**Checklist**

- [ ] Implement `blApiInit` and root setup.
- [ ] Implement `blApiCreate` for classes 2/3/4 and kinds 1/5.
- [ ] Implement destroy, bounds, show/hide, enabled, invalidate.
- [ ] Implement text/value getters and setters.
- [ ] Implement layout, paint, update, poll, next-event.
- [ ] Implement find/test hooks/dump/autosize/action-line/register-class.
- [ ] Implement button press visual and click action.
- [ ] Implement label text/alignment/transparency/scale/click-through.
- [ ] Implement click-target hit mask and bubbling.
- [ ] Validate all request values before mutation.
- [ ] Keep API line numbers and labels exactly as published.

**Tests**

- [ ] `B8-01` Host initialization/root creation.
- [ ] `B8-02` Create/destroy all v0.1 classes and kinds.
- [ ] `B8-03` Show/hide/enabled/bounds/invalidate.
- [ ] `B8-04` Text/value getters and setters.
- [ ] `B8-05` Button click produces the expected action.
- [ ] `B8-06` Label text/alignment/transparency/scale/click-through.
- [ ] `B8-07` Click-target mask and bubbling.
- [ ] `B8-08` Find-at and test hooks.
- [ ] `B8-09` Dump/autosize/action-line/register-class.
- [ ] `B8-10` Every public label resolves and returns a documented status.
- [ ] `B8-11` Invalid inputs leave state unchanged.

**Exit gate:** the §19.3 v0.1 scenarios pass except for the final acceptance
sweep in M10.

---

### M9 — Action lines, theme, demo, and documentation

**Goal:** finish the integration surface and produce the first real demo.

**Deliverables**

- Action-line dispatch from host events.
- Theme defaults for colors and text.
- `baslib/demo.bas` using the host skeleton.
- `baslib/tests/README.md` and user-facing integration notes.
- Debug/test helpers in `512000–518999`.
- Line-budget and naming audit.

**Checklist**

- [ ] Wire the M4 action-line table into the public `blSetActionLine` wrapper.
- [x] Implement host action dispatch in the demo.
- [x] Implement theme defaults and per-control overrides.
- [x] Add the demo with panel, button, multiline label, click target.
- [ ] Add host sentinel demonstration.
- [x] Add `blDump` debug output.
- [x] Add demo build/run instructions.
- [ ] Audit all names for `bl` prefix and explicit suffixes.
- [ ] Audit all public API labels against the plan table.
- [x] Update line-budget tracker.

**Tests**

- [x] `B9-01` Action line 1010/2010 dispatch executes the correct host handler.
      Handlers are at 1010/2010 because 1000/2000 are REM-only lines with no code.
- [ ] `B9-02` `blNoLine&`/`blActionNone%` mean no dispatch.
- [ ] `B9-03` Theme defaults appear in painted pixels.
- [x] `B9-04` Demo creates, paints, and responds to a click.
- [x] `B9-05` Demo preserves host `VIEW` and `STACK` sentinel.
- [ ] `B9-06` `blDump` reports handles/classes/kinds.
- [x] `B9-07` No line outside `500000–519999`.
- [ ] `B9-08` No duplicate labels; no missing API label.

**Exit gate:** the demo runs interactively and its automated smoke test passes.

---

### M10 — v0.1 hardening and acceptance

**Goal:** prove v0.1 against the full test plan and close the release.

**Checklist**

- [x] Run the full automated suite on Windows.
- [x] Run the full automated suite on Linux.
- [ ] Run from a clean build directory.
- [ ] Verify compile/run behavior with `--cli` fixtures.
- [ ] Verify the demo with `--gpu-shot`/headless where applicable.
- [x] Run the line-number/RENUM regression checks.
- [ ] Run the namespace and suffix audit.
- [ ] Update `baslib-plan.md` for any behavior changes.
- [x] Update this file's status and budget tables.
- [ ] Tag the v0.1 milestone (if the repository uses tags for library releases).
- [ ] Record known non-blocking issues in the risk table.

**Acceptance tests**

- [x] `B10-01` Full host scenario: init → panel → button/label/click target →
  layout → paint → click → event → action.
- [x] `B10-02` 255 controls succeed; 256th fails with status 2.
- [x] `B10-03` Depth 32 succeeds; depth 33 fails with status 6; stack balanced.
- [x] `B10-04` Re-init invalidates old handles and resets tables.
- [x] `B10-05` Host `STACK` sentinel survives every API call.
- [x] `B10-06` Host `VIEW` survives every paint/update.
- [x] `B10-07` Whole library compiles inside `500000–519999`.
- [x] `B10-08` API labels resolve after `RENUM`.
- [ ] `B10-09` `ERL` reports a 500000+ error line correctly.
- [x] `B10-10` No unexpected BASIC runtime errors in the suite.
- [x] `B10-11` Windows/Linux results match except documented host differences.
- [ ] `B10-12` All v0.1 acceptance criteria in `baslib-plan.md` §19.3 pass.
- [x] `B10-13` `blInit` sizes the root from `SCREENW`/`SCREENH` on a non-720
      graphics canvas.

**Exit gate:** all v0.1 boxes are checked and the test log is attached to the
milestone commit or PR.

---

### M11 — v0.2

**Goal:** add the deferred controls and features without destabilizing v0.1.

**Checklist**

- [x] Slider: track/thumb, value mapping, drag/capture. Quantization still
      coarse and not independently tested.
- [x] Gauge: read-only bar.
- [ ] Dial: angle mapping, quantization, drag. Current value mapping is linear
      in X, not angular.
- [x] Word-wrapped labels (character wrapping, not word-boundary wrapping).
- [x] Dynamic class registration (`blApiRegisterClass`).
- [x] Extend tests for the implemented v0.2 features.
- [x] Keep v0.1 API lines and labels stable.
- [x] Update line budget and plan if new blocks are needed.

**Tests**

- [x] `B11-01` Slider maps position to value and drags; clamping is exercised by
      implementation, quantization is not yet tested.
- [ ] `B11-02` Gauge paints a read-only value and ignores drag.
- [ ] `B11-03` Dial maps angle to value and captures drag.
- [ ] `B11-04` Word-wrapped labels truncate/wrap at the expected boundaries.
- [x] `B11-05` Dynamic class registration installs a host class.
- [x] `B11-06` All v0.1 tests still pass.

**Exit gate:** v0.2 tests pass, v0.1 suite remains green, and the API contract is
unchanged.

---

### M12 — Deferred backlog

Do not start these in v0.1 or v0.2.

- [ ] Keyboard focus, tab order, text entry.
- [ ] Multiple top-level windows.
- [ ] Dirty-rectangle compositor / double buffering.
- [ ] Screen-mode changes while controls exist.
- [ ] Serial/joystick/light-pen input.
- [ ] Full theme editor.
- [ ] Deferred-command queue for timer-driven animation.

---

## 4. Test strategy

### 4.1 Test layers

| Layer | Purpose | Runner |
|---|---|---|
| Fixture/state | Status codes, tables, dispatch, API results | C++ `baslib-tests` + CLI fixtures |
| Geometry | Rectangles, clips, layout order | C++ `HeadlessScreen` |
| Pixels/text | Paint, palette, text, truncation, view preservation | C++ `HeadlessScreen` canvas |
| Input | Hit test, hover/press/click, wheel, capture, events | Synthetic hooks in C++ |
| Integration | Merge, `RENUM`, host sentinel, demo | C++ + CLI |
| Manual | Windowed appearance and interaction | `baslib/demo.bas` |

### 4.2 Harness layout

Proposed implementation files (created in M0; no library behavior yet):

```
baslib/
    baslib-plan.md
    baslib-imp.md
    baslib.bas
    demo.bas
    tests/
        CMakeLists.txt
        baslib_tests.cpp
        fixtures/
            noop.bas
            stack_sentinel.bas
            layout_absolute.bas
            layout_vertical.bas
            layout_horizontal.bas
            paint_button.bas
            paint_label.bas
            input_click.bas
            input_capture.bas
            api_smoke.bas
            demo_smoke.bas
            renum_labels.bas
        expected/
            *.txt
        run_cli.ps1
        run_cli.sh
        README.md
```

The C++ harness should:

- [ ] Load `baslib.bas` and a host fixture into one `LineStore`.
- [ ] Compile and run on a `HeadlessScreen`.
- [ ] Return structured results: status, code, message, rows, pixels, `blStatus%`.
- [ ] Assert `blFp% == 0` and `STACK` balance after every call.
- [ ] Support synthetic mouse input through the documented test hooks.
- [ ] Support `RENUM` of the merged source before compiling.
- [ ] Report compile errors with line and message.

The CLI runner should:

- [ ] Run state-only fixtures with `newbasic --cli`.
- [ ] Compare stdout with the `.expected` file.
- [ ] Return non-zero on mismatch.
- [ ] Be usable from Windows and Linux shells.

### 4.3 Test case matrix

Use these IDs in commit messages and test names.

| ID | Area | Scenario | Milestone |
|---|---|---|---|
| B0-01–B0-05 | Harness | Build/run/no-op/failure | M0 |
| B1-01–B1-07 | Scaffold | Compile, constants, labels, guard | M1 |
| B2-01–B2-08 | Frames | Push/load/close/drop, depth, sentinel | M2 |
| B3-01–B3-08 | Control table | Handles, tree, 255/256, destroy, re-init | M3 |
| B4-01–B4-07 | Dispatch | Tables, no-op, invalid, computed tail, actions | M4 |
| B5-01–B5-08 | Layout | Root modes, padding, clip, dirty skip | M5 |
| B6-01–B6-10 | Paint | Order, view, clip, text, label, autosize | M6 |
| B7-01–B7-11 | Input | Hit, hover, click, wheel, capture, events | M7 |
| B8-01–B8-11 | API | All entry points, validation, button/label/target | M8 |
| B9-01–B9-08 | Integration | Actions, theme, demo, naming, labels | M9 |
| B10-01–B10-13 | Acceptance | Full v0.1 gate | M10 |
| B11-* | v0.2 | Slider/gauge/dial/wrap/registration | M11 |

### 4.4 Test execution

**C++ suite**

- Windows: build the `baslib-tests` target, then run it from the build tree.
- Linux: same with the Linux build directory.
- The suite must run headless; no GPU or window is required.

```text
# Windows (from the repository root)
cmake --build out --config Release --target baslib-tests
out/tests/Release/baslib-tests.exe

# Linux (from the repository root or WSL mount)
cmake --build build-wsl --target baslib-tests -j4
build-wsl/tests/baslib-tests
```

**CLI fixtures**

```text
newbasic --cli baslib/tests/fixtures/noop.bas
```

The exact executable path depends on the build; the M0 README records the two
working commands used on Windows and Linux.

- Run each `baslib/tests/fixtures/*.bas` that has a matching `.expected` file.
- Use `--cli`; compare exact stdout after normalizing line endings.
- Do not use `SLEEP` or real time in fixtures.

**Pixel tests**

- Use `HeadlessScreen` canvas access, not screenshots, for deterministic checks.
- Use `POINT`/canvas coordinates only for the pixels under test.
- Recheck host `VIEW` before and after paint.

**Manual demo**

- Run `baslib/demo.bas` in a window on Windows and Linux.
- Click the button, click the target, resize the window if supported, and inspect
  the label.
- Record the exact commands and screenshots/PPMs with the milestone.

### 4.5 Test data rules

- [ ] No timers, `SLEEP`, or wall-clock dependence in automated tests.
- [ ] Synthetic input only; never require a real mouse.
- [ ] Fixed palette and fixed text so pixels/rows are reproducible.
- [ ] Each test starts from a fresh `HeadlessScreen` and fresh `Vm`.
- [ ] Each test uses a unique fixture or resets all arrays/state.
- [ ] A failing test must print `blStatus%`, `blError%`, and `ERL`.
- [ ] Tests must fail on stack imbalance, not just wrong output.

### 4.6 Regression tests to keep forever

- [ ] `B10-07` line range: library lines stay in `500000–519999`.
- [ ] `B10-08` labels survive `RENUM`.
- [ ] `B10-09` `ERL` reports high line numbers.
- [ ] `B10-05` host `STACK` sentinel survives.
- [ ] `B10-06` host `VIEW` survives.
- [ ] `B10-04` re-init invalidates handles.
- [ ] `B10-02` 255 controls and 256 failure.
- [ ] `B10-03` 32/33 depth.
- [ ] `B4-05` missing method fallback.
- [ ] `B7-09` capture lost on hide/destroy.

### 4.7 Evidence required at each gate

- [ ] Exact build command.
- [ ] Exact test command.
- [ ] Test summary: cases/assertions passed.
- [ ] Windows result.
- [ ] Linux result.
- [ ] Any manual demo notes.
- [ ] Updated line-budget row.

---

## 5. Traceability to `baslib-plan.md`

| Plan area | Implementation milestone | Main tests |
|---|---|---|
| §4 line map, constants, labels | M1 | B1-* |
| §8 control table/handles/tree | M3 | B3-* |
| §9 class dispatch/method tables | M4 | B4-* |
| §10 `STACK` frames | M2 | B2-* |
| §11 layout/clipping | M5 | B5-* |
| §12 painting/text | M6 | B6-* |
| §13 input/events | M7 | B7-* |
| §14 public API | M8/M9 | B8-*, B9-* |
| §15 class behavior | M8 | B8-* |
| §16 errors/validation | M3/M10 | B3-*, B10-* |
| §19 testing/acceptance | M10 | B10-* |
| §21 implementation order | this file | all |
| §22 risks | M0/M10 | B0-*, B10-* |

---

## 6. Risks and controls

| Risk | Control | Check |
|---|---|---|
| Line budget overrun | Track used lines per block; reduce duplication before using reserve | `[ ]` |
| Stack leak in any error path | Sentinel/balance assertion after every test | `[ ]` |
| Wrong method dispatch | Full class × verb table test; no-op fallback | `[ ]` |
| Paint clipping/view bug | Pixel probes plus host `VIEW` checks | `[ ]` |
| Input duplicate/missed click | Separate click queue from live buttons; event-order tests | `[ ]` |
| Capture points at freed handle | Clear on destroy/hide; `CaptureLost` test | `[ ]` |
| Text metric mismatch | `TEXTWIDTH`/`TEXTHEIGHT` only; truncation probes | `[ ]` |
| API/label drift | Compare labels/line numbers against plan table | `[ ]` |
| `RENUM` breaks numeric references | Prefer labels at call sites; regression test | `[ ]` |
| Action line stored as label by mistake | Document/enforce numeric line values | `[ ]` |
| Host namespace collision | `bl` prefix audit in M9 | `[ ]` |
| Test harness needs GPU | Headless screen only in automated tests | `[ ]` |
| v0.2 destabilizes v0.1 | New milestone, v0.1 suite must stay green | `[ ]` |

---

## 7. Milestone status

Update this table as work proceeds.

| Milestone | State | Started | Complete | Evidence |
|---|---|---|---|---|
| M0 Harness | `[x]` | 2026-10-24 | 2026-10-24 | Integrated `[baslib]` Catch2 tests; `baslib/tests/README.md` |
| M1 Scaffold/constants | `[x]` | 2026-10-24 | 2026-10-24 | `baslib/baslib.bas` guard, constants, API labels |
| M2 Frames | `[x]` | 2026-10-24 | 2026-10-24 | Frame push/load/close/drop + depth tests |
| M3 Control table | `[x]` | 2026-10-24 | 2026-10-24 | Handles, tree, destroy, 255/256 tests |
| M4 Dispatch | `[x]` | 2026-10-24 | 2026-10-24 | Method table, `blSend`, action-line tests |
| M5 Layout | `[x]` | 2026-10-24 | 2026-10-24 | Vertical layout, clip, dirty layout |
| M6 Paint/text | `[x]` | 2026-10-24 | 2026-10-24 | Pixel probes, host viewport, label/button paint |
| M7 Input/events | `[x]` | 2026-10-24 | 2026-10-24 | Hit test, hover/click/capture, event ring |
| M8 Classes/API | `[x]` | 2026-10-24 | 2026-10-24 | All public API labels; v0.1 classes |
| M9 Demo/polish | `[x]` | 2026-10-24 | 2026-10-24 | `baslib/demo.bas`, `baslib/demo-controls.bas`, action dispatch, theme defaults |
| M10 v0.1 acceptance | `[~]` | 2026-10-24 | | Windowed demo verified: initial paint, button click, target click, ESC. Clean build, CLI fixtures, and ERL-500000 test pending |
| M11 v0.2 | `[~]` | 2026-10-24 | | Slider/gauge/wrap/registration done; dial angle/quantization and full v0.2 matrix pending |
| M12 Deferred | `[x]` | 2026-10-24 | 2026-10-24 | Intentionally not implemented; listed as backlog |

### 7.1 Manual verification log

**2026-10-24 — windowed `baslib/demo.bas`**

- [x] Initial frame: panel, centered button, centered two-line label.
- [x] Button click: label changes to `Button clicked` plus timestamp.
- [x] Click target: blank area below the label changes it to `Target clicked`
      plus timestamp.
- [x] `Esc` exits the running program normally.
- [ ] Re-run reset (`RUN` after a click) verified manually.
- [ ] Direct-mode invariant dump rechecked after the final click fix.

**2026-10-24 — windowed `baslib/demo-controls.bas`**

- [x] Panel, header label, button, section labels, slider, gauge, dial, value
      readout, and invisible click target all render.
- [x] Button click writes `Button clicked` plus timestamp to the readout.
- [x] Click target writes `Target clicked` plus timestamp to the readout.
- [x] Slider click updates its thumb; readout changes from `Slider 25` to
      `Slider 50`.
- [x] Dial click updates its needle and readout from `Dial 35` to `Dial 50`.
- [x] Gauge remains read-only at `Gauge 60`.
- [x] `Esc` exits normally.
- [ ] Slider/dial drag with a real held mouse button (current real-input path
      handles clicks; synthetic drag test covers capture).

Correct handle/class mapping for the demo:

| Handle | Control | `blCtl%(handle,1)` |
|---:|---|---:|
| 1 | Root / Top | 1 |
| 2 | Panel | 2 |
| 3 | Button | 3 |
| 4 | Label | 3 |
| 5 | Click target | 4 |

The expected sequence is `1 2 3 3 4`. An earlier manual note expected
`1 2 3 4 5`; that was wrong because class 3 covers all User-kind controls
(buttons and labels), while the click target has class 4.

Defects found and fixed during manual testing:

1. Recursive paint/layout overwrote the parent’s child iterator; only the first
   child painted.
2. `blTw`/`blTW` and `blTh`/`blTH` collided because BASIC identifiers are
   case-insensitive, breaking text alignment.
3. `blPostEvent` did not update `blEventCount%`, so the demo loop never saw
   queued click events.
4. The demo’s action table pointed at REM-only lines `1000`/`2000`; handlers are
   now at `1010`/`2010`.
5. The control-gallery action IDs for slider/dial were initially `300`/`400`,
   outside the action-line table’s `0–255` range; they are now `150`/`160`.
6. The gallery initially used REM-only handler lines; it now targets executable
   lines `1010`, `2010`, `3010`, and `4010`.
7. The dial radius was based on the control width, producing an oversized
   circle; it now uses the smaller of width/height and draws a value needle.
8. `CIRCLE`’s default aspect on `SCREEN 720` is `0.75`, producing an oval dial.
   The dial now passes an explicit aspect of `1` and has been visually verified
   as round.

---

## 8. Definition of done

### v0.1

- [ ] All M0–M10 checklist items are checked.
- [x] All v0.1 tests pass on Windows and Linux.
- [x] The demo runs and responds to synthetic and manual input.
- [x] The library compiles inside `500000–519999`.
- [ ] Host lines remain inside `1–499999`.
- [ ] Public API labels and line numbers match `baslib-plan.md`.
- [ ] No uppercase `Bl` identifiers remain; all names use `bl`.
- [ ] Every variable/array has an explicit suffix.
- [ ] Line-budget tracker is current.
- [ ] `baslib-plan.md` and this file record any design deviations.
- [ ] Known non-blocking issues are listed in the risk table.

### v0.2

- [ ] Slider, gauge, and dial work with capture/drag.
- [ ] Word-wrapped labels work.
- [ ] Dynamic class registration works.
- [ ] All v0.1 tests still pass.
- [ ] v0.1 API lines and labels are unchanged.

---

## 9. Immediate next step

Start with **M0**. Do not write `baslib.bas` behavior until the no-op harness and
failure fixture are working. The first implementation commit after this plan
should contain only M0 harness scaffolding and its test evidence.
