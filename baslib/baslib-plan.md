# baslib — a generic BASIC control framework

**Status:** Implemented 0.2 — shipped in [`baslib.bas`](baslib.bas).  
**Target:** newbasic (GW-BASIC-compatible dialect in this repository)  
**Implementation plan:** [`baslib-imp.md`](baslib-imp.md)  
**Programmer reference:** [`baslib-prog-ref.md`](baslib-prog-ref.md)  
**Last updated:** 2026-10-24

This document records the design agreed so far. The existing sketches are in [`stackish.bas`](../samples/baslib/stackish.bas) and [`windows.bas`](../samples/baslib/windows.bas); they are prototypes, not the final implementation. The real library, demo, tests, and this plan live in the top-level `baslib/` directory.

> **Draft 0.2.** newbasic now has computed `GOTO`/`GOSUB`, a user-visible
> `STACK`, `SELECT CASE`, `SLEEP`, live mouse state, `TEXTWIDTH`/`TEXTHEIGHT`,
> `SCREENW`/`SCREENH`, viewport queries, `PUSH VIEW`/`POP VIEW`, symbolic
> `LABEL` targets, read-only `CONST` values, and signed 32-bit integers and line
> numbers. These remove the old static-dispatch, click-only, fixed-screen and
> text-cell constraints. The frame stack, dispatch, input, painting and layout
> sections below have been redesigned around them. Draft 0.1 details that no
> longer apply were removed.

---

## 1. Objective

Create a reusable BASIC source library that manages a tree of graphical controls:

- a single top-level control that owns the screen and receives mouse input;
- panels that contain other controls and determine their layout;
- user controls that paint themselves (v0.1: button and label; v0.2: slider, gauge, dial);
- click targets that respond to mouse input.

Each control has a class. Instances have independent state, stored in arrays.
The shipped 0.2 library uses `SUB`/`FUNCTION` procedures with per-call `LOCAL`
variables for layout, painting, recursion, and text helpers. It does **not**
use a frame protocol or class-handler dispatch; the public API uses request and
result globals and the `blApi...` labels. The original design's `STACK`/computed
`GOSUB` sections are retained as historical rationale, not as the implemented
control flow.

> **Implementation note (2026-10-24):** where this document and
> [`baslib-prog-ref.md`](baslib-prog-ref.md) disagree, the programmer reference
> and `baslib.bas` are authoritative. The frame, dispatch, and class-router
> sections below are design history.

The library is a *source fragment*, not an interpreter extension. A host
program merges it with its own source and calls labelled entry points backed by
stable line numbers.

### Non-goals for v0.1

- A general-purpose window manager or multiple top-level windows.
- Keyboard focus, tab navigation, and text entry (reserved for a later revision).
- A full constraint solver or CSS-like layout engine.
- Pixel-perfect native widgets.
- Word-wrapped labels. Explicit multiline text (CR/LF) and auto-size are in v0.1; wrapping is v0.2.
- Slider, gauge, and dial in v0.1. The class, value storage, and drag infrastructure support them; v0.1 proves the button, label, and click target first.
- Dynamic class registration in v0.1. The method table makes it a data operation; it is a v0.2 feature.
- Double buffering or a general damage-region compositor.
- A full theme editor.
- Changing `SCREEN` mode while controls exist. `blInit` supports any graphics canvas that is already selected.

---

## 2. Newbasic capabilities and constraints

These are facts from the current implementation and reference manual. They drive the design.

| Capability / constraint | Consequence |
|---|---|
| No `INCLUDE` or import statement. `MERGE` is direct-mode only. | baslib is a plain source fragment with fixed line numbers. It is merged or concatenated before `RUN`. |
| `GOSUB` accepts a literal line, a `LABEL`, or a numeric expression. Literal/label targets keep compile-time resolution. | The public API is a line-number contract plus symbolic labels, but class/verb dispatch can fetch a method line from a table and `GOSUB` it. |
| `GOTO` accepts a literal line, a `LABEL`, or a numeric expression. `GOTO` out of an active `SELECT CASE` is a runtime error. | Computed `GOTO` is used for tail dispatch inside a method (the pending `GOSUB` return is preserved). Do not use it to leave a `SELECT CASE`; use computed `GOSUB` where a return is needed. |
| `LABEL name` declares a symbolic line target; labels are case-insensitive, compile-time, and survive `RENUM`. | Public API entry points get labels; host code uses `GOSUB blApiCreate` instead of a literal line. Method tables still store numeric lines because computed `GOSUB` needs a value. |
| `CONST name = literal` declares a read-only global initialized before the first instruction. | Version, limits, status codes, enum IDs, and flag bits are `CONST`; host or library code cannot overwrite them. |
| `STACK PUSH` / `STACK POP` / `STACK TOP` / `STACK PEEK` maintain one global LIFO stack of BASIC values. Whole arrays are copied by value. List operations are all-or-nothing. There is no stack-depth query. | baslib uses `STACK` as its call-frame transport and work stack. It must keep exact push/pop discipline and must never assume the stack is otherwise empty. |
| `STACK` is cleared by `CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN`, but is preserved between direct-mode commands and is not touched by `GOSUB`/`RETURN`. | A host may use `STACK` too. baslib frames must be balanced and transparent to values below them. `blInit` is required after any operation that clears the stack. |
| `STACK` underflow raises **Stack empty** (error 77). The stack is capped at 1,000,000 entries and elements. | Frame and work-stack code validates its own counts; it never relies on underflow as control flow. |
| `ON expression GOSUB` still selects from a static list, but it is no longer required for dispatch. | baslib uses data-driven method tables, not per-class `ON` lists. Class and verb IDs are bounded by table dimensions, not by the 255-entry `ON` limit. |
| `SELECT CASE` supports value lists, `CASE IS`, `CASE ELSE`, and nesting. `CASE 1 TO 5` is not implemented. | `SELECT CASE` is used for validation, event state, alignment, and other small enum logic. Dispatch stays table-driven. |
| Newbasic has an internal `GOSUB`/`RETURN` stack, so nested calls work. It has no user-defined local variables. | The `STACK` frame plus depth-indexed local arrays supply arguments, locals, and results. GOSUB nesting itself is not the problem. |
| Arrays are 0-based unless `OPTION BASE 1` is in effect. There is one program-wide base. | baslib requires base 0 and reserves index 0 as “null”. The host must not use `OPTION BASE 1`. |
| `DIM` is executable and re-`DIM` resets an array. | Initialization is explicit through `blInit`. The library is not self-starting. |
| Numeric arrays and string arrays are distinct types. | Common numeric state and text state live in separate arrays. |
| Variables are global unless inside a `DEF FN` expression, and `DEFINT`/`DEFSNG`/`DEFSTR` can change defaults. | Every baslib variable and array gets an explicit type suffix. No baslib code relies on the host’s default type. |
| Strings are limited to 255 characters. | Captions, names, and per-control text are validated/truncated at 255. |
| `DRAWTEXT` draws CP437 text at arbitrary graphics coordinates with transparent ink, `VIEW` clipping, CR/LF line breaks, and scale 1–8. | All control text uses pixel text. Labels may be multiline without a text-grid conversion. |
| `TEXTWIDTH` and `TEXTHEIGHT` return pixel metrics for `DRAWTEXT`. | Text centering, truncation, multiline layout, and auto-size use exact metrics rather than `LEN(text$) * 8`. |
| `SCREENW` and `SCREENH` return the current graphics canvas size. | The root defaults to the canvas, not a hard-coded 1280×720. Any graphics mode is usable. |
| `VIEW` clips graphics to a viewport and makes coordinates relative to it. `VIEWX1`/`Y1`/`X2`/`Y2` report the current viewport. `PUSH VIEW`/`POP VIEW` save and restore it. | Painting sets `VIEW` per control with `PUSH VIEW`/`POP VIEW`, so a host viewport is preserved exactly. The old “host must not keep a VIEW” rule is gone. |
| Live mouse state is available through `MOUSEPOSX`/`MOUSEPOSY`, `MOUSEBUTTONS`, and `MOUSEWHEEL`. `MOUSEB` still drains a queued click and `MOUSEX`/`MOUSEY` remain latched to that click. | Input supports hover, enter/leave, press/release, wheel, and (v0.2) drag. Clicks remain the action mechanism; live state drives visuals and continuous controls. |
| `SLEEP milliseconds` blocks through the host. Timer events are not serviced during the sleep. | The host loop uses short sleeps for pacing and animation. baslib never sleeps inside paint or dispatch. |
| `COMMON` of arrays is refused and `CHAIN MERGE` is unsupported. | The library and host must be one merged program; state cannot be handed to a separate chained program. |
| `RENUM` renumbers the whole merged program; newbasic now accepts signed 32-bit line numbers. | Do not run `RENUM` if fixed API *numbers* matter; symbolic `LABEL` calls survive, but baslib still keeps static lines at or below 65529. |

### 2.1 What “no user frames” means now

Newbasic still has no user-defined local variables: every scalar is global. A handler that calls another handler would clobber ordinary globals. baslib therefore supplies a call protocol:

- a fixed-size frame is pushed on `STACK`;
- the dispatcher peeks metadata, selects a method line, and calls it with computed `GOSUB`;
- the method loads its arguments into depth-indexed cache arrays, uses depth-indexed locals for state that must survive a nested call, and closes its frame with one result pair.

The language `GOSUB` stack handles return addresses. `STACK` handles data.

---

## 3. Accepted decisions

| Area | Decision |
|---|---|
| Packaging | One self-contained `.BAS` fragment in lines **500000–519999**; host owns **1–499999**. Merged with `MERGE` or concatenated at build time. |
| Instance model | One global handle space (1–255). Common fields in a shared control table; class-specific state in per-class tables indexed by the same handle; text in separate `$` arrays. |
| Dispatch | Data-driven method table: `blMethod&(class, verb)` stores a line number; `GOSUB blMethod&(class, verb)`. No per-class static `ON` lists. |
| User kinds | Separate `blUserMethod&(kind, verb)` table, reached by computed `GOTO` tail dispatch from the User class router. |
| Call protocol | Fixed 14-slot frame on `STACK`, `blFp%` depth counter, depth-indexed argument cache and locals, one numeric/one string result. |
| Geometry | Child coordinates are local to the parent. A layout pass computes absolute rectangles and clip rectangles. |
| Layout modes | Absolute, vertical stack, and horizontal row in v0.1; grid is reserved. |
| Click delivery | Bubble from the hit control to its parent until one handles the click; the root is last. |
| Input | Click, hover, enter/leave, press/release, and wheel in v0.1. Drag/capture is in the core; continuous controls use it in v0.2. |
| Text | `DRAWTEXT` plus `TEXTWIDTH`/`TEXTHEIGHT`; labels support explicit multiline and auto-size. Word wrap is v0.2. |
| Display | Any graphics canvas selected before `blInit`; root defaults to `SCREENW`/`SCREENH`. |
| Painting | Per-control `PUSH VIEW` / `VIEW clip` / `POP VIEW`; the host viewport is preserved. |
| v0.1 controls | Top-level, panel, button, label, and click target. Slider, gauge, and dial are v0.2. |
| Action dispatch | Host action IDs plus an optional `blActionLine&(action)` line table and `blSetActionLine`; the host can `GOSUB` the line directly. |
| Source location | Top-level `baslib/` directory for library, demo, tests, and this plan. `samples/baslib/` keeps the prototypes. |

---

## 4. Source, line numbers, and merge contract

### 4.1 File layout

```
baslib/
    baslib-plan.md          this document
    baslib-imp.md           implementation milestones and test checklist
    baslib.bas              the library fragment (500000-519999)
    demo.bas                a host program (1-499999)
    demo-controls.bas       all-control gallery host program (1-499999)
    tests/                  BASIC test programs and expected output

samples/baslib/
    stackish.bas            early manual-stack prototype; superseded by STACK
    windows.bas             early control-table sketch; superseded by baslib.bas
```

`samples/baslib/` keeps the prototypes for reference. The final library is not a runnable program by itself: it has no main.

### 4.2 Line-number map

| Lines | Contents |
|---|---|
| 1–499999 | Host program. baslib never uses these. |
| 500000 | Hard `END` guard. If host execution falls through 499999, it stops instead of running library subroutines. |
| 500010–500099 | Version constants, limits, initialization flags, theme defaults. |
| 500100–500299 | `STACK` frame protocol: push, load, dispatch, close, drop; per-depth argument cache and locals. |
| 500300–500599 | Errors, validation, string and small numeric helpers. |
| 500600–500999 | Method tables, action-line table, class/kind registration data. |
| 501000–501999 | Control table: allocation, free list, creation/destruction, tree links. |
| 502000–502999 | Dispatch, message helpers, result transport. |
| 503000–503999 | Layout pass, absolute rectangles, clipping, work-stack traversal. |
| 504000–504999 | Paint pass, `PUSH VIEW`/`VIEW`/`POP VIEW`, dirty/update plumbing. |
| 505000–505999 | Input polling, hover/press/capture, hit testing, event queue. |
| 506000–506999 | Top-level class methods. |
| 507000–507999 | Panel class methods. |
| 508000–508999 | User class methods and kind-specific paint/update/click code. |
| 509000–509499 | Click-target class methods. |
| 509500–509999 | Shared drawing, text, geometry, event and value helpers. |
| 510000–511999 | Public API entry points. |
| 512000–518999 | Debug/test helpers and example-only code if needed. |
| 519000–519999 | Reserved for future public API. |

