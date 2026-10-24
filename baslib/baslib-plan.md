# baslib — a generic BASIC control framework

**Status:** Draft 0.2 — planning only. No library code has been written yet.  
**Target:** newbasic (GW-BASIC-compatible dialect in this repository)  
**Last updated:** 2026-10-24

This document records the design agreed so far. The existing sketches are in [`stackish.bas`](../samples/baslib/stackish.bas) and [`windows.bas`](../samples/baslib/windows.bas); they are prototypes, not the final implementation. The real library, demo, tests, and this plan live in the top-level `baslib/` directory.

> **Draft 0.2.** newbasic now has computed `GOTO`/`GOSUB`, a user-visible
> `STACK`, `SELECT CASE`, `SLEEP`, live mouse state, `TEXTWIDTH`/`TEXTHEIGHT`,
> `SCREENW`/`SCREENH`, viewport queries, and `PUSH VIEW`/`POP VIEW`. These remove
> the old static-dispatch, click-only, fixed-screen and text-cell constraints.
> The frame stack, dispatch, input, painting and layout sections below have been
> redesigned around them. Draft 0.1 details that no longer apply were removed.

---

## 1. Objective

Create a reusable BASIC source library that manages a tree of graphical controls:

- a single top-level control that owns the screen and receives mouse input;
- panels that contain other controls and determine their layout;
- user controls that paint themselves (v0.1: button and label; v0.2: slider, gauge, dial);
- click targets that respond to mouse input.

Each control has a class. Every instance of a class shares one class handler. Instances have independent state, stored in arrays. Parameters and return values travel through the language `STACK`; class and verb handlers are reached through computed `GOSUB` and computed `GOTO`.

The library is a *source fragment*, not an interpreter extension. A host program merges it with its own source and calls fixed line-number entry points.

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
- Changing `SCREEN` mode while controls exist. `BlInit` supports any graphics canvas that is already selected.

---

## 2. Newbasic capabilities and constraints

These are facts from the current implementation and reference manual. They drive the design.

| Capability / constraint | Consequence |
|---|---|
| No `INCLUDE` or import statement. `MERGE` is direct-mode only. | baslib is a plain source fragment with fixed line numbers. It is merged or concatenated before `RUN`. |
| `GOSUB` accepts a literal line or a numeric expression. Literal targets keep compile-time resolution. | The public API remains a line-number contract, but class/verb dispatch can fetch a method line from a table and `GOSUB` it. |
| `GOTO` accepts a literal line or a numeric expression. `GOTO` out of an active `SELECT CASE` is a runtime error. | Computed `GOTO` is used for tail dispatch inside a method (the pending `GOSUB` return is preserved). Do not use it to leave a `SELECT CASE`; use computed `GOSUB` where a return is needed. |
| `STACK PUSH` / `STACK POP` / `STACK TOP` / `STACK PEEK` maintain one global LIFO stack of BASIC values. Whole arrays are copied by value. List operations are all-or-nothing. There is no stack-depth query. | baslib uses `STACK` as its call-frame transport and work stack. It must keep exact push/pop discipline and must never assume the stack is otherwise empty. |
| `STACK` is cleared by `CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN`, but is preserved between direct-mode commands and is not touched by `GOSUB`/`RETURN`. | A host may use `STACK` too. baslib frames must be balanced and transparent to values below them. `BlInit` is required after any operation that clears the stack. |
| `STACK` underflow raises **Stack empty** (error 77). The stack is capped at 1,000,000 entries and elements. | Frame and work-stack code validates its own counts; it never relies on underflow as control flow. |
| `ON expression GOSUB` still selects from a static list, but it is no longer required for dispatch. | baslib uses data-driven method tables, not per-class `ON` lists. Class and verb IDs are bounded by table dimensions, not by the 255-entry `ON` limit. |
| `SELECT CASE` supports value lists, `CASE IS`, `CASE ELSE`, and nesting. `CASE 1 TO 5` is not implemented. | `SELECT CASE` is used for validation, event state, alignment, and other small enum logic. Dispatch stays table-driven. |
| Newbasic has an internal `GOSUB`/`RETURN` stack, so nested calls work. It has no user-defined local variables. | The `STACK` frame plus depth-indexed local arrays supply arguments, locals, and results. GOSUB nesting itself is not the problem. |
| Arrays are 0-based unless `OPTION BASE 1` is in effect. There is one program-wide base. | baslib requires base 0 and reserves index 0 as “null”. The host must not use `OPTION BASE 1`. |
| `DIM` is executable and re-`DIM` resets an array. | Initialization is explicit through `BlInit`. The library is not self-starting. |
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
| `RENUM` renumbers the whole merged program and caps at 65529. | Do not run `RENUM` after merging if fixed API lines matter. All baslib lines stay at or below 65529. |

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
| Packaging | One self-contained `.BAS` fragment in lines **30000–49999**; host owns **1–29999**. Merged with `MERGE` or concatenated at build time. |
| Instance model | One global handle space (1–255). Common fields in a shared control table; class-specific state in per-class tables indexed by the same handle; text in separate `$` arrays. |
| Dispatch | Data-driven method table: `BlMethod%(class, verb)` stores a line number; `GOSUB BlMethod%(class, verb)`. No per-class static `ON` lists. |
| User kinds | Separate `BlUserMethod%(kind, verb)` table, reached by computed `GOTO` tail dispatch from the User class router. |
| Call protocol | Fixed 14-slot frame on `STACK`, `BlFp%` depth counter, depth-indexed argument cache and locals, one numeric/one string result. |
| Geometry | Child coordinates are local to the parent. A layout pass computes absolute rectangles and clip rectangles. |
| Layout modes | Absolute, vertical stack, and horizontal row in v0.1; grid is reserved. |
| Click delivery | Bubble from the hit control to its parent until one handles the click; the root is last. |
| Input | Click, hover, enter/leave, press/release, and wheel in v0.1. Drag/capture is in the core; continuous controls use it in v0.2. |
| Text | `DRAWTEXT` plus `TEXTWIDTH`/`TEXTHEIGHT`; labels support explicit multiline and auto-size. Word wrap is v0.2. |
| Display | Any graphics canvas selected before `BlInit`; root defaults to `SCREENW`/`SCREENH`. |
| Painting | Per-control `PUSH VIEW` / `VIEW clip` / `POP VIEW`; the host viewport is preserved. |
| v0.1 controls | Top-level, panel, button, label, and click target. Slider, gauge, and dial are v0.2. |
| Action dispatch | Host action IDs plus an optional `BlActionLine%(action)` line table and `BlSetActionLine`; the host can `GOSUB` the line directly. |
| Source location | Top-level `baslib/` directory for library, demo, tests, and this plan. `samples/baslib/` keeps the prototypes. |

---

## 4. Source, line numbers, and merge contract

### 4.1 File layout

```
baslib/
    baslib-plan.md          this document
    baslib.bas              the library fragment (30000-49999)
    demo.bas                a host program (1-29999)
    tests/                  BASIC test programs and expected output

samples/baslib/
    stackish.bas            early manual-stack prototype; superseded by STACK
    windows.bas             early control-table sketch; superseded by baslib.bas
```

`samples/baslib/` keeps the prototypes for reference. The final library is not a runnable program by itself: it has no main.

### 4.2 Line-number map

| Lines | Contents |
|---|---|
| 1–29999 | Host program. baslib never uses these. |
| 30000 | Hard `END` guard. If host execution falls through 29999, it stops instead of running library subroutines. |
| 30010–30099 | Version constants, limits, initialization flags, theme defaults. |
| 30100–30299 | `STACK` frame protocol: push, load, dispatch, close, drop; per-depth argument cache and locals. |
| 30300–30599 | Errors, validation, string and small numeric helpers. |
| 30600–30999 | Method tables, action-line table, class/kind registration data. |
| 31000–31999 | Control table: allocation, free list, creation/destruction, tree links. |
| 32000–32999 | Dispatch, message helpers, result transport. |
| 33000–33999 | Layout pass, absolute rectangles, clipping, work-stack traversal. |
| 34000–34999 | Paint pass, `PUSH VIEW`/`VIEW`/`POP VIEW`, dirty/update plumbing. |
| 35000–35999 | Input polling, hover/press/capture, hit testing, event queue. |
| 36000–36999 | Top-level class methods. |
| 37000–37999 | Panel class methods. |
| 38000–38999 | User class methods and kind-specific paint/update/click code. |
| 39000–39499 | Click-target class methods. |
| 39500–39999 | Shared drawing, text, geometry, event and value helpers. |
| 40000–41999 | Public API entry points. |
| 42000–48999 | Debug/test helpers and example-only code if needed. |
| 49000–49999 | Reserved for future public API. |

Line assignment inside each block is flexible; the block boundaries are the contract. Public API line numbers from §14 are stable once v0.1 ships.

The line space is partitioned:

| Lines | Owner |
|---|---|
| 1–29999 | Host program |
| 30000–49999 | baslib |
| 50000–65529 | Other libraries (do not overlap baslib) |

Although newbasic’s line store accepts numbers up to 999999, `RENUM` caps generated line numbers at **65529**. Keeping all static line numbers at or below 65529 preserves GW-BASIC compatibility and leaves headroom.

`RENUM` renumbers the entire merged program and would destroy the fixed API map (`GOSUB 40000`, `GOSUB 40100`, and so on). Do not run `RENUM` after merging if code or documentation relies on the stable baslib entry points; renumber the host before merging, or leave the merged program unrenumbered.

### 4.3 Host workflow

At the `Ok` prompt:

```
LOAD "DEMO.BAS"
MERGE "BASLIB.BAS"
RUN
```

Or, for a build or launch script, concatenate the two text files in line-number order. The host must not define lines in 30000–49999.

The host must also honor these rules:

- Do not use `OPTION BASE 1`. baslib requires the default base 0.
- Do not define variables beginning with `Bl` (case-insensitive). That is the baslib namespace.
- Do not rely on default numeric types inside baslib; the library uses explicit suffixes, and host code should use matching types in request variables.
- End the host’s main flow with `END` or a loop. The `30000 END` guard is a safety net, not a substitute.
- Select a graphics screen (for example `SCREEN 720`) before `BlInit`. baslib reads `SCREENW`/`SCREENH`; it does not change the screen mode or palette.
- Call `BlInit` before any other API.
- Host `RUN`, `LOAD`, `NEW`, or `CHAIN` clears `STACK` and variables; run `BlInit` again afterward.
- The host may keep its own values on `STACK`; baslib frames are balanced above them. A host sentinel (`STACK PUSH 12345` before a baslib call, `STACK POP` after) is a useful integration test.
- A host `ON ERROR` handler that catches an unexpected baslib runtime error should `CLEAR` and reinitialize; baslib avoids expected runtime errors, but an exceptional error can leave a frame on `STACK`.

### 4.4 Why a fixed line range

The public API is a line-number contract. Even with computed `GOSUB`, the host must be able to call literal entry points such as `GOSUB 40100`, and baslib methods are lines in a fixed range. Keeping the whole library in 30000–49999 also makes `MERGE` predictable: library lines never replace host lines.

---

## 5. Namespace and typing rules

All library names use the `Bl` prefix:

- Numeric scalars: `BlName%` (integer), `BlName&` (long), `BlName!`/`BlName#` (floating).
- String scalars: `BlName$`.
- Arrays: `DIM BlName%(...)`, `DIM BlName#(...)`, `DIM BlName$(...)`.

Every variable gets an explicit suffix. This protects the library from a host `DEFINT`, `DEFSNG`, `DEFDBL`, or `DEFSTR` that applies to the initial letter `B`.

Rules:

- Handles, coordinates, flags, counts, class IDs, verb IDs, and array indices are `%` (16-bit signed). This is sufficient for 255 controls and the current canvas sizes.
- Values that can exceed 32767 use `#` (double).
- Text uses `$`.
- Never use an unsuffixed scalar for persistent state.
- Never use `Bl` names for host data.
- `BlStatus%` is 0 on success and non-zero on failure. The library validates inputs and returns status instead of raising runtime errors wherever possible, so a host `ON ERROR` trap is not disturbed.

---

## 6. Architecture

```
Host program (1-29999)
    │
    ├── public API (40000-41999)
    │       init, create, layout, paint, poll, events, setters
    │
    ├── core
    │       control table, handles, tree, class registry
    │
    ├── STACK frames
    │       fixed frames, per-depth argument cache/locals, result pairs
    │
    ├── dispatch
    │       BlMethod%(class, verb) → computed GOSUB
    │       BlUserMethod%(kind, verb) → computed GOTO tail dispatch
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

`BlMaxVerbs% = 20` leaves room for future verbs without changing the table shape. Every method slot in `BlMethod%` and `BlUserMethod%` is initialized to a class-local no-op line; a missing implementation is explicit, not accidental fall-through.

### 7.2 Built-in classes

| ID | Class | Implements | Notes |
|---|---|---|---|
| 1 | Top | Paint, Layout, HitTest, Click (optional) | Root container and input owner. One instance, handle 1. |
| 2 | Panel | Paint, Layout, HitTest, Click (optional) | Container; computes child layout and draws background/border. |
| 3 | User | Paint, Update, Click, HitTest, MouseDown/Up/Move/Enter/Leave, Wheel | One class; `BlUserKind%` selects button, label, slider, gauge, or dial. |
| 4 | ClickTarget | HitTest, Click, MouseDown/Up/Enter/Leave, Wheel | Invisible hotspot; can also be a transparent overlay child. |
| 5–16 | Reserved | — | Method table points to the no-op line. |

Class IDs 1–4 are defined for v0.1. The shipped v0.1 methods are Top, Panel, User (button and label kinds), and ClickTarget; the slider/gauge/dial branches are added in v0.2. Adding a built-in class means filling its method table row and adding method lines; there is no static `ON` list to edit.

Class metadata:

```
BlClassName$(1..BlMaxClasses%)      debug name
BlClassFlags%(1..BlMaxClasses%)     default flags
BlClassFace%(1..BlMaxClasses%)      default face colour
BlClassText%(1..BlMaxClasses%)      default text colour
BlClassAccent%(1..BlMaxClasses%)    default accent colour
BlClassMethodCount%(...)            optional bookkeeping
```

`BlInit` fills classes 1–4 and leaves 5–16 zeroed except for the no-op method lines. A zero metadata row does not imply a callable class; `BlMethod%` must be populated.

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

Label remains a User kind rather than a separate class. It shares the User handler, needs no new class table row, and leaves class IDs 5–16 free.

---

## 8. Instance storage

### 8.1 Limits

| Constant | Value | Meaning |
|---|---:|---|
| `BlMaxControls%` | 255 | Handles 1–255; index 0 is null. |
| `BlMaxFrames%` | 32 | Maximum nested dispatch depth. |
| `BlMaxArgs%` | 8 | Numeric arguments per frame. |
| `BlMaxStrArgs%` | 2 | String arguments per frame. |
| `BlMaxClasses%` | 16 | Method-table rows. |
| `BlMaxVerbs%` | 20 | Method-table columns. |
| `BlMaxKinds%` | 8 | User-kind method rows. |
| `BlMaxEvents%` | 64 | Event ring size. |

A 255-handle limit keeps the control table bounded and matches the event action ID range. Class and verb counts are no longer tied to the `ON ... GOSUB` 255 limit; 16 × 20 is a table, not a list. Raising a limit is a source edit plus rechecking the table initialization.

### 8.2 Common control table

`BlCtl%(handle, field)` holds numeric state common to every control. Column 0 is unused.

| Field | Name | Type | Meaning |
|---:|---|---|---|
| 1 | `BlFClass%` | int | Class ID; 0 means the handle is free. |
| 2 | `BlFParent%` | int | Parent handle; 0 for the root. |
| 3 | `BlFFirstChild%` | int | First (back-most) child. |
| 4 | `BlFLastChild%` | int | Last (front-most) child. |
| 5 | `BlFPrevSibling%` | int | Previous sibling. |
| 6 | `BlFNextSibling%` | int | Next sibling. |
| 7 | `BlFX%` | int | Local left. |
| 8 | `BlFY%` | int | Local top. |
| 9 | `BlFW%` | int | Width. |
| 10 | `BlFH%` | int | Height. |
| 11 | `BlFAbsX%` | int | Absolute left after layout. |
| 12 | `BlFAbsY%` | int | Absolute top after layout. |
| 13 | `BlFFlags%` | int | Bit flags. |
| 14 | `BlFState%` | int | Class-specific state (normal/hot/pressed/etc.). |
| 15 | `BlFAction%` | int | Host action ID posted with events. |
| 16 | `BlFTag%` | int | Host-owned numeric tag. |
| 17 | `BlFCapture%` | int | Non-zero when this handle owns mouse capture. |

String state:

```
BlCtlText$(handle)     caption / label text
BlCtlName$(handle)     optional debug/name text
```

Additional per-control arrays:

```
BlCtlZ%(handle)        paint order, 0 = not painted/visible
BlCtlClipL%(handle)    cached absolute clip rectangle
BlCtlClipT%(handle)
BlCtlClipR%(handle)
BlCtlClipB%(handle)
BlFreeNext%(handle)    free-list link
```

Flags:

| Bit | Name | Meaning |
|---:|---|---|
| 1 | `BlFlagVisible%` | Control and subtree are visible. |
| 2 | `BlFlagEnabled%` | Accepts input. |
| 4 | `BlFlagClip%` | Clip children to this control’s bounds. |
| 8 | `BlFlagDirty%` | Needs repaint. |
| 16 | `BlFlagHot%` | Pointer is over this control. |
| 32 | `BlFlagPressed%` | Left/right/middle button is held on this control. |
| 64 | `BlFlagClickable%` | Optional hint; actual behavior is the class `Click` method. |
| 128 | `BlFlagFocusable%` | Reserved for keyboard focus. |
| 256 | `BlFlagTransparent%` | Paint is a no-op; hit test still applies. |
| 512 | `BlFlagLayoutDirty%` | Subtree needs layout. |
| 1024 | `BlFlagCaptured%` | This control owns mouse capture. |

### 8.3 Class-specific tables

Panel (`BlPanel%(handle, field)`):

| Field | Name | Meaning |
|---:|---|---|
| 1 | `BlPanelMode%` | 0 absolute, 1 vertical stack, 2 horizontal row, 3 grid. |
| 2 | `BlPanelPad%` | Inner padding. |
| 3 | `BlPanelGap%` | Gap between children. |
| 4 | `BlPanelCols%` | Grid column count (future). |
| 5 | `BlPanelRowH%` | Default row height (future). |
| 6 | `BlPanelBg%` | Background palette index. |
| 7 | `BlPanelBorder%` | Border palette index. |
| 8 | `BlPanelReserved%` | Future. |

User (`BlUser#(handle, field)`):

This is a **double** array because slider/gauge/dial values are floating point. Integer-valued fields are stored as doubles. The names in the Field column are integer column-index constants.

| Field | Name | Meaning |
|---:|---|---|
| 1 | `BlUserKind%` | 1 button, 2 slider, 3 gauge, 4 dial, 5 label. |
| 2 | `BlUserValue%` | Current value. |
| 3 | `BlUserMin%` | Minimum value. |
| 4 | `BlUserMax%` | Maximum value. |
| 5 | `BlUserStep%` | Step/quantization. |
| 6 | `BlUserOrient%` | 0 horizontal, 1 vertical. |
| 7 | `BlUserStyle%` | Visual variant. |
| 8 | `BlUserFace%` | Face/background color. |
| 9 | `BlUserAccent%` | Accent/thumb color. |
| 10 | `BlUserDrag%` | Drag state: 0 idle, 1 dragging. |
| 11 | `BlUserDecimals%` | Digits to display, or 0 for no numeric label. |
| 12 | `BlUserReserved%` | Future. |
| 13 | `BlUserPad%` | Label text padding. |
| 14 | `BlUserAlign%` | Horizontal align: 0 left, 1 center, 2 right. |
| 15 | `BlUserVAlign%` | Vertical align: 0 top, 1 middle, 2 bottom. |
| 16 | `BlUserWrap%` | 0 no wrap in v0.1; 1 word wrap in v0.2. |
| 17 | `BlUserBorder%` | Border: 0 none, 1 single. |
| 18 | `BlUserTransparent%` | Background: 0 opaque, 1 transparent (default 1). |
| 19 | `BlUserTextColor%` | Text color; -1 = theme default. |
| 20 | `BlUserTextScale%` | Text scale 1–8 (default 1). |
| 21 | `BlUserAutoSize%` | 1 = width/height may be recomputed from text; default 0. |
| 22 | `BlUserLines%` | Cached rendered line count (read-only). |

Click target (`BlClick%(handle, field)`):

| Field | Name | Meaning |
|---:|---|---|
| 1 | `BlClickButtonMask%` | 1 left, 2 right, 4 middle; combinations allowed. |
| 2 | `BlClickHot%` | Cached hover state. |
| 3 | `BlClickPressed%` | Cached pressed state. |
| 4 | `BlClickWheel%` | Wheel event opt-in. |

Representative declarations:

```
DIM BlCtl%(BlMaxControls%, 17)
DIM BlPanel%(BlMaxControls%, 8)
DIM BlUser#(BlMaxControls%, 22)
DIM BlClick%(BlMaxControls%, 4)
DIM BlMethod%(BlMaxClasses%, BlMaxVerbs%)
DIM BlUserMethod%(BlMaxKinds%, BlMaxVerbs%)
DIM BlPanelLayoutLine%(3)
DIM BlActionLine%(255)
```

Color fields (`BlPanelBg%`, `BlPanelBorder%`, `BlUserFace%`, `BlUserAccent%`, `BlUserTextColor%`) default to -1, meaning “use the class/theme default”. Zero is a valid palette index.

The common row is allocated for every handle. Class-specific rows are allocated for every handle too, but only the matching class reads them. This trades a few dozen numeric cells per handle for direct indexing and no per-class handle arithmetic.

### 8.4 Handles and the free list

```
BlNextHandle%     next never-used handle
BlFreeHead%       first free handle, or 0
```

- `BlRoot%` is 1 and is allocated by `BlInit`.
- `BlAllocHandle%` returns a free handle, or 0 if the table is full.
- `BlFreeHandle%` clears the row, increments nothing, and pushes the handle onto the free list.
- Reuse is allowed. The library assumes no stale handle outlives a destroy; there is no generation counter in v0.1. Event handlers, capture fields, hover/press state, and pending event references are cleared on destroy.
- `BlDestroy` recursively destroys children, front-to-back, before freeing the parent. It is iterative with a `STACK` work stack.
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
BlMethod%(1..BlMaxClasses%, 1..BlMaxVerbs%)       class × verb → line
BlUserMethod%(1..BlMaxKinds%, 1..BlMaxVerbs%)     user kind × verb → line
BlPanelLayoutLine%(0..3)                          panel mode → layout branch
BlActionLine%(0..255)                             host action ID → line
```

`BlInit` fills every method slot with the class-local no-op line, installs the built-in rows, and clears `BlActionLine%`. The no-op line closes the current frame, returns a default result, and returns.

Adding a built-in class or verb is a table write. A v0.2 `BlRegisterClass` can install host-provided method lines at runtime; class-specific storage is still limited to the common table plus the generic host-data columns.

### 9.2 Dispatcher shape

```
32000 REM BlDispatch: dispatch the frame already pushed on STACK
32010 IF BlFp% >= BlMaxFrames% THEN BlStatus% = 6 : GOTO 32090
32020 STACK TOP BlArgcS%, BlArgcN%, BlDispVerb%, BlDispCtl%
32030 IF BlDispCtl% < 1 OR BlDispCtl% > BlMaxControls% THEN BlStatus% = 3 : GOTO 32090
32040 BlDispClass% = BlCtl%(BlDispCtl%, BlFClass%)
32050 IF BlDispClass% < 1 OR BlDispClass% > BlMaxClasses% THEN BlStatus% = 4 : GOTO 32090
32060 IF BlDispVerb% < 1 OR BlDispVerb% > BlMaxVerbs% THEN BlStatus% = 5 : GOTO 32090
32070 BlMethodLine% = BlMethod%(BlDispClass%, BlDispVerb%)
32080 IF BlMethodLine% = 0 THEN BlStatus% = 5 : GOTO 32090
32082 BlFp% = BlFp% + 1
32084 GOSUB BlFrameLoad          ' cache the 14-slot frame at BlFp%
32086 GOSUB BlMethodLine%        ' method closes the frame and pushes the result
32088 BlFp% = BlFp% - 1 : RETURN
32090 GOSUB BlFrameDrop          ' invalid/overflow: consume frame, push default result
32092 RETURN
```

The exact line layout will be finalized during implementation. The important contract is: the dispatcher peeks metadata without removing the frame, validates, calls the method with `GOSUB`, and the method closes the frame.

### 9.3 Computed GOTO tail dispatch

A method that has already been called by `GOSUB` can use computed `GOTO` to jump to a branch and let that branch’s `RETURN` go straight back to the dispatcher. This avoids an extra `GOSUB`/`RETURN` for every kind or mode:

```
38000 REM User class router (called by BlDispatch)
38010 BlUk% = BlUser#(BlArgCtl%(BlFp%), BlUserKind%)
38020 GOTO BlUserMethod%(BlUk%, BlArgVerb%(BlFp%))
```

Panel layout uses the same pattern:

```
37000 REM Panel Layout
37010 GOTO BlPanelLayoutLine%(BlPanel%(BlArgCtl%(BlFp%), BlPanelMode%))
```

Each target method ends with the shared `BlFrameClose`/`RETURN` sequence. The frame and `BlFp%` stay valid across the `GOTO`.

**Rule:** do not use computed `GOTO` from inside a `SELECT CASE`; newbasic raises `GOTO would leave a SELECT CASE`. Use computed `GOSUB` or structure the branch outside the SELECT.

### 9.4 What a method receives

A method receives a frame on `STACK` and a depth `BlFp%`. The dispatcher loads the frame into the per-depth cache before calling:

```
BlArgCtl%(BlFp%)        control handle
BlArgVerb%(BlFp%)       verb ID
BlArgcN%(BlFp%)         numeric argument count
BlArgcS%(BlFp%)         string argument count
BlArgN#(BlFp%, index)   numeric arguments, 1-based
BlArgS$(BlFp%, index)   string arguments, 1-based
BlLocalN#(BlFp%, slot)  mutable local numeric slots
BlLocalS$(BlFp%, slot)  mutable local string slots
```

A method must not decrement `BlFp%` or manipulate another frame. It may call another control through `BlSend`; that pushes a new frame, dispatches it, and restores `BlFp%` on return. State that must survive that nested call lives in `BlLocal...`, not in shared scratch scalars.

### 9.5 Action-line dispatch

`BlSetActionLine(action%, line%)` stores a host line number in `BlActionLine%(action%)`. The host loop can then use computed `GOSUB`:

```
GOSUB 41600                    ' BlNextEvent
IF BlEventAction% <> 0 AND BlActionLine%(BlEventAction%) <> 0 THEN GOSUB BlActionLine%(BlEventAction%)
```

The library does not call host code itself; the host performs the computed `GOSUB`. A zero line means “no handler” and must be checked. Action IDs are 0–255; 0 remains “no action” by convention.

### 9.6 Dynamic class registration (v0.2)

`BlRegisterClass(class%, paintLine%, updateLine%, ...)` fills a free row of `BlMethod%`. A host class can use the common control columns and a small generic host-data area. It cannot add new class-specific arrays, so a host class is a paint/input callback over common state. This is a v0.2 feature; v0.1 ships built-in classes only.

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
STACK PUSH BlCallN#(1), ..., BlCallN#(8), BlCallS$(1), BlCallS$(2), BlCallCtl%, BlCallVerb%, BlCallArgcN%, BlCallArgcS%
```

The top is `ArgcS`. `STACK TOP` reads top-first, so the metadata read order is `ArgcS, ArgcN, Verb, Ctl, S2, S1, N8, ..., N1`.

The result pair is pushed by the method as:

```
STACK PUSH BlRetS$, BlRetN#
```

so the numeric result is top. The caller pops `BlRetN#, BlRetS$`.

### 10.3 Depth and per-depth state

```
BlFp%                    active baslib frame index, 0 = none
BlMaxFrames%             32
```

`BlFp%` counts baslib frames only. Host values below them are invisible and must never be popped. The dispatcher increments `BlFp%` after validation and decrements it after the method returns. A depth overflow drops the new frame, returns status 6, and leaves the previous frame current.

Per-depth arrays:

```
BlArgCtl%(1..BlMaxFrames%)
BlArgVerb%(1..BlMaxFrames%)
BlArgcN%(1..BlMaxFrames%)
BlArgcS%(1..BlMaxFrames%)
BlArgN#(1..BlMaxFrames%, 1..BlMaxArgs%)
BlArgS$(1..BlMaxFrames%, 1..BlMaxStrArgs%)
BlLocalN#(1..BlMaxFrames%, 1..BlMaxArgs%)
BlLocalS$(1..BlMaxFrames%, 1..BlMaxStrArgs%)
```