Line assignment inside each block is flexible; the block boundaries are the contract. Public API line numbers from §14 are stable once v0.1 ships, and each entry line carries a `LABEL` (`blApiInit`, `blApiCreate`, …).

The line space is partitioned:

| Lines | Owner |
|---|---|
| 1–499999 | Host program |
| 500000–519999 | baslib |
| 520000–999999 | Other libraries (do not overlap baslib) |

newbasic’s line store and `RENUM` accept signed 32-bit line numbers. baslib keeps its static lines inside **500000–519999**; the host owns **1–499999**. Both are far inside the signed 32-bit range, so the old GW-BASIC 65529 ceiling no longer applies.

`RENUM` moves the numeric entry points. Symbolic calls (`GOSUB blApiCreate`) and the `LABEL` definitions survive, so a renumbered merged program still runs; tools that rely on the published numeric map still break. Do not run `RENUM` after merging if anything depends on those numbers; renumber the host before merging, or leave the merged program unrenumbered.

#### 4.2.1 Named constants

The 500010–500099 block uses `CONST` for every published number and string. The
values are installed before the program starts, so no initialization order
matters, and host or library code cannot assign to them.

```basic
500010 CONST blVersion$ = "0.2"
500011 CONST blVersionNum# = 0.2
500012 CONST blMaxControls% = 255
500013 CONST blMaxFrames% = 32
500014 CONST blMaxArgs% = 8
500015 CONST blMaxStrArgs% = 2
500016 CONST blMaxClasses% = 16
500017 CONST blMaxVerbs% = 20
500018 CONST blMaxKinds% = 8
500019 CONST blMaxEvents% = 64
500020 CONST blHandleRoot% = 1
500021 CONST blActionNone% = 0
500022 CONST blStatusOk% = 0
500023 CONST blStatusNotInit% = 1
500024 CONST blStatusControlTableFull% = 2
500025 CONST blStatusInvalidHandle% = 3
500026 CONST blStatusInvalidClass% = 4
500027 CONST blStatusInvalidVerb% = 5
500028 CONST blStatusFrameOverflow% = 6
500029 CONST blStatusInvalidParent% = 7
500030 CONST blStatusCannotDestroyRoot% = 8
500031 CONST blStatusInvalidGeometry% = 9
500032 CONST blStatusInvalidValue% = 10
500033 CONST blStatusInvalidLayout% = 11
500034 CONST blStatusEventQueueFull% = 12
500035 CONST blStatusEventQueueEmpty% = 13
500036 CONST blStatusNoGraphics% = 14
500037 CONST blStatusFrameProtocol% = 15
500038 CONST blStatusMissingActionHandler% = 16
500039 CONST blStatusClassTableFull% = 17
500040 CONST blClassTop% = 1
500041 CONST blClassPanel% = 2
500042 CONST blClassUser% = 3
500043 CONST blClassClickTarget% = 4
500044 CONST blVerbPaint% = 1
500045 CONST blVerbUpdate% = 2
500046 CONST blVerbShow% = 3
500047 CONST blVerbHide% = 4
500048 CONST blVerbLayout% = 5
500049 CONST blVerbClick% = 6
500050 CONST blVerbHitTest% = 7
500051 CONST blVerbMouseDown% = 8
500052 CONST blVerbMouseUp% = 9
500053 CONST blVerbMouseMove% = 10
500054 CONST blVerbMouseEnter% = 11
500055 CONST blVerbMouseLeave% = 12
500056 CONST blVerbWheel% = 13
500057 CONST blVerbCaptureLost% = 14
500058 CONST blVerbDestroy% = 15
500059 CONST blVerbKey% = 16
500060 CONST blVerbFocus% = 17
500061 CONST blKindButton% = 1
500062 CONST blKindSlider% = 2
500063 CONST blKindGauge% = 3
500064 CONST blKindDial% = 4
500065 CONST blKindLabel% = 5
500066 CONST blEvClick% = 1
500067 CONST blEvChange% = 2
500068 CONST blEvMove% = 3
500069 CONST blEvEnter% = 4
500070 CONST blEvLeave% = 5
500071 CONST blEvDown% = 6
500072 CONST blEvUp% = 7
500073 CONST blEvWheel% = 8
500074 CONST blEvKey% = 9
500075 CONST blEvFocus% = 10
500076 CONST blEvCaptureLost% = 11
500077 CONST blFlagVisible% = 1
500078 CONST blFlagEnabled% = 2
500079 CONST blFlagClip% = 4
500080 CONST blFlagDirty% = 8
500081 CONST blFlagHot% = 16
500082 CONST blFlagPressed% = 32
500083 CONST blFlagClickable% = 64
500084 CONST blFlagFocusable% = 128
500085 CONST blFlagTransparent% = 256
500086 CONST blFlagLayoutDirty% = 512
500087 CONST blFlagCaptured% = 1024
500088 CONST blNoLine& = 0
```

The numbers are illustrative within the 500010–500099 block; the names are the
contract. Method tables store numeric line values because computed `GOSUB`
needs a number, but every public API entry point has a `LABEL` (see §14.2).

### 4.3 Host workflow

At the `Ok` prompt:

```
LOAD "DEMO.BAS"
MERGE "BASLIB.BAS"
RUN
```

Or, for a build or launch script, concatenate the two text files in line-number order. The host must not define lines in 500000–519999.

The host must also honor these rules:

- Do not use `OPTION BASE 1`. baslib requires the default base 0.
- Do not define variables beginning with `bl` (case-insensitive). That is the baslib namespace.
- Do not assign to `bl...` `CONST` names; they are read-only. Reading them is fine.
- Call public routines with labels (`GOSUB blApiCreate`), not literal lines. The numeric lines remain the compatibility contract.
- A host `LABEL` name is allowed; labels are a separate namespace from variables. Do not name a host label `bl...`.
- Do not rely on default numeric types inside baslib; the library uses explicit suffixes, and host code should use matching types in request variables.
- End the host’s main flow with `END` or a loop. The `500000 END` guard is a safety net, not a substitute.
- Select a graphics screen (for example `SCREEN 720`) before `blInit`. baslib reads `SCREENW`/`SCREENH`; it does not change the screen mode or palette.
- Call `blInit` before any other API.
- Host `RUN`, `LOAD`, `NEW`, or `CHAIN` clears `STACK` and variables; run `blInit` again afterward.
- The host may keep its own values on `STACK`; baslib frames are balanced above them. A host sentinel (`STACK PUSH 12345` before a baslib call, `STACK POP` after) is a useful integration test.
- A host `ON ERROR` handler that catches an unexpected baslib runtime error should `CLEAR` and reinitialize; baslib avoids expected runtime errors, but an exceptional error can leave a frame on `STACK`.

### 4.4 Why a fixed line range

The public API is a line-number contract plus symbolic labels. The host calls entry points such as `GOSUB blApiCreate`; labels resolve at compile time, while baslib methods remain lines in a fixed range. Keeping the whole library in 500000–519999 also makes `MERGE` predictable: library lines never replace host lines.

---

## 5. Namespace and typing rules

All library names use the `bl` prefix:

- Numeric scalars: `blName%` (integer), `blName&` (long), `blName!`/`blName#` (floating).
- String scalars: `blName$`.
- Arrays: `DIM blName%(...)`, `DIM blName#(...)`, `DIM blName$(...)`.

Every variable gets an explicit suffix. This protects the library from a host `DEFINT`, `DEFSNG`, `DEFDBL`, or `DEFSTR` that applies to the initial letter `B`.

Rules:

- Handles, coordinates, flags, counts, class IDs, verb IDs, and array indices are `%` (now signed 32-bit; still used for the small values). This is sufficient for 255 controls and the current canvas sizes.
- **Line-number-bearing values use `&` (Long), never `%`.** Both are signed 32-bit in newbasic now, but `&` keeps the line-number tables visually distinct and matches the published API contract (`blMethod&`, `blReqLine&`). This applies to method tables, action lines, `blReqLine&`, and any API-name constant.
- Values that can exceed the signed 32-bit range use `#` (double).
- Text uses `$`.
- Never use an unsuffixed scalar for persistent state.
- Never use `bl` names for host data.
- `blStatus%` is 0 on success and non-zero on failure. The library validates inputs and returns status instead of raising runtime errors wherever possible, so a host `ON ERROR` trap is not disturbed.

---

## 6. Architecture

```
Host program (1-499999)
    │
    ├── public API (510000-511999)
    │       init, create, layout, paint, poll, events, setters
    │
    ├── core
    │       control table, handles, tree, class registry
    │
    ├── STACK frames
    │       fixed frames, per-depth argument cache/locals, result pairs
    │
    ├── dispatch
    │       blMethod&(class, verb) → computed GOSUB
    │       blUserMethod&(kind, verb) → computed GOTO tail dispatch
    │
    ├── layout / paint
    │       work stack on STACK, local→absolute geometry,
    │       per-control PUSH VIEW / POP VIEW
    │
    ├── input / events
    │       live mouse + click queue, hover/press/capture,
    │       bounded event ring
    │
    └── class methods
            Top, Panel, User (button/slider/gauge/dial), ClickTarget
```

The core owns traversal. A class method paints *itself*, lays out *its own children’s local positions*, and handles *its own* input. The core walks the tree, computes absolute rectangles, sets clipping, and calls the method table. The method table removes the per-class dispatch lists; adding a built-in class or verb becomes a table write plus method lines.

---

## 7. Control model

### 7.1 Class, instance, verb

- **Class**: a fixed handler bundle identified by a small integer. All instances share it.
- **Instance**: one row in the control tables, identified by a handle (1–255). Handle 0 means “none”.
- **Verb**: an operation the core asks a class to perform.

Verbs:

| ID | Name | Purpose |
|---|---|---|
| 1 | Paint | Draw the whole control into the current clip/viewport. |
| 2 | Update | Redraw only the part that changed. |
| 3 | Show | Visibility changed to visible. |
| 4 | Hide | Visibility changed to hidden. |
| 5 | Layout | A container positions its children’s local rectangles. |
| 6 | Click | Handle a click at local coordinates. |
| 7 | HitTest | Return 1 if the point hits. |
| 8 | MouseDown | Button pressed over the control. |
| 9 | MouseUp | Button released. |
| 10 | MouseMove | Live pointer moved over or captured by the control. |
| 11 | MouseEnter | Pointer entered the control. |
| 12 | MouseLeave | Pointer left the control. |
| 13 | Wheel | Wheel movement over the control. |
| 14 | CaptureLost | The control lost mouse capture. |
| 15 | Destroy | Release class-specific state. |
| 16 | Key | Reserved for keyboard input. |
| 17 | Focus | Reserved for focus changes. |

The verb IDs are exposed as `blVerbPaint%`…`blVerbFocus%` constants. `CONST blMaxVerbs% = 20` leaves room for future verbs without changing the table shape. Every method slot in `blMethod&` and `blUserMethod&` is initialized to a class-local no-op line; a missing implementation is explicit, not accidental fall-through.

### 7.2 Built-in classes

| ID | Class | Implements | Notes |
|---|---|---|---|
| 1 | Top | Paint, Layout, HitTest, Click (optional) | Root container and input owner. One instance, handle 1. |
| 2 | Panel | Paint, Layout, HitTest, Click (optional) | Container; computes child layout and draws background/border. |
| 3 | User | Paint, Update, Click, HitTest, MouseDown/Up/Move/Enter/Leave, Wheel | One class; `blUserKind%` selects button, label, slider, gauge, or dial. |
| 4 | ClickTarget | HitTest, Click, MouseDown/Up/Enter/Leave, Wheel | Invisible hotspot; can also be a transparent overlay child. |
| 5–16 | Reserved | — | Method table points to the no-op line. |

Class IDs 1–4 are defined for v0.1 and exposed as `blClassTop%`…`blClassClickTarget%` constants. The shipped v0.1 methods are Top, Panel, User (button and label kinds), and ClickTarget; the slider/gauge/dial branches are added in v0.2. Adding a built-in class means filling its method table row and adding method lines; there is no static `ON` list to edit.

Class metadata:

```
blClassName$(1..blMaxClasses%)      debug name
blClassFlags%(1..blMaxClasses%)     default flags
blClassFace%(1..blMaxClasses%)      default face colour
blClassText%(1..blMaxClasses%)      default text colour
blClassAccent%(1..blMaxClasses%)    default accent colour
blClassMethodCount%(...)            optional bookkeeping
```

`blInit` fills classes 1–4 and leaves 5–16 zeroed except for the no-op method lines. A zero metadata row does not imply a callable class; `blMethod&` must be populated.

### 7.3 Roles, not multiple inheritance

BASIC has no inheritance. “Clickable” is an implemented verb:

- A button is a User kind that implements Paint + HitTest + Click + press visuals.
- A label is a User kind that implements Paint + Update and draws text. Its HitTest returns 0 so it is click-through.
- A click target implements HitTest + Click and paints nothing.
- A gauge implements Paint + HitTest but not Click; it may implement Wheel.
- A panel may implement Click if the host wants clicks on empty panel space.
- A slider/dial implements MouseDown/MouseMove/MouseUp for drag in v0.2.

Shared behavior (bevel drawing, text layout, value mapping) lives in ordinary helper subroutines, not in a base class.

### 7.4 User-control kinds

| Value | Kind | v0.1 | Behavior |
|---:|---|---|---|
| 1 | Button | Yes | Beveled face, caption, click posts the control’s action. |
| 2 | Slider | No, v0.2 | Track and thumb; click/drag sets the value. |
| 3 | Gauge | No, v0.2 | Read-only bar/arc display; no click. |
| 4 | Dial | No, v0.2 | Circular control; click/drag sets the value from the angle. |
| 5 | Label | Yes | Multiline static text with alignment, padding, optional border, auto-size, and no click. |
| 6–8 | Reserved | — | Kind methods point to the no-op line. |

Label remains a User kind rather than a separate class. It shares the User handler, needs no new class table row, and leaves class IDs 5–16 free. The kind values are exposed as `blKindButton%`…`blKindLabel%` constants.

---

## 8. Instance storage

### 8.1 Limits

| Constant | Value | Meaning |
|---|---:|---|
| `blMaxControls%` | 255 | Handles 1–255; index 0 is null. |
| `blMaxFrames%` | 32 | Maximum nested dispatch depth. |
| `blMaxArgs%` | 8 | Numeric arguments per frame. |
| `blMaxStrArgs%` | 2 | String arguments per frame. |
| `blMaxClasses%` | 16 | Method-table rows. |
| `blMaxVerbs%` | 20 | Method-table columns. |
| `blMaxKinds%` | 8 | User-kind method rows. |
| `blMaxEvents%` | 64 | Event ring size. |

A 255-handle limit keeps the control table bounded and matches the event action ID range. Class and verb counts are no longer tied to the `ON ... GOSUB` 255 limit; 16 × 20 is a table, not a list. Raising a limit is a source edit plus rechecking the table initialization.

These are the `CONST` declarations in §4.2.1 (`500012`–`500019`). They are installed before the program starts, read like ordinary scalars, and cannot be assigned by host or library code.

### 8.2 Common control table

`blCtl%(handle, field)` holds numeric state common to every control. Column 0 is unused.

| Field | Name | Type | Meaning |
|---:|---|---|---|
| 1 | `blFClass%` | int | Class ID; 0 means the handle is free. |
| 2 | `blFParent%` | int | Parent handle; 0 for the root. |
| 3 | `blFFirstChild%` | int | First (back-most) child. |
| 4 | `blFLastChild%` | int | Last (front-most) child. |
| 5 | `blFPrevSibling%` | int | Previous sibling. |
| 6 | `blFNextSibling%` | int | Next sibling. |
| 7 | `blFX%` | int | Local left. |
| 8 | `blFY%` | int | Local top. |
| 9 | `blFW%` | int | Width. |
| 10 | `blFH%` | int | Height. |
| 11 | `blFAbsX%` | int | Absolute left after layout. |
| 12 | `blFAbsY%` | int | Absolute top after layout. |
| 13 | `blFFlags%` | int | Bit flags. |
| 14 | `blFState%` | int | Class-specific state (normal/hot/pressed/etc.). |
| 15 | `blFAction%` | int | Host action ID posted with events. |
| 16 | `blFTag%` | int | Host-owned numeric tag. |
| 17 | `blFCapture%` | int | Non-zero when this handle owns mouse capture. |

String state:

```
blCtlText$(handle)     caption / label text
blCtlName$(handle)     optional debug/name text
```

Additional per-control arrays:

```
blCtlZ%(handle)        paint order, 0 = not painted/visible
blCtlClipL%(handle)    cached absolute clip rectangle
blCtlClipT%(handle)
blCtlClipR%(handle)
blCtlClipB%(handle)
blFreeNext%(handle)    free-list link
```

Flags:

| Bit | Name | Meaning |
|---:|---|---|
| 1 | `blFlagVisible%` | Control and subtree are visible. |
| 2 | `blFlagEnabled%` | Accepts input. |
| 4 | `blFlagClip%` | Clip children to this control’s bounds. |
| 8 | `blFlagDirty%` | Needs repaint. |
| 16 | `blFlagHot%` | Pointer is over this control. |
| 32 | `blFlagPressed%` | Left/right/middle button is held on this control. |
| 64 | `blFlagClickable%` | Optional hint; actual behavior is the class `Click` method. |
| 128 | `blFlagFocusable%` | Reserved for keyboard focus. |
| 256 | `blFlagTransparent%` | Paint is a no-op; hit test still applies. |
| 512 | `blFlagLayoutDirty%` | Subtree needs layout. |
| 1024 | `blFlagCaptured%` | This control owns mouse capture. |

The flag bits are `CONST` declarations in §4.2.1; they cannot be reassigned by
host or library code.

### 8.3 Class-specific tables

Panel (`blPanel%(handle, field)`):

| Field | Name | Meaning |
|---:|---|---|
| 1 | `blPanelMode%` | 0 absolute, 1 vertical stack, 2 horizontal row, 3 grid. |
| 2 | `blPanelPad%` | Inner padding. |
| 3 | `blPanelGap%` | Gap between children. |
| 4 | `blPanelCols%` | Grid column count (future). |
| 5 | `blPanelRowH%` | Default row height (future). |
| 6 | `blPanelBg%` | Background palette index. |
| 7 | `blPanelBorder%` | Border palette index. |
| 8 | `blPanelReserved%` | Future. |

User (`blUser#(handle, field)`):

This is a **double** array because slider/gauge/dial values are floating point. Integer-valued fields are stored as doubles. The names in the Field column are integer column-index constants.

| Field | Name | Meaning |
|---:|---|---|
| 1 | `blUserKind%` | 1 button, 2 slider, 3 gauge, 4 dial, 5 label. |
| 2 | `blUserValue%` | Current value. |
| 3 | `blUserMin%` | Minimum value. |
| 4 | `blUserMax%` | Maximum value. |
| 5 | `blUserStep%` | Step/quantization. |
| 6 | `blUserOrient%` | 0 horizontal, 1 vertical. |
| 7 | `blUserStyle%` | Visual variant. |
| 8 | `blUserFace%` | Face/background color. |
| 9 | `blUserAccent%` | Accent/thumb color. |
| 10 | `blUserDrag%` | Drag state: 0 idle, 1 dragging. |
| 11 | `blUserDecimals%` | Digits to display, or 0 for no numeric label. |
| 12 | `blUserReserved%` | Future. |
| 13 | `blUserPad%` | Label text padding. |
| 14 | `blUserAlign%` | Horizontal align: 0 left, 1 center, 2 right. |
| 15 | `blUserVAlign%` | Vertical align: 0 top, 1 middle, 2 bottom. |
| 16 | `blUserWrap%` | 0 no wrap in v0.1; 1 word wrap in v0.2. |
| 17 | `blUserBorder%` | Border: 0 none, 1 single. |
| 18 | `blUserTransparent%` | Background: 0 opaque, 1 transparent (default 1). |
| 19 | `blUserTextColor%` | Text color; -1 = theme default. |
| 20 | `blUserTextScale%` | Text scale 1–8 (default 1). |
| 21 | `blUserAutoSize%` | 1 = width/height may be recomputed from text; default 0. |
| 22 | `blUserLines%` | Cached rendered line count (read-only). |

Click target (`blClick%(handle, field)`):

| Field | Name | Meaning |
|---:|---|---|
| 1 | `blClickButtonMask%` | 1 left, 2 right, 4 middle; combinations allowed. |
| 2 | `blClickHot%` | Cached hover state. |
| 3 | `blClickPressed%` | Cached pressed state. |
| 4 | `blClickWheel%` | Wheel event opt-in. |

Representative declarations:

```
DIM blCtl%(blMaxControls%, 17)
DIM blPanel%(blMaxControls%, 8)
DIM blUser#(blMaxControls%, 22)
DIM blClick%(blMaxControls%, 4)
DIM blMethod&(blMaxClasses%, blMaxVerbs%)
DIM blUserMethod&(blMaxKinds%, blMaxVerbs%)
DIM blPanelLayoutLine&(3)
DIM blActionLine&(255)
```

Color fields (`blPanelBg%`, `blPanelBorder%`, `blUserFace%`, `blUserAccent%`, `blUserTextColor%`) default to -1, meaning “use the class/theme default”. Zero is a valid palette index.

The common row is allocated for every handle. Class-specific rows are allocated for every handle too, but only the matching class reads them. This trades a few dozen numeric cells per handle for direct indexing and no per-class handle arithmetic.

### 8.4 Handles and the free list

```
blNextHandle%     next never-used handle
blFreeHead%       first free handle, or 0
```

- `blRoot%` is 1 and is allocated by `blInit`.
- `blAllocHandle%` returns a free handle, or 0 if the table is full.
- `blFreeHandle%` clears the row, increments nothing, and pushes the handle onto the free list.
- Reuse is allowed. The library assumes no stale handle outlives a destroy; there is no generation counter in v0.1. Event handlers, capture fields, hover/press state, and pending event references are cleared on destroy.
- `blDestroy` recursively destroys children, front-to-back, before freeing the parent. It is iterative with a `STACK` work stack.
- Destroying the root is refused. End the program instead.

### 8.5 Tree representation

The control tree is a doubly linked child list per parent:

```
parent.firstChild ↔ ... ↔ parent.lastChild
```

- Append makes a control the front-most sibling (`lastChild`).
- Paint order is back-to-front (first child to last child).
- Hit test selects the highest paint order among candidates, which is the front-most control.
- Detach is O(1) because both `prev` and `next` links are stored.
- The root has parent 0 and no siblings.
- Capture, hover, and pressed state are single global handles, not per-parent.

Why a linked list instead of a 2D child array: unbounded child counts, O(1) insert/remove, and no fixed “max children per parent” to guess.

---

## 9. Class dispatch

### 9.1 Method table, not static lists

Two line-number tables replace the old two-level `ON ... GOSUB` lists:

```
blMethod&(1..blMaxClasses%, 1..blMaxVerbs%)       class × verb → line
blUserMethod&(1..blMaxKinds%, 1..blMaxVerbs%)     user kind × verb → line
blPanelLayoutLine&(0..3)                          panel mode → layout branch
blActionLine&(0..255)                             host action ID → line
```

`blInit` fills every method slot with the class-local no-op line, installs the built-in rows, and clears `blActionLine&`. The no-op line closes the current frame, returns a default result, and returns. Method tables store numeric lines, not labels, because computed `GOSUB` needs a value; literal API calls use `blApi...` labels.

Internal routines that are called literally also carry labels: `blDispatch`,
`blFramePush`, `blFrameLoad`, `blFrameClose`, `blFrameDrop`, and `blSend`.
These are library-private; only the `blApi...` labels in §14.2 are public.

Adding a built-in class or verb is a table write. A v0.2 `blRegisterClass` can install host-provided method lines at runtime; class-specific storage is still limited to the common table plus the generic host-data columns.

### 9.2 Dispatcher shape

```
502000 REM blDispatch: dispatch the frame already pushed on STACK
502010 IF blFp% >= blMaxFrames% THEN blStatus% = blStatusFrameOverflow% : GOTO 502090
502020 STACK TOP blArgcS%, blArgcN%, blDispVerb%, blDispCtl%
502030 IF blDispCtl% < 1 OR blDispCtl% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : GOTO 502090
502040 blDispClass% = blCtl%(blDispCtl%, blFClass%)
502050 IF blDispClass% < 1 OR blDispClass% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : GOTO 502090
502060 IF blDispVerb% < 1 OR blDispVerb% > blMaxVerbs% THEN blStatus% = blStatusInvalidVerb% : GOTO 502090
502070 blMethodLine& = blMethod&(blDispClass%, blDispVerb%)
502080 IF blMethodLine& = blNoLine& THEN blStatus% = blStatusInvalidVerb% : GOTO 502090
502082 blFp% = blFp% + 1
502084 GOSUB blFrameLoad          ' cache the 14-slot frame at blFp%
502086 GOSUB blMethodLine&        ' method closes the frame and pushes the result
502088 blFp% = blFp% - 1 : RETURN
502090 GOSUB blFrameDrop          ' invalid/overflow: consume frame, push default result
502092 RETURN
```

The exact line layout will be finalized during implementation. The important contract is: the dispatcher peeks metadata without removing the frame, validates, calls the method with `GOSUB`, and the method closes the frame.

### 9.3 Computed GOTO tail dispatch

A method that has already been called by `GOSUB` can use computed `GOTO` to jump to a branch and let that branch’s `RETURN` go straight back to the dispatcher. This avoids an extra `GOSUB`/`RETURN` for every kind or mode:

```
508000 REM User class router (called by blDispatch)
508010 blUk% = blUser#(blArgCtl%(blFp%), blUserKind%)
508020 GOTO blUserMethod&(blUk%, blArgVerb%(blFp%))
```

Panel layout uses the same pattern:

```
507000 REM Panel Layout
507010 GOTO blPanelLayoutLine&(blPanel%(blArgCtl%(blFp%), blPanelMode%))
```

Each target method ends with the shared `blFrameClose`/`RETURN` sequence. The frame and `blFp%` stay valid across the `GOTO`.

**Rule:** do not use computed `GOTO` from inside a `SELECT CASE`; newbasic raises `GOTO would leave a SELECT CASE`. Use computed `GOSUB` or structure the branch outside the SELECT.

### 9.4 What a method receives

A method receives a frame on `STACK` and a depth `blFp%`. The dispatcher loads the frame into the per-depth cache before calling:

```
blArgCtl%(blFp%)        control handle
blArgVerb%(blFp%)       verb ID
blArgcN%(blFp%)         numeric argument count
blArgcS%(blFp%)         string argument count
blArgN#(blFp%, index)   numeric arguments, 1-based
blArgS$(blFp%, index)   string arguments, 1-based
blLocalN#(blFp%, slot)  mutable local numeric slots
blLocalS$(blFp%, slot)  mutable local string slots
```

A method must not decrement `blFp%` or manipulate another frame. It may call another control through `blSend`; that pushes a new frame, dispatches it, and restores `blFp%` on return. State that must survive that nested call lives in `blLocal...`, not in shared scratch scalars.

### 9.5 Action-line dispatch

`blSetActionLine(action%, line&)` stores a host line number in `blActionLine&(action%)`. The host loop can then use computed `GOSUB`:

```
GOSUB blApiNextEvent                    ' blNextEvent
IF blEventAction% <> blActionNone% AND blActionLine&(blEventAction%) <> blNoLine& THEN GOSUB blActionLine&(blEventAction%)
```

The library does not call host code itself; the host performs the computed `GOSUB`. `blNoLine&` means “no handler” and must be checked. Action IDs are 0–255; `blActionNone%` remains “no action” by convention.

### 9.6 Dynamic class registration (v0.2)

`blRegisterClass(class%, paintLine&, updateLine&, ...)` fills a free row of `blMethod&`. A host class can use the common control columns and a small generic host-data area. It cannot add new class-specific arrays, so a host class is a paint/input callback over common state. This is a v0.2 feature; v0.1 ships built-in classes only.

Dynamic class registration is no longer blocked by the language; it is blocked only by the fixed class-specific storage. A future “class bag” array could remove that limit.

---

## 10. STACK call protocol

### 10.1 Why STACK

The old design used a hand-rolled numeric/string frame stack and a hand-rolled work stack. `STACK` provides both, with:

- atomic multi-value push/pop;
- type preservation and normal conversion on targets;
- whole-array copy by value;
- one global stack shared by host and library;
- bounded capacity and a clear underflow error.

The design keeps `STACK` canonical. Arrays are still used for random access to the current frame and for per-depth mutable locals, because BASIC has no pointers into the stack.

### 10.2 Frame layout

Every call frame is exactly 14 stack entries, pushed bottom-to-top:

| Position | Slot | Type | Meaning |
|---:|---|---|---|
| 1–8 | `N1..N8` | numeric | Numeric arguments. |
| 9–10 | `S1,S2` | string | String arguments. |
| 11 | `Ctl` | numeric | Target control handle. |
| 12 | `Verb` | numeric | Verb ID. |
| 13 | `ArgcN` | numeric | Number of numeric arguments. |
| 14 | `ArgcS` | numeric | Number of string arguments (top). |

A frame is pushed with one statement, for example:

```
STACK PUSH blCallN#(1), ..., blCallN#(8), blCallS$(1), blCallS$(2), blCallCtl%, blCallVerb%, blCallArgcN%, blCallArgcS%
```

The top is `ArgcS`. `STACK TOP` reads top-first, so the metadata read order is `ArgcS, ArgcN, Verb, Ctl, S2, S1, N8, ..., N1`.

The result pair is pushed by the method as:

```
STACK PUSH blRetS$, blRetN#
```

so the numeric result is top. The caller pops `blRetN#, blRetS$`.

### 10.3 Depth and per-depth state

```
blFp%                    active baslib frame index, 0 = none
blMaxFrames%             32
```

`blFp%` counts baslib frames only. Host values below them are invisible and must never be popped. The dispatcher increments `blFp%` after validation and decrements it after the method returns. A depth overflow drops the new frame, returns status 6, and leaves the previous frame current.

Per-depth arrays:

```
blArgCtl%(1..blMaxFrames%)
blArgVerb%(1..blMaxFrames%)
blArgcN%(1..blMaxFrames%)
blArgcS%(1..blMaxFrames%)
blArgN#(1..blMaxFrames%, 1..blMaxArgs%)
blArgS$(1..blMaxFrames%, 1..blMaxStrArgs%)
blLocalN#(1..blMaxFrames%, 1..blMaxArgs%)
blLocalS$(1..blMaxFrames%, 1..blMaxStrArgs%)
```

Representative declarations:

```
DIM blArgN#(blMaxFrames%, blMaxArgs%)
DIM blArgS$(blMaxFrames%, blMaxStrArgs%)
DIM blLocalN#(blMaxFrames%, blMaxArgs%)
DIM blLocalS$(blMaxFrames%, blMaxStrArgs%)
DIM blArgCtl%(blMaxFrames%)
DIM blArgVerb%(blMaxFrames%)
DIM blArgcN%(blMaxFrames%)
DIM blArgcS%(blMaxFrames%)
```

These are a cache, not the canonical frame. The frame remains on `STACK` until the method closes it.

### 10.4 Push, load, close

**Push** (`blFramePush`):

1. Validate `blCallCtl%`, `blCallVerb%`, `blCallArgcN%`, `blCallArgcS%`.
2. Fill unused `blCallN#`/`blCallS$` slots with 0/empty.
3. `STACK PUSH` all 14 slots.
4. Return status. If validation fails, no frame is pushed and the caller must not call `blDispatch`; if it succeeds, the caller must call `blDispatch` exactly once.

**Load** (`blFrameLoad`, called by the dispatcher after incrementing `blFp%`):

```
STACK TOP blArgcS%(fp), blArgcN%(fp), blArgVerb%(fp), blArgCtl%(fp), ...
          blArgS$(fp,2), blArgS$(fp,1), blArgN#(fp,8), ..., blArgN#(fp,1)
```

This leaves the frame intact. Methods read the cache. If a method needs a fresh copy after a nested call, the cache is still valid because each depth has its own row.

**Close** (`blFrameClose`, called at the end of every method):

1. Pop the 14 slots into typed scratch variables in top-first order.
2. `STACK PUSH blRetS$, blRetN#`.
3. Return.

After close, the only new value above the caller’s entry depth is the result pair.

**Drop** (`blFrameDrop`, invalid class/verb/depth or no-op):

1. Pop the 14 slots into typed scratch variables.
2. `STACK PUSH "", 0` (default result).
3. Return.

`blFrameClose` and `blFrameDrop` must be used symmetrically. A method must never return with its frame still on the stack.

### 10.5 Caller shape

Call setup globals:

```
blCallCtl%         target control
blCallVerb%        verb ID
blCallArgcN%       numeric argument count
blCallArgcS%       string argument count
blCallN#(1..8)     numeric arguments
blCallS$(1..2)     string arguments
blRetN#            numeric result
blRetS$            string result
```

`blCallN#`/`blCallS$` are dimensioned once at `blMaxArgs%`/`blMaxStrArgs%`; unused slots are zeroed by `blFramePush`.

Internal callers use:

```
blCallCtl% = target%
blCallVerb% = verb%
blCallArgcN% = n%
blCallArgcS% = s%
blCallN#(1) = ...
blCallS$(1) = ...
GOSUB blFramePush
GOSUB blDispatch
STACK POP blRetN#, blRetS$
```

`blSend` (a convenience helper) performs push, dispatch, and result pop for zero- or few-argument calls. If `blFramePush` rejects the request, `blSend` returns the status without calling `blDispatch` and without popping a result. Argument-heavy callers set the `blCall...` block directly.

The result pair is always present when a frame was pushed, even for a void verb; it is simply ignored.

### 10.6 Nested calls

A method’s frame remains on `STACK` while it runs. A nested `blSend` pushes a child frame above it, dispatches, and pops the child result. When the child returns, the parent frame is on top again.

- Parent arguments are safe in `blArg...` at the parent depth.
- Parent mutable locals are safe in `blLocal...` at the parent depth.
- Shared scratch scalars (`blTmp...`) are clobbered by nested calls; do not use them across a `blSend`.
- `blFp%` is restored after the child returns.

### 10.7 Invariants and host coexistence

- Every dispatch consumes exactly one 14-slot frame and leaves exactly one 2-slot result.
- `blFp%` is balanced after every API call.
- baslib never assumes `STACK` is empty. A host may push its own values before calling baslib; the frame and result sit above them.
- baslib never pops below its own frames. If a method or API returns an error, it still closes its frame.
- Expected failures use `blStatus%`, not BASIC runtime errors, so `ON ERROR` is not used as control flow.
- An unexpected runtime error inside a method can leave a frame. The host should `CLEAR` and reinitialize; baslib does not attempt to repair an arbitrary stack.
- `STACK` is cleared by `CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN`; `blInit` is required afterward.
- `GOSUB`/`RETURN` do not touch `STACK`; nested calls are the source of depth, not the GOSUB stack.

### 10.8 Work stack

Layout, paint, and destroy use `STACK` as an iterative work stack:

```
blWorkCount%     number of live work items
```

- Push a work item as `[handle, clipL, clipT, clipR, clipB]` (or `[handle]` for destroy).
- `blWorkCount%` counts live items; `STACK` has no depth query.
- Pop one item, process, push children in the required order.
- Class methods use their own frames above the work items; a method must close its frame before returning, so the next work item is on top again.
- The work count is decremented on every pop and asserted to reach 0.

For paint, children are pushed in reverse sibling order so the pop order is back-to-front. For destroy, children are pushed front-to-back and popped front-to-back.

This removes the old fixed `blMaxWork%` array and makes a 255-control tree safe without a separate limit.

### 10.9 What STACK is not used for

- **Event queue**: FIFO, not LIFO; it stays a fixed ring.
- **Random access to a control table**: arrays remain the right tool.
- **Fixed frame arrays**: `STACK` is the frame transport, but the current frame’s arguments are cached in indexed arrays because BASIC has no stack pointer.
- **Whole-array call arguments in v0.1**: fixed scalar slots keep frame cleanup deterministic. Whole arrays can be stored on `STACK` separately for snapshots, tests, and future APIs.

---

## 11. Geometry and layout

### 11.1 Coordinate model

- Units are integer pixels.
- Every control has a local rectangle: `X`, `Y`, `W`, `H`, relative to the **parent’s outer top-left**, not its padded client area.
- Panel padding is applied by the panel’s Layout method. The core never adds padding during local→absolute conversion.
- The root’s local rectangle is `(0, 0, blRootW%, blRootH%)`.
- The root size defaults to `SCREENW`/`SCREENH`; `blInit` reads `blReqW%`/`blReqH%` and falls back to the canvas.
- The layout pass computes `AbsX`, `AbsY`, and a cached absolute clip rectangle.
- A control draws in its own local coordinate system (0,0 at its top-left), with the paint offset described in §12.3.

### 11.2 Client area and padding

For a panel, the inner area is:

```
clientW = W - 2*Pad
clientH = H - 2*Pad
child origin = (Pad, Pad)
```

A border occupies the outer edge; padding is inside it. If `clientW` or `clientH` is not positive, the panel has no visible children and layout skips them.

### 11.3 Layout modes

Panel `blPanelMode%`:

| Value | Mode | Behavior |
|---:|---|---|
| 0 | Absolute | Child X/Y are used as-is relative to the panel’s outer top-left; padding is ignored. |
| 1 | Vertical | Children stack from `(Pad, Pad)` top-to-bottom with `Gap`; width is `clientW`. |
| 2 | Horizontal | Children flow from `(Pad, Pad)` left-to-right with `Gap`; height is `clientH`. |
| 3 | Grid | Equal-width columns; reserved for a later revision. |

v0.1 implements 0, 1, and 2. Grid is parsed and reserved, not promised.

Panels do not dispatch their children’s Layout methods. The core does that in its own pre-order walk. This keeps a parent’s layout loop free of nested dispatches and shared-loop clobbering.

### 11.4 Layout pass

`blLayoutAll`:

1. Start with the root and the root clip.
2. For each container popped from the `STACK` work stack:
   - Dispatch `Layout` so the class sets its children’s local rectangles.
   - For each child, compute absolute position:
     ```
     child.AbsX = parent.AbsX + child.X
     child.AbsY = parent.AbsY + child.Y
     ```
   - Compute the child clip as the intersection of the parent clip and the child’s absolute rectangle when `blFlagClip%` is set; otherwise inherit the parent clip.
   - Push the child for its own layout.
3. Clear `blFlagLayoutDirty%` and the global layout-dirty flag.

The pass is iterative. A deep tree cannot overflow the interpreter’s GOSUB stack, and the `STACK` work items are independent of call frames.