Representative declarations:

```
DIM BlArgN#(BlMaxFrames%, BlMaxArgs%)
DIM BlArgS$(BlMaxFrames%, BlMaxStrArgs%)
DIM BlLocalN#(BlMaxFrames%, BlMaxArgs%)
DIM BlLocalS$(BlMaxFrames%, BlMaxStrArgs%)
DIM BlArgCtl%(BlMaxFrames%)
DIM BlArgVerb%(BlMaxFrames%)
DIM BlArgcN%(BlMaxFrames%)
DIM BlArgcS%(BlMaxFrames%)
```

These are a cache, not the canonical frame. The frame remains on `STACK` until the method closes it.

### 10.4 Push, load, close

**Push** (`BlFramePush`):

1. Validate `BlCallCtl%`, `BlCallVerb%`, `BlCallArgcN%`, `BlCallArgcS%`.
2. Fill unused `BlCallN#`/`BlCallS$` slots with 0/empty.
3. `STACK PUSH` all 14 slots.
4. Return status. If validation fails, no frame is pushed and the caller must not call `BlDispatch`; if it succeeds, the caller must call `BlDispatch` exactly once.

**Load** (`BlFrameLoad`, called by the dispatcher after incrementing `BlFp%`):

```
STACK TOP BlArgcS%(fp), BlArgcN%(fp), BlArgVerb%(fp), BlArgCtl%(fp), ...
          BlArgS$(fp,2), BlArgS$(fp,1), BlArgN#(fp,8), ..., BlArgN#(fp,1)
```

This leaves the frame intact. Methods read the cache. If a method needs a fresh copy after a nested call, the cache is still valid because each depth has its own row.

**Close** (`BlFrameClose`, called at the end of every method):

1. Pop the 14 slots into typed scratch variables in top-first order.
2. `STACK PUSH BlRetS$, BlRetN#`.
3. Return.

After close, the only new value above the caller’s entry depth is the result pair.

**Drop** (`BlFrameDrop`, invalid class/verb/depth or no-op):

1. Pop the 14 slots into typed scratch variables.
2. `STACK PUSH "", 0` (default result).
3. Return.

`BlFrameClose` and `BlFrameDrop` must be used symmetrically. A method must never return with its frame still on the stack.

### 10.5 Caller shape

Call setup globals:

```
BlCallCtl%         target control
BlCallVerb%        verb ID
BlCallArgcN%       numeric argument count
BlCallArgcS%       string argument count
BlCallN#(1..8)     numeric arguments
BlCallS$(1..2)     string arguments
BlRetN#            numeric result
BlRetS$            string result
```

`BlCallN#`/`BlCallS$` are dimensioned once at `BlMaxArgs%`/`BlMaxStrArgs%`; unused slots are zeroed by `BlFramePush`.

Internal callers use:

```
BlCallCtl% = target%
BlCallVerb% = verb%
BlCallArgcN% = n%
BlCallArgcS% = s%
BlCallN#(1) = ...
BlCallS$(1) = ...
GOSUB BlFramePush
GOSUB BlDispatch
STACK POP BlRetN#, BlRetS$
```

`BlSend` (a convenience helper) performs push, dispatch, and result pop for zero- or few-argument calls. If `BlFramePush` rejects the request, `BlSend` returns the status without calling `BlDispatch` and without popping a result. Argument-heavy callers set the `BlCall...` block directly.

The result pair is always present when a frame was pushed, even for a void verb; it is simply ignored.

### 10.6 Nested calls

A method’s frame remains on `STACK` while it runs. A nested `BlSend` pushes a child frame above it, dispatches, and pops the child result. When the child returns, the parent frame is on top again.

- Parent arguments are safe in `BlArg...` at the parent depth.
- Parent mutable locals are safe in `BlLocal...` at the parent depth.
- Shared scratch scalars (`BlTmp...`) are clobbered by nested calls; do not use them across a `BlSend`.
- `BlFp%` is restored after the child returns.

### 10.7 Invariants and host coexistence

- Every dispatch consumes exactly one 14-slot frame and leaves exactly one 2-slot result.
- `BlFp%` is balanced after every API call.
- baslib never assumes `STACK` is empty. A host may push its own values before calling baslib; the frame and result sit above them.
- baslib never pops below its own frames. If a method or API returns an error, it still closes its frame.
- Expected failures use `BlStatus%`, not BASIC runtime errors, so `ON ERROR` is not used as control flow.
- An unexpected runtime error inside a method can leave a frame. The host should `CLEAR` and reinitialize; baslib does not attempt to repair an arbitrary stack.
- `STACK` is cleared by `CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN`; `BlInit` is required afterward.
- `GOSUB`/`RETURN` do not touch `STACK`; nested calls are the source of depth, not the GOSUB stack.

### 10.8 Work stack

Layout, paint, and destroy use `STACK` as an iterative work stack:

```
BlWorkCount%     number of live work items
```

- Push a work item as `[handle, clipL, clipT, clipR, clipB]` (or `[handle]` for destroy).
- `BlWorkCount%` counts live items; `STACK` has no depth query.
- Pop one item, process, push children in the required order.
- Class methods use their own frames above the work items; a method must close its frame before returning, so the next work item is on top again.
- The work count is decremented on every pop and asserted to reach 0.

For paint, children are pushed in reverse sibling order so the pop order is back-to-front. For destroy, children are pushed front-to-back and popped front-to-back.

This removes the old fixed `BlMaxWork%` array and makes a 255-control tree safe without a separate limit.

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
- The root’s local rectangle is `(0, 0, BlRootW%, BlRootH%)`.
- The root size defaults to `SCREENW`/`SCREENH`; `BlInit` reads `BlReqW%`/`BlReqH%` and falls back to the canvas.
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

Panel `BlPanelMode%`:

| Value | Mode | Behavior |
|---:|---|---|
| 0 | Absolute | Child X/Y are used as-is relative to the panel’s outer top-left; padding is ignored. |
| 1 | Vertical | Children stack from `(Pad, Pad)` top-to-bottom with `Gap`; width is `clientW`. |
| 2 | Horizontal | Children flow from `(Pad, Pad)` left-to-right with `Gap`; height is `clientH`. |
| 3 | Grid | Equal-width columns; reserved for a later revision. |

v0.1 implements 0, 1, and 2. Grid is parsed and reserved, not promised.

Panels do not dispatch their children’s Layout methods. The core does that in its own pre-order walk. This keeps a parent’s layout loop free of nested dispatches and shared-loop clobbering.

### 11.4 Layout pass

`BlLayoutAll`:

1. Start with the root and the root clip.
2. For each container popped from the `STACK` work stack:
   - Dispatch `Layout` so the class sets its children’s local rectangles.
   - For each child, compute absolute position:
     ```
     child.AbsX = parent.AbsX + child.X
     child.AbsY = parent.AbsY + child.Y
     ```
   - Compute the child clip as the intersection of the parent clip and the child’s absolute rectangle when `BlFlagClip%` is set; otherwise inherit the parent clip.
   - Push the child for its own layout.
3. Clear `BlFlagLayoutDirty%` and the global layout-dirty flag.

The pass is iterative. A deep tree cannot overflow the interpreter’s GOSUB stack, and the `STACK` work items are independent of call frames.

### 11.5 When layout runs

- `BlCreate`, `BlSetBounds`, `BlShow`, `BlHide`, and `BlAutoSize` mark the tree layout-dirty.
- `BlLayoutAll` is explicit.
- `BlPaintAll` calls `BlLayoutAll` automatically if the layout-dirty flag is set.
- `BlHitTest` assumes layout has run; `BlPaintAll` is the normal way to guarantee it. `BlPollInput` documents this precondition.

### 11.6 Clipping

- Root clip: `(0, 0, rootW-1, rootH-1)`.
- Child clip: intersection as above. Empty intersections are not painted or hit-tested.
- `BlFlagClip%` is set by default on panels and the root. A leaf usually does not need it, but inherits the parent’s clip.
- Clipping is cached per handle for fast hit testing and for `BlUpdate`.

---

## 12. Painting

### 12.1 Paint traversal

`BlPaintAll`:

1. `BlLayoutAll` if needed.
2. Clear all `BlCtlZ%` to 0.
3. Push the root with the root clip onto the `STACK` work stack.
4. While `BlWorkCount% > 0`:
   - Pop a control and its clip.
   - Skip hidden controls and empty clips.
   - Set the current paint control and clip.
   - `PUSH VIEW`
   - `VIEW (clipL, clipT)-(clipR, clipB)`
   - Dispatch `Paint`.
   - `POP VIEW`
   - Assign `BlCtlZ%` in paint order.
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
BlPaintOX% = AbsX - ClipL
BlPaintOY% = AbsY - ClipT
```

A class draws local `(x, y)` at `(BlPaintOX% + x, BlPaintOY% + y)`. For an unclipped control, both offsets are 0. Shared drawing helpers add the offsets; class code should not use raw absolute coordinates.

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

Text helpers wrap `DRAWTEXT` and work in control-local coordinates, adding `BlPaintOX%`/`BlPaintOY%` for the active viewport. Controls fill their own background first when they want an opaque surface, then draw text over it.

### 12.5 Label text

Labels support:

- explicit multiline text via CR/LF;
- horizontal alignment (left/center/right) using `TEXTWIDTH` of the widest line;
- vertical alignment (top/middle/bottom) using `TEXTHEIGHT`;
- pixel truncation to the available width;
- optional border and padding;
- optional transparent background;
- text scale 1–8;
- auto-size via `BlAutoSize` or `BlUserAutoSize%`.

`BlTextRect` computes metrics with `TEXTWIDTH`/`TEXTHEIGHT`, splits lines at CR/LF, truncates each line to the available pixel width, and draws each line with `DRAWTEXT`. `VIEW` clips any overflow.

Word wrap (`BlUserWrap% = 1`) is v0.2. It uses `TEXTWIDTH` to fit words and `TEXTHEIGHT` for the resulting height. v0.1 accepts `BlUserWrap% = 0` only.

`BlAutoSize(handle%)` sets width/height from `TEXTWIDTH`/`TEXTHEIGHT` plus padding and border, marks layout dirty and `BlNeedsPaint%`, and is valid for labels and buttons. It does not paint.

### 12.6 Paint vs update

- `Paint` draws the whole control. It is called by `BlPaintAll`.
- `Update` redraws only the changed part. It is called by `BlUpdate(handle%)` under a `PUSH VIEW` / `VIEW clip` / `POP VIEW` wrapper and uses the control’s cached clip. The class decides what “changed” means (for example, a slider redraws its thumb and the vacated old thumb area).
- `BlInvalidate(handle%)` sets `BlFlagDirty%` and `BlNeedsPaint%`. `BlPaintAll` clears both; `BlUpdate` clears the target’s dirty flag and recomputes `BlNeedsPaint%` from the remaining controls.
- `BlNeedsLayout%` is set by creation, bounds changes, show/hide, and auto-size; `BlLayoutAll` clears it.
- v0.1 does not implement a dirty-rectangle compositor. `BlUpdate` is a per-control partial redraw, not a general damage-region system. A host that changes many controls should call `BlPaintAll`.
- `BlUpdate` is only for content changes within stable bounds. After a move, resize, show, hide, or auto-size, call `BlPaintAll` so the vacated area is repainted.
- `BlSetText`, `BlSetValue`, `BlSetEnabled`, hover/press transitions, and wheel changes mark the control dirty and set `BlNeedsPaint%`, but do not paint. The host calls `BlUpdate` for that control or `BlPaintAll` for the frame.

### 12.7 Theme

The library never calls `PALETTE` or `SCREEN`. It uses palette indices and exposes theme globals:

```
BlThemeDesktop%
BlThemePanel%
BlThemeFace%
BlThemeLight%
BlThemeDark%
BlThemeText%
BlThemeAccent%
BlThemeDisabled%
```

`BlInit` sets defaults appropriate to the current palette. The host may override them after `BlInit` and repaint. `BlCreate` can set per-control colors through `BlReqFace%`, `BlReqAccent%`, `BlReqBg%`, and `BlReqBorder%`. The value -1 means “use the theme default”; 0 is a real palette index.

### 12.8 No double buffering in v0.1

A `GET`/`PUT` screen-image buffer would allow flicker-free full repaints, but it costs a large integer array and complicates clipping. v0.1 draws directly. `BlUpdate` avoids full repaints in interactive paths. Double buffering can be evaluated after the control model is stable.

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

`BlHitTest(x%, y%)`:

1. Ensure layout is current (`BlPaintAll` normally did this).
2. Loop handles 1..`BlMaxControls%`.
3. Reject free handles, hidden controls, disabled controls, zero paint order, and points outside the cached clip or absolute rectangle.
4. Optionally dispatch `HitTest` for a shape check (dial circle, for example). The default is “inside rectangle”.
5. Track the candidate with the highest `BlCtlZ%`.
6. Return that handle, or 0.

The loop is O(255) per input event, which is negligible and avoids maintaining a spatial index. `BlCtlZ%` is assigned by the last paint, so hit testing is defined against what the user actually sees.

Visibility and enabled state are inherited: a hidden or disabled ancestor excludes its whole subtree from painting and hit testing. The paint/layout walks skip such subtrees; the hit test checks the parent chain for a disabled ancestor. A disabled control that should swallow clicks is a future flag.

### 13.3 Click dispatch

`BlClickAt(x%, y%, button%)`:

1. `target% = BlHitTest(x%, y%)`.
2. While `target% <> 0`:
   - Push a frame with `Click`, local coordinates, and the button.
   - Dispatch.
   - If the class returns non-zero (handled), post a click event and stop.
   - Otherwise move to the parent and repeat (bubbling).
3. If no control handles the click, no click event is posted.

Bubbling lets a panel or the root handle clicks on empty space without every leaf needing a handler. A button or click target returns 1 when it handles the click; future slider/dial handlers do the same.

Click button codes are `1` left, `2` right, and `3` middle, matching `MOUSEB`. The click-target mask uses bits `1`, `2`, and `4`; the click-target method maps code 3 to bit 4 before testing.

### 13.4 Live input state machine

`BlPollInput` reads live state once per host frame:

```
x = MOUSEPOSX : y = MOUSEPOSY
buttons = MOUSEBUTTONS
wheel = MOUSEWHEEL
```

It maintains:

```
BlPrevX%, BlPrevY%, BlPrevButtons%
BlHover%          current live hit control, or 0
BlCapture%        control receiving drag/move while a button is held
BlPressed%        control that received the last down
BlMouseIn%        pointer is over the canvas
```

Processing order:

1. **Hover**: if the hit control changed, post `MouseLeave` to the old, `MouseEnter` to the new, update `BlFlagHot%`, and mark both dirty.
2. **Buttons**: compare `BlPrevButtons%` with `buttons`. For each newly pressed bit, post `MouseDown` to the hit control and set `BlCapture%`/`BlPressed%`; for each released bit, post `MouseUp` to the capture/pressed control and release capture.
3. **Move**: if the pointer moved, dispatch `MouseMove` to `BlCapture%` when captured, otherwise to the hit control. Post a move event only when the target or position changed; coalesce repeats.
4. **Clicks**: while the queue is not empty and a bound is not reached, call `MOUSEB`; on a non-zero button, read `MOUSEX`/`MOUSEY` and call `BlClickAt`. This is the action path.
5. **Wheel**: if `wheel <> 0`, dispatch `Wheel` to `BlCapture%` or the hit control, post a wheel event, and mark dirty.
6. **Pointer outside**: when the live position is -1, clear hover/pressed visuals but keep capture until the button is released.
7. Save previous position/buttons.

Any hot/pressed change sets `BlNeedsPaint%`; any geometry change sets `BlNeedsLayout%` and `BlNeedsPaint%`. The exact ordering of Move/Down/Up versus Click is documented in the event queue order. A click is posted by `BlClickAt`; down/up are visual/drag events and do not duplicate the action.

### 13.5 Drag and capture

Capture is global:

```
BlCapture%     control receiving move/up while a button is held
```

- On a press over a hit control, set capture and dispatch `MouseDown`.
- While captured, `MouseMove` goes to the capture control, not the hit control.
- On release, dispatch `MouseUp`, then clear capture.
- `BlFlagCaptured%` mirrors `BlCapture%`.
- Destroying or hiding the capture control clears capture and dispatches `CaptureLost`.
- Slider and dial use capture in v0.2: MouseDown sets the value, MouseMove drags, MouseUp finishes.

### 13.6 Event queue

Internal ring (private to the library):

```
BlEvType%(1..BlMaxEvents%)
BlEvCtl%(1..BlMaxEvents%)
BlEvAction%(1..BlMaxEvents%)
BlEvX%(1..BlMaxEvents%)
BlEvY%(1..BlMaxEvents%)
BlEvButton%(1..BlMaxEvents%)
BlEvWheel%(1..BlMaxEvents%)
BlEvHead%, BlEvTail%, BlEvCount%
```

Public values filled by `BlNextEvent`:

```
BlEventType%       event type (0 when the queue is empty)
BlEventCtl%        control that generated the event
BlEventAction%     action ID copied from the control
BlEventX%, BlEventY%
BlEventButton%
BlEventWheel%
BlEventCount%      number of events still queued
```

Event types:

| Value | Name | Meaning |
|---:|---|---|
| 1 | `BlEvClick%` | A control handled a click. |
| 2 | `BlEvChange%` | Value changed (v0.2 continuous controls). |
| 3 | `BlEvMove%` | Live pointer moved. |
| 4 | `BlEvEnter%` | Pointer entered a control. |
| 5 | `BlEvLeave%` | Pointer left a control. |
| 6 | `BlEvDown%` | Button pressed. |
| 7 | `BlEvUp%` | Button released. |
| 8 | `BlEvWheel%` | Wheel moved. |
| 9 | `BlEvKey%` | Reserved. |
| 10 | `BlEvFocus%` | Reserved. |
| 11 | `BlEvCaptureLost%` | Capture was released by hide/destroy. |

Move events are coalesced per poll; the queue is bounded at 64. If the queue fills, the newest non-click event is dropped and `BlStatus%`/`BlError%` indicate a full queue; clicks take priority. `BlPollInput` drains at most eight queued clicks per call.

### 13.7 Action IDs and action lines

`BlFAction%` is a host-assigned integer. On a handled click, it is copied to the event. The host can dispatch with a static `ON BlEventAction% GOSUB ...` or with a line table:

```
BlActionLine%(action%) = handler line
IF BlActionLine%(BlEventAction%) <> 0 THEN GOSUB BlActionLine%(BlEventAction%)
```

`BlSetActionLine(action%, line%)` fills the table. Action 0 is valid but should be reserved for “no action”; a zero line means no handler.

This is the BASIC substitute for a callback function pointer. The library never calls host code directly; it posts an event and returns. The host may call the line explicitly, which keeps the host in control of its own stack.

### 13.8 Test hooks

Synthetic input must not depend on window events:

- `BlTestClickAt(x%, y%, button%)` performs hit test and click dispatch without the queue.
- `BlTestMouseMove(x%, y%)` sets the live position and runs hover/move processing.
- `BlTestMouseButton(button%, down%, x%, y%)` sets/clears a live button bit and runs down/up processing; button is 1 left, 2 right, 3 middle.
- `BlTestWheel(delta%)` injects wheel movement.

These call the same internal routines as `BlPollInput`, so tests cover the real state machine.

### 13.9 Keyboard

Out of scope for v0.1. The `Key` and `Focus` verb slots and `BlFlagFocusable%` are reserved. The host can poll `INKEY$` itself. Tab order will follow child order when implemented.

---

## 14. Public API

All calls are `GOSUB <literal line>`. Set request globals, call, then check `BlStatus%` and read `BlRet...`. Internal methods use `STACK` frames; the host API does not.

### 14.1 Common request/return globals

```
BlStatus%         0 success, non-zero error
BlError%          last error code
BlError$          last error message
BlReqClass%       class ID
BlReqParent%      parent handle
BlReqHandle%      target handle
BlReqX%, BlReqY%  local or absolute coordinates, depending on call
BlReqW%, BlReqH%  width/height
BlReqFlags%       initial flags
BlReqAction%      host action ID
BlReqTag%         host numeric tag
BlReqKind%        user-control kind
BlReqFace%        user-control face/background color; -1 = theme default
BlReqAccent%      user-control accent color; -1 = theme default
BlReqTextColor%   label/button text color; -1 = theme default
BlReqTextScale%   text scale 1-8
BlReqTransparent% label background: 0 opaque, 1 transparent
BlReqAlign%       text align: 0 left, 1 center, 2 right
BlReqVAlign%      text vertical align: 0 top, 1 middle, 2 bottom
BlReqBorderStyle% text border: 0 none, 1 single
BlReqWrap%        0 no wrap, 1 word wrap (v0.2)
BlReqAutoSize%    1 = auto-size after create/set-text
BlReqBg%          panel background color; -1 = theme default
BlReqBorder%      panel border color; -1 = theme default
BlReqMode%        panel layout mode
BlReqPad%         panel or text padding
BlReqGap%         panel gap
BlReqCols%        panel grid columns (reserved)
BlReqButton%      button number for test hooks
BlReqEnabled%     1 = enabled, 0 = disabled
BlReqButtonMask%  click-target mask: 1 left, 2 right, 4 middle
BlReqText$        caption/name/label text
BlReqValue#       slider/gauge/dial value
BlReqLine%        callback line for BlSetActionLine
BlReqActionId%    action ID for BlSetActionLine
BlRoot%           handle of the top-level control
BlRetHandle%      returned handle
BlRetValue#       returned value
BlRetN%           generic numeric result
BlEventType%      event type, 0 when empty
BlEventCtl%       control that generated the event
BlEventAction%    action ID copied from the control
BlEventX%, BlEventY%
BlEventButton%    button that caused the event
BlEventWheel%     wheel delta
BlEventCount%     queued event count
BlNeedsPaint%     1 if a control or input state marked the tree dirty
BlNeedsLayout%    1 if layout is stale
BlActionLine%(0..255)  host action ID → handler line
```

`BlCreate` uses the request block and returns a handle in `BlRetHandle%`.

`BlCreate` defaults:

- flags = `BlReqFlags%` OR visible + enabled + clip;
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

`BlInit` reads `BlReqW%`/`BlReqH%` first, then initializes the rest of the request block to safe defaults. If `BlReqW%`/`BlReqH%` are 0, it uses `SCREENW`/`SCREENH`. If the canvas is unavailable (text mode or zero size), it returns error 14. `BlInit` sets `BlNeedsPaint% = 1` and `BlNeedsLayout% = 1`.

### 14.2 Entry points

| Line | Routine | Inputs | Outputs |
|---:|---|---|---|
| 40000 | `BlInit` | `BlReqW%`, `BlReqH%` (0 → canvas) | `BlRoot%`, `BlStatus%` |
| 40100 | `BlCreate` | class, parent, local x/y/w/h, flags, action, tag, kind, button mask, text, value, mode, pad, gap, text properties | `BlRetHandle%` |
| 40200 | `BlDestroy` | `BlReqHandle%` | `BlStatus%` |
| 40300 | `BlSetBounds` | handle, local x/y/w/h | `BlStatus%` |
| 40400 | `BlShow` | handle | `BlStatus%` |
| 40500 | `BlHide` | handle | `BlStatus%` |
| 40600 | `BlSetText` | handle, `BlReqText$` | `BlStatus%` |
| 40700 | `BlSetValue` | handle, `BlReqValue#` | `BlStatus%` |
| 40800 | `BlGetValue` | handle | `BlRetValue#` |
| 40900 | `BlSetLayout` | handle, mode, pad, gap, cols | `BlStatus%` |
| 41000 | `BlSetAction` | handle, action | `BlStatus%` |
| 41100 | `BlInvalidate` | handle | `BlStatus%` |
| 41200 | `BlLayoutAll` | — | `BlStatus%` |
| 41300 | `BlPaintAll` | — | `BlStatus%` |
| 41400 | `BlUpdate` | handle | `BlStatus%` |
| 41500 | `BlPollInput` | — | count in `BlRetN%` |
| 41550 | `BlPollMouse` | — | alias for `BlPollInput` |
| 41600 | `BlNextEvent` | — | `BlEvent...`, `BlEventCount%` |
| 41700 | `BlFindAt` | absolute `BlReqX%`, `BlReqY%` | `BlRetHandle%` |
| 41800 | `BlTestClickAt` | absolute `BlReqX%`, `BlReqY%`, `BlReqButton%` | `BlRetN%` |
| 41810 | `BlTestMouseMove` | absolute x/y | `BlRetN%` |
| 41820 | `BlTestMouseButton` | button, down, x/y | `BlRetN%` |
| 41830 | `BlTestWheel` | delta | `BlRetN%` |
| 41900 | `BlDump` | optional handle | debug print |
| 41910 | `BlAutoSize` | handle | `BlStatus%` |
| 41920 | `BlSetActionLine` | action ID, line | `BlStatus%` |
| 41930 | `BlRegisterClass` | class ID, method lines (v0.2) | `BlStatus%` |
| 41950 | `BlSetEnabled` | handle, `BlReqEnabled%` | `BlStatus%` |