### 11.5 When layout runs

- `blCreate`, `blSetBounds`, `blShow`, `blHide`, and `blAutoSize` mark the tree layout-dirty.
- `blLayoutAll` is explicit.
- `blPaintAll` calls `blLayoutAll` automatically if the layout-dirty flag is set.
- `blHitTest` assumes layout has run; `blPaintAll` is the normal way to guarantee it. `blPollInput` documents this precondition.

### 11.6 Clipping

- Root clip: `(0, 0, rootW-1, rootH-1)`.
- Child clip: intersection as above. Empty intersections are not painted or hit-tested.
- `blFlagClip%` is set by default on panels and the root. A leaf usually does not need it, but inherits the parent’s clip.
- Clipping is cached per handle for fast hit testing and for `blUpdate`.

---

## 12. Painting

### 12.1 Paint traversal

`blPaintAll`:

1. `blLayoutAll` if needed.
2. Clear all `blCtlZ%` to 0.
3. Push the root with the root clip onto the `STACK` work stack.
4. While `blWorkCount% > 0`:
   - Pop a control and its clip.
   - Skip hidden controls and empty clips.
   - Set the current paint control and clip.
   - `PUSH VIEW`
   - `VIEW (clipL, clipT)-(clipR, clipB)`
   - Dispatch `Paint`.
   - `POP VIEW`
   - Assign `blCtlZ%` in paint order.
   - Push children in reverse (last→first) so they are popped and painted back-to-front.
5. The host viewport is restored by every `POP VIEW`; no bare `VIEW` is needed.

The root and panels paint only their own background/border. The core paints their children. A class Paint method must never dispatch another control; that would change `VIEW` and the current frame underneath it.

### 12.2 Why PUSH VIEW / POP VIEW

`VIEW` gives the paint pass a real graphics clip and a local coordinate origin. `PUSH VIEW`/`POP VIEW` preserve the host’s viewport exactly, even if the host has a custom `VIEW` or a different `WINDOW` mapping. This removes the old integration rule that a host must not keep a viewport active.

- Each control push/pop is balanced even if a method returns early.
- `VIEWX1`/`Y1`/`X2`/`Y2` can be used to inspect the current viewport in tests.
- A class method must not issue its own `VIEW` or leave a viewport active.
- A paint error is not expected; if one occurs, the host should `CLEAR` and reinitialize.

### 12.3 Paint offset

`VIEW` is set to the **clip** rectangle, which may be smaller than the control when a parent clips it. Therefore:

```
blPaintOX% = AbsX - ClipL
blPaintOY% = AbsY - ClipT
```

A class draws local `(x, y)` at `(blPaintOX% + x, blPaintOY% + y)`. For an unclipped control, both offsets are 0. Shared drawing helpers add the offsets; class code should not use raw absolute coordinates.

### 12.4 Text

Text uses `DRAWTEXT` plus the new metrics:

```basic
DRAWTEXT (x, y), text$, colour, size
```

Properties that matter to baslib:

- The anchor is the top-left of the first glyph cell.
- The built-in 8×16 CP437 bitmap is deterministic across platforms.
- Only ink pixels are written; text is naturally transparent over a filled control.
- Graphics `VIEW` clips glyphs, so text needs no separate clipping path.
- `size` is 1–8; `TEXTWIDTH(text$, size)` and `TEXTHEIGHT(text$, size)` give exact pixel metrics.
- `CHR$(13)` and `CHR$(10)` start a new line; `TEXTWIDTH` returns the widest line and `TEXTHEIGHT` the total line height.
- `DRAWTEXT` updates the last graphics point; baslib does not rely on it across paints.

Text helpers wrap `DRAWTEXT` and work in control-local coordinates, adding `blPaintOX%`/`blPaintOY%` for the active viewport. Controls fill their own background first when they want an opaque surface, then draw text over it.

### 12.5 Label text

Labels support:

- explicit multiline text via CR/LF;
- horizontal alignment (left/center/right) using `TEXTWIDTH` of the widest line;
- vertical alignment (top/middle/bottom) using `TEXTHEIGHT`;
- pixel truncation to the available width;
- optional border and padding;
- optional transparent background;
- text scale 1–8;
- auto-size via `blAutoSize` or `blUserAutoSize%`.

`blTextRect` computes metrics with `TEXTWIDTH`/`TEXTHEIGHT`, splits lines at CR/LF, truncates each line to the available pixel width, and draws each line with `DRAWTEXT`. `VIEW` clips any overflow.

Word wrap (`blUserWrap% = 1`) is v0.2. It uses `TEXTWIDTH` to fit words and `TEXTHEIGHT` for the resulting height. v0.1 accepts `blUserWrap% = 0` only.

`blAutoSize(handle%)` sets width/height from `TEXTWIDTH`/`TEXTHEIGHT` plus padding and border, marks layout dirty and `blNeedsPaint%`, and is valid for labels and buttons. It does not paint.

### 12.6 Paint vs update

- `Paint` draws the whole control. It is called by `blPaintAll`.
- `Update` redraws only the changed part. It is called by `blUpdate(handle%)` under a `PUSH VIEW` / `VIEW clip` / `POP VIEW` wrapper and uses the control’s cached clip. The class decides what “changed” means (for example, a slider redraws its thumb and the vacated old thumb area).
- `blInvalidate(handle%)` sets `blFlagDirty%` and `blNeedsPaint%`. `blPaintAll` clears both; `blUpdate` clears the target’s dirty flag and recomputes `blNeedsPaint%` from the remaining controls.
- `blNeedsLayout%` is set by creation, bounds changes, show/hide, and auto-size; `blLayoutAll` clears it.
- v0.1 does not implement a dirty-rectangle compositor. `blUpdate` is a per-control partial redraw, not a general damage-region system. A host that changes many controls should call `blPaintAll`.
- `blUpdate` is only for content changes within stable bounds. After a move, resize, show, hide, or auto-size, call `blPaintAll` so the vacated area is repainted.
- `blSetText`, `blSetValue`, `blSetEnabled`, hover/press transitions, and wheel changes mark the control dirty and set `blNeedsPaint%`, but do not paint. The host calls `blUpdate` for that control or `blPaintAll` for the frame.

### 12.7 Theme

The library never calls `PALETTE` or `SCREEN`. It uses palette indices and exposes theme globals:

```
blThemeDesktop%
blThemePanel%
blThemeFace%
blThemeLight%
blThemeDark%
blThemeText%
blThemeAccent%
blThemeDisabled%
```

`blInit` sets defaults appropriate to the current palette. The host may override them after `blInit` and repaint. `blCreate` can set per-control colors through `blReqFace%`, `blReqAccent%`, `blReqBg%`, and `blReqBorder%`. The value -1 means “use the theme default”; 0 is a real palette index.

### 12.8 No double buffering in v0.1

A `GET`/`PUT` screen-image buffer would allow flicker-free full repaints, but it costs a large integer array and complicates clipping. v0.1 draws directly. `blUpdate` avoids full repaints in interactive paths. Double buffering can be evaluated after the control model is stable.

---

## 13. Input, hit testing, and events

### 13.1 Input capabilities

- `MOUSEB` returns 1 (left), 2 (right), or 3 (middle), or 0 if no click is queued. It consumes one queued click.
- `MOUSEX`/`MOUSEY` are latched to the most recent consumed click.
- `MOUSEPOSX`/`MOUSEPOSY` are the live pointer position; a window backend returns -1 when outside the graphics canvas.
- `MOUSEBUTTONS` is a live bit mask: 1 left, 2 right, 4 middle.
- `MOUSEWHEEL` returns accumulated wheel movement and consumes it.
- `SLEEP` lets the host pace the loop; the library never sleeps inside paint or dispatch.

Clicks remain the action mechanism. Live state drives hover, press/release, wheel, and drag. This avoids double-posting a click from both the click queue and the live button transition.

### 13.2 Hit testing

`blHitTest(x%, y%)`:

1. Ensure layout is current (`blPaintAll` normally did this).
2. Loop handles 1..`blMaxControls%`.
3. Reject free handles, hidden controls, disabled controls, zero paint order, and points outside the cached clip or absolute rectangle.
4. Optionally dispatch `HitTest` for a shape check (dial circle, for example). The default is “inside rectangle”.
5. Track the candidate with the highest `blCtlZ%`.
6. Return that handle, or 0.

The loop is O(255) per input event, which is negligible and avoids maintaining a spatial index. `blCtlZ%` is assigned by the last paint, so hit testing is defined against what the user actually sees.

Visibility and enabled state are inherited: a hidden or disabled ancestor excludes its whole subtree from painting and hit testing. The paint/layout walks skip such subtrees; the hit test checks the parent chain for a disabled ancestor. A disabled control that should swallow clicks is a future flag.

### 13.3 Click dispatch

`blClickAt(x%, y%, button%)`:

1. `target% = blHitTest(x%, y%)`.
2. While `target% <> 0`:
   - Push a frame with `Click`, local coordinates, and the button.
   - Dispatch.
   - If the class returns non-zero (handled), post a click event and stop.
   - Otherwise move to the parent and repeat (bubbling).
3. If no control handles the click, no click event is posted.

Bubbling lets a panel or the root handle clicks on empty space without every leaf needing a handler. A button or click target returns 1 when it handles the click; future slider/dial handlers do the same.

Click button codes are `1` left, `2` right, and `3` middle, matching `MOUSEB`. The click-target mask uses bits `1`, `2`, and `4`; the click-target method maps code 3 to bit 4 before testing.

### 13.4 Live input state machine

`blPollInput` reads live state once per host frame:

```
x = MOUSEPOSX : y = MOUSEPOSY
buttons = MOUSEBUTTONS
wheel = MOUSEWHEEL
```

It maintains:

```
blPrevX%, blPrevY%, blPrevButtons%
blHover%          current live hit control, or 0
blCapture%        control receiving drag/move while a button is held
blPressed%        control that received the last down
blMouseIn%        pointer is over the canvas
```

Processing order:

1. **Hover**: if the hit control changed, post `MouseLeave` to the old, `MouseEnter` to the new, update `blFlagHot%`, and mark both dirty.
2. **Buttons**: compare `blPrevButtons%` with `buttons`. For each newly pressed bit, post `MouseDown` to the hit control and set `blCapture%`/`blPressed%`; for each released bit, post `MouseUp` to the capture/pressed control and release capture.
3. **Move**: if the pointer moved, dispatch `MouseMove` to `blCapture%` when captured, otherwise to the hit control. Post a move event only when the target or position changed; coalesce repeats.
4. **Clicks**: while the queue is not empty and a bound is not reached, call `MOUSEB`; on a non-zero button, read `MOUSEX`/`MOUSEY` and call `blClickAt`. This is the action path.
5. **Wheel**: if `wheel <> 0`, dispatch `Wheel` to `blCapture%` or the hit control, post a wheel event, and mark dirty.
6. **Pointer outside**: when the live position is -1, clear hover/pressed visuals but keep capture until the button is released.
7. Save previous position/buttons.

Any hot/pressed change sets `blNeedsPaint%`; any geometry change sets `blNeedsLayout%` and `blNeedsPaint%`. The exact ordering of Move/Down/Up versus Click is documented in the event queue order. A click is posted by `blClickAt`; down/up are visual/drag events and do not duplicate the action.

### 13.5 Drag and capture

Capture is global:

```
blCapture%     control receiving move/up while a button is held
```

- On a press over a hit control, set capture and dispatch `MouseDown`.
- While captured, `MouseMove` goes to the capture control, not the hit control.
- On release, dispatch `MouseUp`, then clear capture.
- `blFlagCaptured%` mirrors `blCapture%`.
- Destroying or hiding the capture control clears capture and dispatches `CaptureLost`.
- Slider and dial use capture in v0.2: MouseDown sets the value, MouseMove drags, MouseUp finishes.

### 13.6 Event queue

Internal ring (private to the library):

```
blEvType%(1..blMaxEvents%)
blEvCtl%(1..blMaxEvents%)
blEvAction%(1..blMaxEvents%)
blEvX%(1..blMaxEvents%)
blEvY%(1..blMaxEvents%)
blEvButton%(1..blMaxEvents%)
blEvWheel%(1..blMaxEvents%)
blEvHead%, blEvTail%, blEvCount%
```

Public values filled by `blNextEvent`:

```
blEventType%       event type (0 when the queue is empty)
blEventCtl%        control that generated the event
blEventAction%     action ID copied from the control
blEventX%, blEventY%
blEventButton%
blEventWheel%
blEventCount%      number of events still queued
```

Event types:

| Value | Name | Meaning |
|---:|---|---|
| 1 | `blEvClick%` | A control handled a click. |
| 2 | `blEvChange%` | Value changed (v0.2 continuous controls). |
| 3 | `blEvMove%` | Live pointer moved. |
| 4 | `blEvEnter%` | Pointer entered a control. |
| 5 | `blEvLeave%` | Pointer left a control. |
| 6 | `blEvDown%` | Button pressed. |
| 7 | `blEvUp%` | Button released. |
| 8 | `blEvWheel%` | Wheel moved. |
| 9 | `blEvKey%` | Reserved. |
| 10 | `blEvFocus%` | Reserved. |
| 11 | `blEvCaptureLost%` | Capture was released by hide/destroy. |

These event numbers are `CONST` declarations in §4.2.1 (`blEv...%`). Move events are coalesced per poll; the queue is bounded at 64. If the queue fills, the newest non-click event is dropped and `blStatus%`/`blError%` indicate a full queue; clicks take priority. `blPollInput` drains at most eight queued clicks per call.

### 13.7 Action IDs and action lines

`blFAction%` is a host-assigned integer. On a handled click, it is copied to the event. The host can dispatch with a static `ON blEventAction% GOSUB ...` or with a line table:

```
blActionLine&(action%) = handler line
IF blActionLine&(blEventAction%) <> blNoLine& THEN GOSUB blActionLine&(blEventAction%)
```

`blSetActionLine(action%, line&)` fills the table. Action 0 is valid but should be reserved for “no action”; `blNoLine&` means no handler.

Action IDs are ordinary request values; host code can declare them with `CONST`
(for example `CONST ActButton% = 100`). The handler **line** must be numeric
because `blSetActionLine` stores a line value for later computed `GOSUB`;
`LABEL` is for direct `GOTO`/`GOSUB` calls, not for a value stored in a table.

This is the BASIC substitute for a callback function pointer. The library never calls host code directly; it posts an event and returns. The host may call the line explicitly, which keeps the host in control of its own stack.

### 13.8 Test hooks

Synthetic input must not depend on window events:

- `blTestClickAt(x%, y%, button%)` performs hit test and click dispatch without the queue.
- `blTestMouseMove(x%, y%)` sets the live position and runs hover/move processing.
- `blTestMouseButton(button%, down%, x%, y%)` sets/clears a live button bit and runs down/up processing; button is 1 left, 2 right, 3 middle.
- `blTestWheel(delta%)` injects wheel movement.

These call the same internal routines as `blPollInput`, so tests cover the real state machine.

### 13.9 Keyboard

Out of scope for v0.1. The `Key` and `Focus` verb slots and `blFlagFocusable%` are reserved. The host can poll `INKEY$` itself. Tab order will follow child order when implemented.

---

## 14. Public API

All public calls use `GOSUB blApi...` labels; internal method dispatch uses numeric table lines. Set request globals, call, then check `blStatus%` and read `blRet...`. Internal methods use `STACK` frames; the host API does not.

### 14.1 Common request/return globals

```
blStatus%         0 success, non-zero error
blError%          last error code
blError$          last error message
blReqClass%       class ID
blReqParent%      parent handle
blReqHandle%      target handle
blReqX%, blReqY%  local or absolute coordinates, depending on call
blReqW%, blReqH%  width/height
blReqFlags%       initial flags
blReqAction%      host action ID
blReqTag%         host numeric tag
blReqKind%        user-control kind
blReqFace%        user-control face/background color; -1 = theme default
blReqAccent%      user-control accent color; -1 = theme default
blReqTextColor%   label/button text color; -1 = theme default
blReqTextScale%   text scale 1-8
blReqTransparent% label background: 0 opaque, 1 transparent
blReqAlign%       text align: 0 left, 1 center, 2 right
blReqVAlign%      text vertical align: 0 top, 1 middle, 2 bottom
blReqBorderStyle% text border: 0 none, 1 single
blReqWrap%        0 no wrap, 1 word wrap (v0.2)
blReqAutoSize%    1 = auto-size after create/set-text
blReqBg%          panel background color; -1 = theme default
blReqBorder%      panel border color; -1 = theme default
blReqMode%        panel layout mode
blReqPad%         panel or text padding
blReqGap%         panel gap
blReqCols%        panel grid columns (reserved)
blReqButton%      button number for test hooks
blReqEnabled%     1 = enabled, 0 = disabled
blReqButtonMask%  click-target mask: 1 left, 2 right, 4 middle
blReqText$        caption/name/label text
blReqValue#       slider/gauge/dial value
blReqLine&        callback line for blSetActionLine
blReqActionId%    action ID for blSetActionLine
blRoot%           handle of the top-level control
blRetHandle%      returned handle
blRetValue#       returned value
blRetN%           generic numeric result
blEventType%      event type, 0 when empty
blEventCtl%       control that generated the event
blEventAction%    action ID copied from the control
blEventX%, blEventY%
blEventButton%    button that caused the event
blEventWheel%     wheel delta
blEventCount%     queued event count
blNeedsPaint%     1 if a control or input state marked the tree dirty
blNeedsLayout%    1 if layout is stale
blActionLine&(0..255)  host action ID → handler line
```

`blCreate` uses the request block and returns a handle in `blRetHandle%`.

`blCreate` defaults:

- flags = `blReqFlags%` OR visible + enabled + clip;
- action/tag = 0;
- text = empty;
- value = 0;
- colors = -1 (theme default; 0 is a valid palette index);
- panel mode = 0, pad = 0, gap = 0;
- text scale = 1;
- label align = 0, valign = 1, border = 0, wrap = 0, transparent = 1, auto-size = 0;
- click-target mask = 1 (left);
- user kind is **required** for class 3;
- width/height must be set explicitly; zero is legal but invisible.

`blInit` reads `blReqW%`/`blReqH%` first, then initializes the rest of the request block to safe defaults. If `blReqW%`/`blReqH%` are 0, it uses `SCREENW`/`SCREENH`. If the canvas is unavailable (text mode or zero size), it returns error 14. `blInit` sets `blNeedsPaint% = 1` and `blNeedsLayout% = 1`.

### 14.2 Entry points

| Line | Label | Routine | Inputs | Outputs |
|---:|---|---|---|---|
| 510000 | `blApiInit` | `blInit` | `blReqW%`, `blReqH%` (0 → canvas) | `blRoot%`, `blStatus%` |
| 510100 | `blApiCreate` | `blCreate` | class, parent, local x/y/w/h, flags, action, tag, kind, button mask, text, value, mode, pad, gap, text properties | `blRetHandle%` |
| 510200 | `blApiDestroy` | `blDestroy` | `blReqHandle%` | `blStatus%` |
| 510300 | `blApiSetBounds` | `blSetBounds` | handle, local x/y/w/h | `blStatus%` |
| 510400 | `blApiShow` | `blShow` | handle | `blStatus%` |
| 510500 | `blApiHide` | `blHide` | handle | `blStatus%` |
| 510600 | `blApiSetText` | `blSetText` | handle, `blReqText$` | `blStatus%` |
| 510700 | `blApiSetValue` | `blSetValue` | handle, `blReqValue#` | `blStatus%` |
| 510800 | `blApiGetValue` | `blGetValue` | handle | `blRetValue#` |
| 510900 | `blApiSetLayout` | `blSetLayout` | handle, mode, pad, gap, cols | `blStatus%` |
| 511000 | `blApiSetAction` | `blSetAction` | handle, action | `blStatus%` |
| 511100 | `blApiInvalidate` | `blInvalidate` | handle | `blStatus%` |
| 511200 | `blApiLayoutAll` | `blLayoutAll` | — | `blStatus%` |
| 511300 | `blApiPaintAll` | `blPaintAll` | — | `blStatus%` |
| 511400 | `blApiUpdate` | `blUpdate` | handle | `blStatus%` |
| 511500 | `blApiPollInput` | `blPollInput` | — | count in `blRetN%` |
| 511550 | `blApiPollMouse` | `blPollMouse` | — | alias for `blPollInput` |
| 511600 | `blApiNextEvent` | `blNextEvent` | — | `blEvent...`, `blEventCount%` |
| 511700 | `blApiFindAt` | `blFindAt` | absolute `blReqX%`, `blReqY%` | `blRetHandle%` |
| 511800 | `blApiTestClickAt` | `blTestClickAt` | absolute `blReqX%`, `blReqY%`, `blReqButton%` | `blRetN%` |
| 511810 | `blApiTestMouseMove` | `blTestMouseMove` | absolute x/y | `blRetN%` |
| 511820 | `blApiTestMouseButton` | `blTestMouseButton` | button, down, x/y | `blRetN%` |
| 511830 | `blApiTestWheel` | `blTestWheel` | delta | `blRetN%` |
| 511900 | `blApiDump` | `blDump` | optional handle | debug print |
| 511910 | `blApiAutoSize` | `blAutoSize` | handle | `blStatus%` |
| 511920 | `blApiSetActionLine` | `blSetActionLine` | action ID, line | `blStatus%` |
| 511930 | `blApiRegisterClass` | `blRegisterClass` | class ID, method lines (v0.2) | `blStatus%` |
| 511950 | `blApiSetEnabled` | `blSetEnabled` | handle, `blReqEnabled%` | `blStatus%` |

Every public entry line begins with a `LABEL`:

```
blApiInit          blApiCreate        blApiDestroy       blApiSetBounds
blApiShow          blApiHide          blApiSetText       blApiSetValue
blApiGetValue      blApiSetLayout     blApiSetAction     blApiInvalidate
blApiLayoutAll     blApiPaintAll      blApiUpdate        blApiPollInput
blApiPollMouse     blApiNextEvent     blApiFindAt        blApiTestClickAt
blApiTestMouseMove blApiTestMouseButton blApiTestWheel   blApiDump
blApiAutoSize      blApiSetActionLine blApiRegisterClass blApiSetEnabled
```

Define each at its entry line, for example `510000 LABEL blApiInit` and
`510100 LABEL blApiCreate`. The host calls `GOSUB blApiCreate`, not
`GOSUB 510100`. Labels are compile-time and `RENUM`-safe; there is no
initialization order, no runtime variable to clobber, and no computed-line
lookup. The numeric lines remain the published contract for tools that still
need them.

Line numbers are the API contract. New routines can be added at unused lines; existing numbers do not move within a major version. `blSend`, `blFramePush`, `blFrameLoad`, `blFrameClose`, and `blFrameDrop` are internal lines, not public API.

### 14.3 Initialization sequence

```
SCREEN 720                    ' any graphics mode
blReqW% = 0 : blReqH% = 0     ' 0 means use SCREENW/SCREENH
GOSUB blApiInit                   ' blInit
IF blStatus% <> blStatusOk% THEN ...    ' handle failure

... create panels and controls with GOSUB blApiCreate ...

GOSUB blApiPaintAll                   ' blPaintAll (does layout if dirty)

loop:
  GOSUB blApiPollInput                 ' blPollInput
  WHILE blEventCount% > 0
    GOSUB blApiNextEvent               ' blNextEvent
    IF blEventAction% <> blActionNone% AND blActionLine&(blEventAction%) <> blNoLine& THEN GOSUB blActionLine&(blEventAction%)
  WEND
  IF blNeedsPaint% THEN GOSUB blApiPaintAll
  K$ = INKEY$ : IF K$ = CHR$(27) THEN END
  SLEEP 16                    ' pace the loop
  GOTO loop
```

`blPaintAll` must precede the first poll so `blCtlZ%` is valid. `blPollInput` can mark controls dirty; the host decides whether to call `blUpdate` or `blPaintAll`. The `bl...` constants are already available because `CONST` is initialized before the first program instruction; no constant initialization call is needed.

### 14.4 Host skeleton

The following is illustrative; line numbers below 500000 are the host’s.

```basic
10  REM baslib demo host
15  CONST ActButton% = 100 : CONST ActTarget% = 200
20  SCREEN 720
30  blReqW% = 0 : blReqH% = 0
40  GOSUB blApiInit
50  IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus%: END
60  REM --- panel ---
80  blReqClass% = blClassPanel% : blReqParent% = blRoot%
90  blReqX% = 40 : blReqY% = 40 : blReqW% = 520 : blReqH% = 360
100 blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
110 GOSUB blApiCreate : blPanel% = blRetHandle%
120 REM --- button ---
130 blReqClass% = blClassUser% : blReqParent% = blPanel%
140 blReqKind% = blKindButton% : blReqX% = 0 : blReqY% = 0 : blReqW% = 220 : blReqH% = 32
150 blReqText$ = "Ready" : blReqAction% = ActButton%
160 GOSUB blApiCreate : blButton% = blRetHandle%
170 REM --- label (multiline) ---
180 blReqClass% = blClassUser% : blReqParent% = blPanel%
190 blReqKind% = blKindLabel% : blReqX% = 0 : blReqY% = 40 : blReqW% = 300 : blReqH% = 40
200 blReqText$ = "Ready" + CHR$(13) + "second line"
205 blReqAlign% = 0 : blReqVAlign% = 1 : blReqTextScale% = 1
210 blReqTransparent% = 1 : blReqTextColor% = -1 : blReqFace% = -1
215 blReqPad% = 4 : blReqBorderStyle% = 0
220 GOSUB blApiCreate : blLabel% = blRetHandle%
230 REM --- click target (invisible hotspot) ---
240 blReqClass% = blClassClickTarget% : blReqParent% = blPanel%
250 blReqX% = 0 : blReqY% = 90 : blReqW% = 300 : blReqH% = 24
260 blReqText$ = "" : blReqAction% = ActTarget%
270 GOSUB blApiCreate : blClickTarget% = blRetHandle%
280 GOSUB blApiPaintAll
290 REM --- action lines ---
300 blReqActionId% = ActButton% : blReqLine& = 1000 : GOSUB blApiSetActionLine
310 blReqActionId% = ActTarget% : blReqLine& = 2000 : GOSUB blApiSetActionLine
320 REM --- main loop ---
330 GOSUB blApiPollInput
340 WHILE blEventCount% > 0
350   GOSUB blApiNextEvent
360   IF blEventAction% <> blActionNone% AND blActionLine&(blEventAction%) <> blNoLine& THEN GOSUB blActionLine&(blEventAction%)
370 WEND
380 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
390 SLEEP 16
400 GOTO 330
1000 REM button action
1010 blReqText$ = "Button clicked" : blReqHandle% = blLabel%
1020 GOSUB blApiSetText
1030 blReqHandle% = blLabel% : GOSUB blApiAutoSize : GOSUB blApiUpdate
1040 RETURN
2000 REM click target action
2010 blReqText$ = "Target clicked" : blReqHandle% = blLabel%
2020 GOSUB blApiSetText
2030 GOSUB blApiUpdate
2040 RETURN
```