Line numbers are the API contract. New routines can be added at unused lines; existing numbers do not move within a major version. `BlSend`, `BlFramePush`, `BlFrameLoad`, `BlFrameClose`, and `BlFrameDrop` are internal lines, not public API.

### 14.3 Initialization sequence

```
SCREEN 720                    ' any graphics mode
BlReqW% = 0 : BlReqH% = 0     ' 0 means use SCREENW/SCREENH
GOSUB 40000                   ' BlInit
IF BlStatus% <> 0 THEN ...    ' handle failure

... create panels and controls with GOSUB 40100 ...

GOSUB 41300                   ' BlPaintAll (does layout if dirty)

loop:
  GOSUB 41500                 ' BlPollInput
  WHILE BlEventCount% > 0
    GOSUB 41600               ' BlNextEvent
    IF BlEventAction% <> 0 AND BlActionLine%(BlEventAction%) <> 0 THEN GOSUB BlActionLine%(BlEventAction%)
  WEND
  IF BlNeedsPaint% THEN GOSUB 41300
  K$ = INKEY$ : IF K$ = CHR$(27) THEN END
  SLEEP 16                    ' pace the loop
  GOTO loop
```

`BlPaintAll` must precede the first poll so `BlCtlZ%` is valid. `BlPollInput` can mark controls dirty; the host decides whether to call `BlUpdate` or `BlPaintAll`.

### 14.4 Host skeleton

The following is illustrative; line numbers below 30000 are the host’s.

```basic
10  REM baslib demo host
20  SCREEN 720
30  BlReqW% = 0 : BlReqH% = 0
40  GOSUB 40000
50  IF BlStatus% <> 0 THEN PRINT "init failed"; BlStatus%: END
60  REM --- panel ---
80  BlReqClass% = 2 : BlReqParent% = BlRoot%
90  BlReqX% = 40 : BlReqY% = 40 : BlReqW% = 520 : BlReqH% = 360
100 BlReqMode% = 1 : BlReqPad% = 12 : BlReqGap% = 8
110 GOSUB 40100 : BlPanel% = BlRetHandle%
120 REM --- button ---
130 BlReqClass% = 3 : BlReqParent% = BlPanel%
140 BlReqKind% = 1 : BlReqX% = 0 : BlReqY% = 0 : BlReqW% = 220 : BlReqH% = 32
150 BlReqText$ = "Ready" : BlReqAction% = 100
160 GOSUB 40100 : BlButton% = BlRetHandle%
170 REM --- label (multiline) ---
180 BlReqClass% = 3 : BlReqParent% = BlPanel%
190 BlReqKind% = 5 : BlReqX% = 0 : BlReqY% = 40 : BlReqW% = 300 : BlReqH% = 40
200 BlReqText$ = "Ready" + CHR$(13) + "second line"
205 BlReqAlign% = 0 : BlReqVAlign% = 1 : BlReqTextScale% = 1
210 BlReqTransparent% = 1 : BlReqTextColor% = -1 : BlReqFace% = -1
215 BlReqPad% = 4 : BlReqBorderStyle% = 0
220 GOSUB 40100 : BlLabel% = BlRetHandle%
230 REM --- click target (invisible hotspot) ---
240 BlReqClass% = 4 : BlReqParent% = BlPanel%
250 BlReqX% = 0 : BlReqY% = 90 : BlReqW% = 300 : BlReqH% = 24
260 BlReqText$ = "" : BlReqAction% = 200
270 GOSUB 40100 : BlClickTarget% = BlRetHandle%
280 GOSUB 41300
290 REM --- action lines ---
300 BlReqActionId% = 100 : BlReqLine% = 1000 : GOSUB 41920
310 BlReqActionId% = 200 : BlReqLine% = 2000 : GOSUB 41920
320 REM --- main loop ---
330 GOSUB 41500
340 WHILE BlEventCount% > 0
350   GOSUB 41600
360   IF BlEventAction% <> 0 AND BlActionLine%(BlEventAction%) <> 0 THEN GOSUB BlActionLine%(BlEventAction%)
370 WEND
380 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
390 SLEEP 16
400 GOTO 330
1000 REM button action
1010 BlReqText$ = "Button clicked" : BlReqHandle% = BlLabel%
1020 GOSUB 40600
1030 BlReqHandle% = BlLabel% : GOSUB 41910 : GOSUB 41400
1040 RETURN
2000 REM click target action
2010 BlReqText$ = "Target clicked" : BlReqHandle% = BlLabel%
2020 GOSUB 40600
2030 GOSUB 41400
2040 RETURN
```