In a real host, event handling includes `blEvMove`, `blEvEnter`, `blEvLeave`, `blEvDown`, `blEvUp`, and `blEvWheel`; the demo above only shows action dispatch.

---

## 15. Class behavior details

### 15.1 Top-level class (1)

State:

- `blRoot%` handle.
- `blHover%`, `blPressed%`, `blCapture%`, `blFocus%` (focus reserved).
- `blTopMouseX%`, `blTopMouseY%`, `blTopButton%` (last click, for convenience).

Methods:

- `Paint`: fill the root bounds with `blThemeDesktop%`; optionally draw a title/status strip.
- `Layout`: no child positioning; children keep local coordinates. The root’s client area is the full bounds.
- `HitTest`: return 1 for inside the root.
- `Click`: return 0 by default; a host can set a root action if it wants desktop clicks.
- `MouseEnter`/`MouseLeave`: update hover state; mark dirty.
- `MouseDown`/`MouseUp`: record pressed state; root may capture.
- `MouseMove`: update live coordinates; mark dirty if a hover visual exists.
- `Wheel`: return 0 by default; the host can handle it via events.

The core, not the top class, performs the hit-test walk and click bubbling. The top class owns input state and the future focus manager.

### 15.2 Panel class (2)

State: `blPanel%` fields from §8.3.

Methods:

- `Paint`: fill background, draw a one-pixel border, optionally draw `blCtlText$` as a title.
- `Layout`: position children according to mode.
- `HitTest`: return 1 inside the panel; children are considered first by z-order.
- `Click`: return 0 by default; if the panel has an action, return 1 and post it.
- `MouseEnter`/`MouseLeave`: hover state; mark dirty.
- `Wheel`: return 0 by default; the core bubbles wheel events.

Vertical layout:

```
x = Pad
y = Pad
for each child:
    child.X = x
    child.Y = y
    child.W = clientW
    y = y + child.H + gap
```

Horizontal layout mirrors this with `child.H = clientH` and `x` advancing. Hidden children and children with zero width or height are skipped and do not advance, so hiding a control closes the gap. `blShow` and `blHide` mark the parent tree layout-dirty.

### 15.3 User-control class (3)

The User class router uses computed `GOTO` tail dispatch:

```
blUk% = blUser#(blArgCtl%(blFp%), blUserKind%)
GOTO blUserMethod&(blUk%, blArgVerb%(blFp%))
```

Kinds:

- **Button (v0.1)**: beveled face, centered `DRAWTEXT` caption at `blUserTextScale%`, press/disabled state. Left click returns 1; right/middle click returns 0 and bubbles. Hover and pressed flags mark dirty.
- **Label (v0.1)**: multiline static text drawn with `DRAWTEXT`. Background fill is optional (transparent by default), border optional, text positioned with `blUserPad%`, `blUserAlign%`, `blUserVAlign%`, and `blUserTextScale%`. Text is truncated to available pixel width; auto-size uses `TEXTWIDTH`/`TEXTHEIGHT`. `HitTest` returns 0, so labels are click-through; a label never posts a click.
- **Slider (v0.2)**: horizontal/vertical track, thumb at value position. MouseDown/Move/Up map position to value, quantize to `Step`, clamp, and capture the mouse.
- **Gauge (v0.2)**: read-only bar or arc; no click; Wheel may be reserved.
- **Dial (v0.2)**: circular track and pointer; MouseDown/Move/Up map angle to value, quantize, clamp, and capture.

In v0.1 `blCreate` accepts user kinds 1 (button) and 5 (label); kinds 2–4 report error 10 until v0.2. The method table and value fields are in place from the start.

`blSetValue` validates range and marks the control for update; `blGetValue` returns the current value. Labels have no value; `blSetValue`/`blGetValue` on a label return error 10.

### 15.4 Label behavior (User kind 5)

State is stored in the User table:

- `blUserPad%` — inner padding.
- `blUserAlign%` — 0 left, 1 center, 2 right.
- `blUserVAlign%` — 0 top, 1 middle, 2 bottom.
- `blUserBorder%` — 0 none, 1 single-pixel border.
- `blUserTextColor%` — text color, or -1 for the theme text color.
- `blUserTextScale%` — DRAWTEXT scale 1–8.
- `blUserFace%` — background color when `blUserTransparent% = 0`; -1 means theme panel.
- `blUserTransparent%` — 1 = no fill, 0 = opaque.
- `blUserWrap%` — 0 in v0.1; 1 = word wrap in v0.2.
- `blUserAutoSize%` — 1 = measure text and adjust bounds.
- `blUserLines%` — cached rendered line count.

Paint:

1. If `blUserTransparent% = 0`, fill the control rectangle with `blUserFace%` (or the theme panel color). Otherwise leave the background untouched.
2. If `blUserBorder% = 1`, draw a one-pixel border. The text area is inset by `Pad + 1`.
3. Call `blTextRect` with the control rectangle, text, alignment, vertical alignment, padding, text color, and scale.
4. `blTextRect` splits at CR/LF, measures with `TEXTWIDTH`/`TEXTHEIGHT`, truncates each line to the available pixel width, and draws each line with `DRAWTEXT`.
5. If disabled, use `blThemeDisabled%` for the text instead of `blUserTextColor%`.

Behavior:

- Explicit multiline is supported; `CHR$(13)` and `CHR$(10)` start lines.
- Word wrap is v0.2 and uses `TEXTWIDTH` to fit words.
- `HitTest` always returns 0. A label is click-through and never blocks a button or click target underneath it.
- `Click` is not implemented; a label never posts a click and has no action.
- `Update` redraws the whole label; label paint is cheap.
- `blCreate` copies `blReqAutoSize%` into `blUserAutoSize%` and calls `blAutoSize` when it is set.
- `blAutoSize(handle%)` sets width/height from `TEXTWIDTH`/`TEXTHEIGHT` plus padding/border; `blUserAutoSize% = 1` requests the same after text changes.
- Labels have no value. `blSetValue`/`blGetValue` return error 10.

### 15.5 Click-target class (4)

- `Paint`: no-op; the target is invisible.
- `HitTest`: rectangular by default.
- `Click`: checks `blClickButtonMask%` (1 left, 2 right, 4 middle; combinations allowed), returns 1 if the button matches, else 0.
- `MouseDown`/`MouseUp`: update pressed state for visual overlays.
- `MouseEnter`/`MouseLeave`: update hot state.
- `Wheel`: return 0 or post a wheel event if `blClickWheel%` is set.
- Useful as an overlay child of a panel or a user control, and as the minimal test class for the event pipeline.

### 15.6 Shared drawing helpers

At 509500–509999:

All coordinates below are local control coordinates.

- `blBox(x%, y%, w%, h%, color%, fill%)`: clipping-aware rectangle.
- `blBevel(x%, y%, w%, h%, light%, dark%, face%)`: raised/sunken face.
- `blText(x%, y%, text$, fg%, scale%)`: one `DRAWTEXT` call at local `(x, y)`, with `blPaintOX%`/`blPaintOY%` applied.
- `blTextRect(x%, y%, w%, h%, text$, align%, valign%, fg%, scale%, pad%)`: multiline pixel text layout using `TEXTWIDTH`/`TEXTHEIGHT`, truncation, and padding.
- `blTextFit(text$, w%, scale%)`: returns the truncation length for a line; can use `TEXTWIDTH` for exactness.
- `blAutoSize`: measure and set bounds.
- `blValueToPos#(value#, min#, max#, length%)`: mapping.
- `blPosToValue#(pos%, length%, min#, max#)`: inverse mapping.
- `blClamp#(v#, lo#, hi#)`.
- `blPostEvent%`: enqueue.
- `blHitTest%`: hit test.
- `blFail%`: set `blStatus%`/`blError%`.

Helpers use `blPaintOX%`/`blPaintOY%` and the current clip. They do not assume the full screen.

---

## 16. Errors and validation

`blStatus%` is the primary result. `blError%` is a numeric code; `blError$` is a short message. The library does not raise BASIC runtime errors for expected failures (bad handle, full table, bad class), so a host `ON ERROR` handler is not surprised by library-internal errors. The status numbers below are published as the `blStatus...%` constants in §4.2.1; use the names in host code.

| Code | Meaning |
|---:|---|
| 0 | Success |
| 1 | Not initialized |
| 2 | Control table full |
| 3 | Invalid handle |
| 4 | Invalid class |
| 5 | Invalid verb |
| 6 | Frame stack overflow |
| 7 | Invalid parent / would create a cycle |
| 8 | Cannot destroy root |
| 9 | Invalid geometry |
| 10 | Invalid value/argument |
| 11 | Invalid layout mode |
| 12 | Event queue full |
| 13 | Empty event queue |
| 14 | No graphics screen / zero canvas |
| 15 | STACK frame protocol error |
| 16 | Missing action-line handler |
| 17 | Class registration full (v0.2) |

Validation rules:

- Handles must be 1..255 and have a non-zero class.
- Class 1 (Top) is created only by `blInit`; `blCreate` rejects it.
- `blCreate` accepts classes 2, 3, and 4. Classes 5–16 are reserved and rejected.
- In v0.1 the User class accepts kind 1 (button) and kind 5 (label); kinds 2–4 are rejected until v0.2.
- Text scale must be 1–8; transparency must be 0 or 1; wrap 0/1.
- Panel mode must be 0, 1, or 2 in v0.1; mode 3 is rejected.
- Parent must exist and not be the child itself or a descendant. Parent 0 is only valid for the root.
- Width and height must be non-negative; zero is allowed but invisible.
- Coordinates are clamped to the 16-bit range; extreme values are rejected rather than wrapped.
- `blCreate` validates everything before allocating or linking. On failure, no partial control remains.
- `blDestroy` clears capture/focus/event references to the handle and dispatches `CaptureLost` if needed.
- A frame that cannot be validated is dropped with a default result; it is never left on `STACK`.
- `blAutoSize` rejects handles without text or with invalid scale.
- `blSetActionLine` validates action 0–255 and line 0–2147483647. The language now
  accepts signed 32-bit lines, but the baslib action-line contract stays within
  the GW-BASIC range.

---

## 17. Concurrency, timers, and reentrancy

baslib is synchronous. The host calls it from its main loop. `ON TIMER` is the only asynchronous BASIC mechanism and runs between statements.

Rules:

- A timer handler must not call baslib APIs. It should set a flag and return.
- The host processes timer flags at the top of its main loop, outside any baslib dispatch.
- If a timer interrupts a baslib call, shared scratch variables could be clobbered. The `STACK` frame and per-depth locals protect method arguments, but not arbitrary temporaries. The no-call rule avoids the problem.
- `blPollInput`, `blPaintAll`, and `blLayoutAll` are not reentrant with respect to themselves.
- Nested class dispatch is supported through `blSend`; it is depth-bounded and frame-balanced.
- `SLEEP` is called only by the host, not inside library dispatch.

This is stricter than necessary for a single-threaded interpreter, but it keeps the library predictable.

---

## 18. Performance

Target: 255 controls, current canvas, interactive input.

- Class/verb dispatch is one array lookup plus one computed `GOSUB`, O(1).
- Paint visits every visible control once. Child lists are walked once per container.
- Layout is O(visible controls + edges) when dirty, not per frame.
- Hit test is O(255) per input event. At 255 controls this is trivial and avoids a spatial index.
- Event queue is a fixed ring; move events are coalesced.
- No `DIM` in loops, no `ON ERROR` in hot paths, no string concatenation in paint.
- `blUpdate` redraws one control’s clip instead of the whole tree.
- Text helpers measure with `TEXTWIDTH`/`TEXTHEIGHT` only when layout or text changes; paint uses cached metrics for fixed-size controls.
- `PUSH VIEW`/`POP VIEW` per control is a small constant cost and preserves host state.

Potential hot spots:

- `VIEW` changes per control during paint.
- `DRAWTEXT` per label; scale 2–8 multiplies pixel work.
- `LINE ... BF` for large panels.
- O(255) hit test if the host polls every frame.
- `STACK` traffic for frames and work items; each dispatch is 14 pushes plus 14 pops.

No optimization should compromise the fixed-array, no-pointer model.

---

## 19. Testing plan

### 19.1 Test layers

1. **STACK protocol tests**: frame push/load/close/drop, result order, nested frames, depth 32/33, invalid class/verb cleanup, host sentinel preservation, frame balance after every API.
2. **Table/core tests**: init, allocate/free, create/link/detach, parent/child consistency, handle exhaustion, destroy root refusal.
3. **Dispatch tests**: method table class × verb, User kind × verb, computed GOTO tail dispatch, no-op line, invalid ID, dynamic registration (v0.2).
4. **Layout tests**: local→absolute, padding, vertical/horizontal stacks, hidden children, clipping, work-stack order.
5. **Paint tests**: pixel probes with `POINT`, host `VIEW` preservation via `PUSH VIEW`/`POP VIEW`, text truncation, multiline labels, auto-size, label alignment, `DRAWTEXT` transparency/scale/clipping, theme colors.
6. **Input tests**: hit-test z-order, disabled pass-through, label click-through, click bubbling, hover/enter/leave, press/release, wheel, capture/drag, action events, test hooks.
7. **Class tests**: button action, label text/alignment/transparency/scale/click-through, click target mask. v0.2: slider mapping/quantization/dragging, gauge read-only, dial angle mapping.
8. **Regression tests**: host merge, init failure paths, re-init, timer non-reentrancy rule, `SCREENW`/`SCREENH` root sizing, `blActionLine&` dispatch.
9. **Line-number width tests**: method/action tables use Long values above 499999; `ERL` reports library lines above 499999 correctly (newbasic `ERL` is 32-bit); symbolic `LABEL` calls resolve after `RENUM`; computed line targets use `&`, not `%`.

### 19.2 How to run them

- State-only tests can run under `newbasic --cli` and print `PASS`/`FAIL` lines; a fixture `.expected` file compares output.
- Graphics tests use a graphics `SCREEN` and `POINT(x,y)` probes after `blPaintAll`; works headlessly because the screen canvas is still present.
- For visual inspection, run the demo in a window and capture with `--gpu-shot FILE` (PPM).
- Synthetic input uses `blTestClickAt`, `blTestMouseMove`, `blTestMouseButton`, and `blTestWheel`, so tests do not depend on OS events.
- A sentinel test `STACK PUSH 12345 : GOSUB <api> : STACK POP X% : IF X% <> 12345 ...` verifies that no library call leaks or consumes host stack values.

### 19.3 Acceptance criteria for v0.1

- A host can merge the library, initialize a root, create a panel with a button, a label, and a click target, lay them out, paint, click, and receive events.
- A host `VIEW` is still active after `blPaintAll`/`blUpdate`; `VIEWX1`/`Y1`/`X2`/`Y2` match the host viewport.
- A label renders multiline text, aligns left/center/right and top/middle/bottom, truncates at pixel width, supports padding/border/transparency, and auto-sizes with `blAutoSize`.
- Hover, enter/leave, press/release, wheel, and click events are observed through the event queue.
- `blActionLine&` dispatch invokes the correct host line.
- Re-running `blInit` resets all state without leaking handles; every handle from before the re-init is invalid.
- 255 controls can be created; the 256th fails cleanly with code 2.
- A 32-deep dispatch succeeds; the 33rd fails with code 6 and every frame is balanced.
- `STACK` has no baslib leakage; host sentinels survive API calls.
- No baslib call raises an unexpected BASIC runtime error in the test suite.
- The demo source stays below line 500000 and the library stays within 500000–519999.
- `blInit` works with a non-720 graphics canvas and sizes the root from `SCREENW`/`SCREENH`.

---

## 20. Planning decisions recorded

### 20.1 Input

v0.1 is **live-polled**: click, hover, enter/leave, press/release, and wheel. The click queue remains the action path; live state drives visuals and future drag. Drag/capture is implemented in the core; continuous controls use it in v0.2.

`SLEEP` is host-loop pacing, not a library primitive. `blPollInput` never blocks.

### 20.2 v0.1 control set

v0.1 ships the **button, label, and click target**, plus Top and Panel. Slider, gauge, and dial are v0.2. The class table, user-kind table, value storage, and capture fields are present from the start.

### 20.3 Library location

```
baslib/
    baslib-plan.md
    baslib-imp.md
    baslib.bas
    demo.bas
    demo-controls.bas
    tests/
```

The early sketches remain under `samples/baslib/` for reference; their manual stack is superseded by `STACK`.

### 20.4 Display and text

baslib requires a graphics canvas but not a specific mode. `blInit` reads `SCREENW`/`SCREENH` unless the host supplies `blReqW%`/`blReqH%`. `DRAWTEXT` plus `TEXTWIDTH`/`TEXTHEIGHT` supplies deterministic text layout. Explicit multiline and auto-size are v0.1; word wrap is v0.2.

### 20.5 Timer policy

baslib calls are synchronous. An `ON TIMER` handler must not call baslib; it should set a flag and return, and the host should handle the flag at the top of its main loop. A deferred-command queue can be considered later if timer-driven animation becomes necessary.

### 20.6 Deferred work

- Keyboard focus, tab order, and text entry.
- Word-wrapped labels (v0.2).
- Slider, gauge, and dial implementations (v0.2).
- Dynamic class registration (v0.2).
- Multi-window support.
- Dirty-rectangle compositor and double buffering.
- Screen-mode changes while controls exist.
- Serial/joystick/light-pen input.
- Full theme editor.

---

## 21. Implementation order

1. **Scaffold** `baslib.bas` with the line map, version constants, and the `500000 END` guard.
2. **STACK frame protocol**: push/load/dispatch/close/drop, depth, per-depth cache/locals, sentinel tests.
3. **Method tables and computed dispatch**: `blMethod&`, `blUserMethod&`, no-op lines, invalid cleanup.
4. **Control table** with allocation, create, link, detach, destroy; test tree invariants.
5. **Layout** for root and panel absolute/vertical/horizontal using the `STACK` work stack.
6. **Paint** with per-control `PUSH VIEW`/`VIEW`/`POP VIEW`; test `POINT` and host viewport preservation.
7. **Input** with live polling, hover/press, click bubbling, wheel, capture, event queue; test hooks.
8. **Button, label, and click target**; multiline/auto-size label; first interactive demo.
9. **Action lines, theme, text helpers, polish**; pixel probes and documentation.
10. **v0.2**: slider/gauge/dial with drag, word wrap, dynamic class registration.
11. **Deferred**: keyboard, multiple windows, compositor.

Each step ends with a runnable test and a short note in this file. No step depends on a later one.

---

## 22. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Host uses `OPTION BASE 1` | Library arrays shift; every index is wrong. | Document as a hard contract; test with base 0; fail loudly in the README. |
| Host uses `DEFINT B` or similar | Unsuffixed library variables change type. | Explicit suffix on every variable and array; no exceptions. |
| Host variable name collides with `bl...` | Silent state corruption. | Namespace rule in README and demo; keep all names prefixed. |
| Host uses `STACK` too | A library frame leak or over-pop corrupts host data. | Fixed 14-slot frames; sentinel tests; never pop below baslib’s frames; drop invalid frames. |
| Unexpected runtime error inside a method | Frame left on `STACK`; depth wrong. | Use status returns for expected failures; host `CLEAR` + `blInit` on unexpected errors; tests assert balance. |
| No `STACK` depth query | Library cannot see how many host values are below. | Track `blFp%` and `blWorkCount%`; only pop known frame/work items. |
| Method table uninitialized | Computed `GOSUB 0` or wrong handler. | `blInit` fills every cell with the no-op line; validate line <> 0 before dispatch. |
| Computed `GOTO` into a `SELECT CASE` | Runtime `GOTO would leave a SELECT CASE`. | Use `GOSUB` or structure tail dispatch outside SELECT. |
| `VIEW` push/pop imbalance | Host viewport shifts; later drawing wrong. | Per-control `PUSH VIEW`/`POP VIEW`; test `VIEWX1`/`Y1`/`X2`/`Y2` after paint. |
| Input event storm | Queue fills; UI lags. | Coalesce move events; bounded ring; process at most eight clicks per poll. |
| Click and live button double-dispatch | Duplicate actions or missed drags. | Click queue is the action path; live buttons only drive down/up/hover/drag. |
| Capture points at destroyed control | Crash or wrong drag target. | Clear capture on destroy/hide; post `CaptureLost`. |
| Text measurement mismatch | Bad centering/truncation at scale. | Use `TEXTWIDTH`/`TEXTHEIGHT` for all layout; avoid `LEN*8` assumptions. |
| Screen mode changed at runtime | Root and clips stale. | `blInit` requires a stable graphics mode; re-init after `SCREEN`. |
| Host forgets `blInit` after `RUN`/`LOAD` | Stack/variables empty; status 1. | Document; `blStatus% = blStatusNotInit%` guard. |
| `MERGE` workflow forgotten | API lines absent. | Demo build script and README. |
| Library too large | Long compile/run times. | Shared class code; avoid duplicated per-kind boilerplate; measure line count per milestone. |
| `ERL` width | Errors in library lines above 499999 could report the wrong `ERL`. | **Fixed 2026-10-24:** newbasic `ERL` returns a 32-bit Long. Keep `blStatus%`/`blError%` for expected failures. |
| String-array memory | 255 controls × several string arrays. | Two common string arrays plus small class text; keep captions short. |

---

## 23. Glossary

- **Class** — a handler bundle shared by instances; identified by a small integer.
- **Instance / handle** — one control row, 1–255; 0 is null.
- **Verb** — an operation dispatched to a class (`Paint`, `Click`, `MouseMove`, …).
- **Method table** — `blMethod&(class, verb)` line-number table used by computed `GOSUB`.
- **Frame** — one 14-slot `STACK` block plus per-depth argument cache/locals.
- **Depth (`blFp%`)** — current baslib frame index; host stack values are below it.
- **Work stack** — `STACK` items used by iterative layout/paint/destroy traversal.
- **Capture** — a control receiving mouse move/up while a button is held.
- **Hover** — live pointer over a control; tracked separately from clicks.
- **Layout pass** — computes local→absolute rectangles and clips.
- **Paint pass** — walks the tree back-to-front, sets `VIEW`/clip, and asks each class to paint itself.
- **Client area** — a panel’s inner rectangle after border/padding.
- **Z-order** — sibling order; later siblings paint in front and win hit tests.
- **Bubbling** — if a control does not handle a click, its parent is offered the click.
- **Auto-size** — set a control’s width/height from `TEXTWIDTH`/`TEXTHEIGHT`.
- **Action line** — host line number stored in `blActionLine&` for an action ID.

---

## 24. Change log

- **2026-10-23** — Initial plan. Accepted packaging, handle model, two-level dispatch, frame stack, and local-coordinate layout. Recorded the click-only input constraint.  
- **2026-10-23** — Closed planning choices: click-only v0.1, button + click target first, `SCREEN 720` required, and the library promoted to top-level `baslib/`.  
- **2026-10-23** — Confirmed click bubbling, absolute/vertical/horizontal layout for the first implementation, and request-global creation APIs.  
- **2026-10-24** — Rebased baslib from 10000–29999 to **500000–519999**. Host owns 1–499999; other libraries may use 520000–999999. The old GW-BASIC 65529 ceiling no longer applies; baslib uses newbasic’s signed 32-bit line range.  
- **2026-10-24** — Added **Label** as User kind 5: single-line text, horizontal/vertical alignment, padding, optional border, configurable text/background colors, and click-through hit testing.  
- **2026-10-24** — Adopted **DRAWTEXT** for text. Labels default to transparent backgrounds and support text scale 1–8.  
- **2026-10-24** — **Draft 0.2 redesign** after newbasic added computed `GOTO`/`GOSUB`, `STACK`, `SELECT CASE`, `SLEEP`, live mouse state, `TEXTWIDTH`/`TEXTHEIGHT`, `SCREENW`/`SCREENH`, `VIEW` queries, and `PUSH VIEW`/`POP VIEW`:
  - replaced static `ON ... GOSUB` dispatch with a class × verb method table and computed `GOSUB`;
  - replaced the hand-rolled frame/work arrays with a 14-slot `STACK` frame protocol and `STACK` work stack;
  - moved from click-only to live hover/press/wheel input, with capture/drag infrastructure for v0.2;
  - made painting preserve the host viewport with `PUSH VIEW`/`POP VIEW`;
  - made the root size from `SCREENW`/`SCREENH` instead of a fixed 1280×720;
  - added exact text metrics, multiline labels, and auto-size;
  - added action-line dispatch and v0.2 dynamic class registration;
  - changed every line-number-bearing table/parameter to Long (`&`) to keep line numbers visually distinct and match the API contract (both `%` and `&` are now 32-bit in newbasic).
- **2026-10-24** — Added `LABEL` to newbasic and adopted symbolic API entry points (`GOSUB blApiInit`, `GOSUB blApiCreate`, ...). The numeric lines remain the contract; labels make host calls readable and survive `RENUM`.
- **2026-10-24** — Added `CONST` to newbasic; baslib limits are read-only constants rather than host-writable variables.