In a real host, event handling includes `BlEvMove`, `BlEvEnter`, `BlEvLeave`, `BlEvDown`, `BlEvUp`, and `BlEvWheel`; the demo above only shows action dispatch.

---

## 15. Class behavior details

### 15.1 Top-level class (1)

State:

- `BlRoot%` handle.
- `BlHover%`, `BlPressed%`, `BlCapture%`, `BlFocus%` (focus reserved).
- `BlTopMouseX%`, `BlTopMouseY%`, `BlTopButton%` (last click, for convenience).

Methods:

- `Paint`: fill the root bounds with `BlThemeDesktop%`; optionally draw a title/status strip.
- `Layout`: no child positioning; children keep local coordinates. The root’s client area is the full bounds.
- `HitTest`: return 1 for inside the root.
- `Click`: return 0 by default; a host can set a root action if it wants desktop clicks.
- `MouseEnter`/`MouseLeave`: update hover state; mark dirty.
- `MouseDown`/`MouseUp`: record pressed state; root may capture.
- `MouseMove`: update live coordinates; mark dirty if a hover visual exists.
- `Wheel`: return 0 by default; the host can handle it via events.

The core, not the top class, performs the hit-test walk and click bubbling. The top class owns input state and the future focus manager.

### 15.2 Panel class (2)

State: `BlPanel%` fields from §8.3.

Methods:

- `Paint`: fill background, draw a one-pixel border, optionally draw `BlCtlText$` as a title.
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

Horizontal layout mirrors this with `child.H = clientH` and `x` advancing. Hidden children and children with zero width or height are skipped and do not advance, so hiding a control closes the gap. `BlShow` and `BlHide` mark the parent tree layout-dirty.

### 15.3 User-control class (3)

The User class router uses computed `GOTO` tail dispatch:

```
BlUk% = BlUser#(BlArgCtl%(BlFp%), BlUserKind%)
GOTO BlUserMethod%(BlUk%, BlArgVerb%(BlFp%))
```

Kinds:

- **Button (v0.1)**: beveled face, centered `DRAWTEXT` caption at `BlUserTextScale%`, press/disabled state. Left click returns 1; right/middle click returns 0 and bubbles. Hover and pressed flags mark dirty.
- **Label (v0.1)**: multiline static text drawn with `DRAWTEXT`. Background fill is optional (transparent by default), border optional, text positioned with `BlUserPad%`, `BlUserAlign%`, `BlUserVAlign%`, and `BlUserTextScale%`. Text is truncated to available pixel width; auto-size uses `TEXTWIDTH`/`TEXTHEIGHT`. `HitTest` returns 0, so labels are click-through; a label never posts a click.
- **Slider (v0.2)**: horizontal/vertical track, thumb at value position. MouseDown/Move/Up map position to value, quantize to `Step`, clamp, and capture the mouse.
- **Gauge (v0.2)**: read-only bar or arc; no click; Wheel may be reserved.
- **Dial (v0.2)**: circular track and pointer; MouseDown/Move/Up map angle to value, quantize, clamp, and capture.

In v0.1 `BlCreate` accepts user kinds 1 (button) and 5 (label); kinds 2–4 report error 10 until v0.2. The method table and value fields are in place from the start.

`BlSetValue` validates range and marks the control for update; `BlGetValue` returns the current value. Labels have no value; `BlSetValue`/`BlGetValue` on a label return error 10.

### 15.4 Label behavior (User kind 5)

State is stored in the User table:

- `BlUserPad%` — inner padding.
- `BlUserAlign%` — 0 left, 1 center, 2 right.
- `BlUserVAlign%` — 0 top, 1 middle, 2 bottom.
- `BlUserBorder%` — 0 none, 1 single-pixel border.
- `BlUserTextColor%` — text color, or -1 for the theme text color.
- `BlUserTextScale%` — DRAWTEXT scale 1–8.
- `BlUserFace%` — background color when `BlUserTransparent% = 0`; -1 means theme panel.
- `BlUserTransparent%` — 1 = no fill, 0 = opaque.
- `BlUserWrap%` — 0 in v0.1; 1 = word wrap in v0.2.
- `BlUserAutoSize%` — 1 = measure text and adjust bounds.
- `BlUserLines%` — cached rendered line count.

Paint:

1. If `BlUserTransparent% = 0`, fill the control rectangle with `BlUserFace%` (or the theme panel color). Otherwise leave the background untouched.
2. If `BlUserBorder% = 1`, draw a one-pixel border. The text area is inset by `Pad + 1`.
3. Call `BlTextRect` with the control rectangle, text, alignment, vertical alignment, padding, text color, and scale.
4. `BlTextRect` splits at CR/LF, measures with `TEXTWIDTH`/`TEXTHEIGHT`, truncates each line to the available pixel width, and draws each line with `DRAWTEXT`.
5. If disabled, use `BlThemeDisabled%` for the text instead of `BlUserTextColor%`.

Behavior:

- Explicit multiline is supported; `CHR$(13)` and `CHR$(10)` start lines.
- Word wrap is v0.2 and uses `TEXTWIDTH` to fit words.
- `HitTest` always returns 0. A label is click-through and never blocks a button or click target underneath it.
- `Click` is not implemented; a label never posts a click and has no action.
- `Update` redraws the whole label; label paint is cheap.
- `BlCreate` copies `BlReqAutoSize%` into `BlUserAutoSize%` and calls `BlAutoSize` when it is set.
- `BlAutoSize(handle%)` sets width/height from `TEXTWIDTH`/`TEXTHEIGHT` plus padding/border; `BlUserAutoSize% = 1` requests the same after text changes.
- Labels have no value. `BlSetValue`/`BlGetValue` return error 10.

### 15.5 Click-target class (4)

- `Paint`: no-op; the target is invisible.
- `HitTest`: rectangular by default.
- `Click`: checks `BlClickButtonMask%` (1 left, 2 right, 4 middle; combinations allowed), returns 1 if the button matches, else 0.
- `MouseDown`/`MouseUp`: update pressed state for visual overlays.
- `MouseEnter`/`MouseLeave`: update hot state.
- `Wheel`: return 0 or post a wheel event if `BlClickWheel%` is set.
- Useful as an overlay child of a panel or a user control, and as the minimal test class for the event pipeline.

### 15.6 Shared drawing helpers

At 39500–39999:

All coordinates below are local control coordinates.

- `BlBox(x%, y%, w%, h%, color%, fill%)`: clipping-aware rectangle.
- `BlBevel(x%, y%, w%, h%, light%, dark%, face%)`: raised/sunken face.
- `BlText(x%, y%, text$, fg%, scale%)`: one `DRAWTEXT` call at local `(x, y)`, with `BlPaintOX%`/`BlPaintOY%` applied.
- `BlTextRect(x%, y%, w%, h%, text$, align%, valign%, fg%, scale%, pad%)`: multiline pixel text layout using `TEXTWIDTH`/`TEXTHEIGHT`, truncation, and padding.
- `BlTextFit(text$, w%, scale%)`: returns the truncation length for a line; can use `TEXTWIDTH` for exactness.
- `BlAutoSize`: measure and set bounds.
- `BlValueToPos#(value#, min#, max#, length%)`: mapping.
- `BlPosToValue#(pos%, length%, min#, max#)`: inverse mapping.
- `BlClamp#(v#, lo#, hi#)`.
- `BlPostEvent%`: enqueue.
- `BlHitTest%`: hit test.
- `BlFail%`: set `BlStatus%`/`BlError%`.

Helpers use `BlPaintOX%`/`BlPaintOY%` and the current clip. They do not assume the full screen.

---

## 16. Errors and validation

`BlStatus%` is the primary result. `BlError%` is a numeric code; `BlError$` is a short message. The library does not raise BASIC runtime errors for expected failures (bad handle, full table, bad class), so a host `ON ERROR` handler is not surprised by library-internal errors.

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
- Class 1 (Top) is created only by `BlInit`; `BlCreate` rejects it.
- `BlCreate` accepts classes 2, 3, and 4. Classes 5–16 are reserved and rejected.
- In v0.1 the User class accepts kind 1 (button) and kind 5 (label); kinds 2–4 are rejected until v0.2.
- Text scale must be 1–8; transparency must be 0 or 1; wrap 0/1.
- Panel mode must be 0, 1, or 2 in v0.1; mode 3 is rejected.
- Parent must exist and not be the child itself or a descendant. Parent 0 is only valid for the root.
- Width and height must be non-negative; zero is allowed but invisible.
- Coordinates are clamped to the 16-bit range; extreme values are rejected rather than wrapped.
- `BlCreate` validates everything before allocating or linking. On failure, no partial control remains.
- `BlDestroy` clears capture/focus/event references to the handle and dispatches `CaptureLost` if needed.
- A frame that cannot be validated is dropped with a default result; it is never left on `STACK`.
- `BlAutoSize` rejects handles without text or with invalid scale.
- `BlSetActionLine` validates action 0–255 and line 0–65529.

---

## 17. Concurrency, timers, and reentrancy

baslib is synchronous. The host calls it from its main loop. `ON TIMER` is the only asynchronous BASIC mechanism and runs between statements.

Rules:

- A timer handler must not call baslib APIs. It should set a flag and return.
- The host processes timer flags at the top of its main loop, outside any baslib dispatch.
- If a timer interrupts a baslib call, shared scratch variables could be clobbered. The `STACK` frame and per-depth locals protect method arguments, but not arbitrary temporaries. The no-call rule avoids the problem.
- `BlPollInput`, `BlPaintAll`, and `BlLayoutAll` are not reentrant with respect to themselves.
- Nested class dispatch is supported through `BlSend`; it is depth-bounded and frame-balanced.
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
- `BlUpdate` redraws one control’s clip instead of the whole tree.
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
8. **Regression tests**: host merge, init failure paths, re-init, timer non-reentrancy rule, `SCREENW`/`SCREENH` root sizing, `BlActionLine%` dispatch.

### 19.2 How to run them

- State-only tests can run under `newbasic --cli` and print `PASS`/`FAIL` lines; a fixture `.expected` file compares output.
- Graphics tests use a graphics `SCREEN` and `POINT(x,y)` probes after `BlPaintAll`; works headlessly because the screen canvas is still present.
- For visual inspection, run the demo in a window and capture with `--gpu-shot FILE` (PPM).
- Synthetic input uses `BlTestClickAt`, `BlTestMouseMove`, `BlTestMouseButton`, and `BlTestWheel`, so tests do not depend on OS events.
- A sentinel test `STACK PUSH 12345 : GOSUB <api> : STACK POP X% : IF X% <> 12345 ...` verifies that no library call leaks or consumes host stack values.

### 19.3 Acceptance criteria for v0.1

- A host can merge the library, initialize a root, create a panel with a button, a label, and a click target, lay them out, paint, click, and receive events.
- A host `VIEW` is still active after `BlPaintAll`/`BlUpdate`; `VIEWX1`/`Y1`/`X2`/`Y2` match the host viewport.
- A label renders multiline text, aligns left/center/right and top/middle/bottom, truncates at pixel width, supports padding/border/transparency, and auto-sizes with `BlAutoSize`.
- Hover, enter/leave, press/release, wheel, and click events are observed through the event queue.
- `BlActionLine%` dispatch invokes the correct host line.
- Re-running `BlInit` resets all state without leaking handles; every handle from before the re-init is invalid.
- 255 controls can be created; the 256th fails cleanly with code 2.
- A 32-deep dispatch succeeds; the 33rd fails with code 6 and every frame is balanced.
- `STACK` has no baslib leakage; host sentinels survive API calls.
- No baslib call raises an unexpected BASIC runtime error in the test suite.
- The demo source stays below line 30000 and the library stays within 30000–49999.
- `BlInit` works with a non-720 graphics canvas and sizes the root from `SCREENW`/`SCREENH`.

---

## 20. Planning decisions recorded

### 20.1 Input

v0.1 is **live-polled**: click, hover, enter/leave, press/release, and wheel. The click queue remains the action path; live state drives visuals and future drag. Drag/capture is implemented in the core; continuous controls use it in v0.2.

`SLEEP` is host-loop pacing, not a library primitive. `BlPollInput` never blocks.

### 20.2 v0.1 control set

v0.1 ships the **button, label, and click target**, plus Top and Panel. Slider, gauge, and dial are v0.2. The class table, user-kind table, value storage, and capture fields are present from the start.

### 20.3 Library location

```
baslib/
    baslib-plan.md
    baslib.bas
    demo.bas
    tests/
```

The early sketches remain under `samples/baslib/` for reference; their manual stack is superseded by `STACK`.

### 20.4 Display and text

baslib requires a graphics canvas but not a specific mode. `BlInit` reads `SCREENW`/`SCREENH` unless the host supplies `BlReqW%`/`BlReqH%`. `DRAWTEXT` plus `TEXTWIDTH`/`TEXTHEIGHT` supplies deterministic text layout. Explicit multiline and auto-size are v0.1; word wrap is v0.2.

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

1. **Scaffold** `baslib.bas` with the line map, version constants, and the `30000 END` guard.
2. **STACK frame protocol**: push/load/dispatch/close/drop, depth, per-depth cache/locals, sentinel tests.
3. **Method tables and computed dispatch**: `BlMethod%`, `BlUserMethod%`, no-op lines, invalid cleanup.
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
| Host variable name collides with `Bl...` | Silent state corruption. | Namespace rule in README and demo; keep all names prefixed. |
| Host uses `STACK` too | A library frame leak or over-pop corrupts host data. | Fixed 14-slot frames; sentinel tests; never pop below baslib’s frames; drop invalid frames. |
| Unexpected runtime error inside a method | Frame left on `STACK`; depth wrong. | Use status returns for expected failures; host `CLEAR` + `BlInit` on unexpected errors; tests assert balance. |
| No `STACK` depth query | Library cannot see how many host values are below. | Track `BlFp%` and `BlWorkCount%`; only pop known frame/work items. |
| Method table uninitialized | Computed `GOSUB 0` or wrong handler. | `BlInit` fills every cell with the no-op line; validate line <> 0 before dispatch. |
| Computed `GOTO` into a `SELECT CASE` | Runtime `GOTO would leave a SELECT CASE`. | Use `GOSUB` or structure tail dispatch outside SELECT. |
| `VIEW` push/pop imbalance | Host viewport shifts; later drawing wrong. | Per-control `PUSH VIEW`/`POP VIEW`; test `VIEWX1`/`Y1`/`X2`/`Y2` after paint. |
| Input event storm | Queue fills; UI lags. | Coalesce move events; bounded ring; process at most eight clicks per poll. |
| Click and live button double-dispatch | Duplicate actions or missed drags. | Click queue is the action path; live buttons only drive down/up/hover/drag. |
| Capture points at destroyed control | Crash or wrong drag target. | Clear capture on destroy/hide; post `CaptureLost`. |
| Text measurement mismatch | Bad centering/truncation at scale. | Use `TEXTWIDTH`/`TEXTHEIGHT` for all layout; avoid `LEN*8` assumptions. |
| Screen mode changed at runtime | Root and clips stale. | `BlInit` requires a stable graphics mode; re-init after `SCREEN`. |
| Host forgets `BlInit` after `RUN`/`LOAD` | Stack/variables empty; status 1. | Document; `BlStatus% = 1` guard. |
| `MERGE` workflow forgotten | API lines absent. | Demo build script and README. |
| Library too large | Long compile/run times. | Shared class code; avoid duplicated per-kind boilerplate; measure line count per milestone. |
| String-array memory | 255 controls × several string arrays. | Two common string arrays plus small class text; keep captions short. |

---

## 23. Glossary

- **Class** — a handler bundle shared by instances; identified by a small integer.
- **Instance / handle** — one control row, 1–255; 0 is null.
- **Verb** — an operation dispatched to a class (`Paint`, `Click`, `MouseMove`, …).
- **Method table** — `BlMethod%(class, verb)` line-number table used by computed `GOSUB`.
- **Frame** — one 14-slot `STACK` block plus per-depth argument cache/locals.
- **Depth (`BlFp%`)** — current baslib frame index; host stack values are below it.
- **Work stack** — `STACK` items used by iterative layout/paint/destroy traversal.
- **Capture** — a control receiving mouse move/up while a button is held.
- **Hover** — live pointer over a control; tracked separately from clicks.
- **Layout pass** — computes local→absolute rectangles and clips.
- **Paint pass** — walks the tree back-to-front, sets `VIEW`/clip, and asks each class to paint itself.
- **Client area** — a panel’s inner rectangle after border/padding.
- **Z-order** — sibling order; later siblings paint in front and win hit tests.
- **Bubbling** — if a control does not handle a click, its parent is offered the click.
- **Auto-size** — set a control’s width/height from `TEXTWIDTH`/`TEXTHEIGHT`.
- **Action line** — host line number stored in `BlActionLine%` for an action ID.

---

## 24. Change log

- **2026-10-23** — Initial plan. Accepted packaging, handle model, two-level dispatch, frame stack, and local-coordinate layout. Recorded the click-only input constraint.  
- **2026-10-23** — Closed planning choices: click-only v0.1, button + click target first, `SCREEN 720` required, and the library promoted to top-level `baslib/`.  
- **2026-10-23** — Confirmed click bubbling, absolute/vertical/horizontal layout for the first implementation, and request-global creation APIs.  
- **2026-10-24** — Rebased baslib from 10000–29999 to **30000–49999**. Host owns 1–29999; other libraries may use 50000–65529. All static baslib lines stay inside `RENUM`’s 65529 ceiling.  
- **2026-10-24** — Added **Label** as User kind 5: single-line text, horizontal/vertical alignment, padding, optional border, configurable text/background colors, and click-through hit testing.  
- **2026-10-24** — Adopted **DRAWTEXT** for text. Labels default to transparent backgrounds and support text scale 1–8.  
- **2026-10-24** — **Draft 0.2 redesign** after newbasic added computed `GOTO`/`GOSUB`, `STACK`, `SELECT CASE`, `SLEEP`, live mouse state, `TEXTWIDTH`/`TEXTHEIGHT`, `SCREENW`/`SCREENH`, `VIEW` queries, and `PUSH VIEW`/`POP VIEW`:
  - replaced static `ON ... GOSUB` dispatch with a class × verb method table and computed `GOSUB`;
  - replaced the hand-rolled frame/work arrays with a 14-slot `STACK` frame protocol and `STACK` work stack;
  - moved from click-only to live hover/press/wheel input, with capture/drag infrastructure for v0.2;
  - made painting preserve the host viewport with `PUSH VIEW`/`POP VIEW`;
  - made the root size from `SCREENW`/`SCREENH` instead of a fixed 1280×720;
  - added exact text metrics, multiline labels, and auto-size;
  - added action-line dispatch and v0.2 dynamic class registration.
