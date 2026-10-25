# baslib Programmer's Reference and User Guide

**Library version:** 0.3 (`blVersion$ = "0.3"`, `blVersionNum# = 0.3`)  
**Target dialect:** newbasic (GW-BASIC-compatible, plus newbasic extensions)  
**Implementation:** [`baslib.bas`](baslib.bas)  
**Design:** [`baslib-plan.md`](baslib-plan.md)  
**Implementation notes:** [`baslib-imp.md`](baslib-imp.md)  
**Examples:** [`samplehost.bas`](samplehost.bas), [`demo.bas`](demo.bas), [`demo-controls.bas`](demo-controls.bas)

> **0.3 API.** baslib is now called with parameters:
>
> ```basic
> CALL blInit(640, 400)
> PanelH% = blCreate%(1, 2, 0, 10, 10, 300, 200, "", 0, 0, 0)
> ButtonH% = blCreate%(PanelH%, 3, 1, 0, 0, 100, 24, "OK", 100, 0, 0)
> CALL blSetLayout(PanelH%, 1, 4, 4)
> CALL blPaintAll
> ```
>
> There are no `blApi...` labels, no `blReq...` inputs, and no `blRet...`
> outputs. `SUB` calls carry inputs; `FUNCTION` calls return handles, strings,
> numbers, and event fields. `blStatus%` is the only common result global.

---

## Contents

1. [What baslib is](#1-what-baslib-is)
2. [Quick start](#2-quick-start)
3. [Calling convention](#3-calling-convention)
4. [Constants](#4-constants)
5. [Public API](#5-public-api)
6. [Controls](#6-controls)
7. [Layout and painting](#7-layout-and-painting)
8. [Input, events, and actions](#8-input-events-and-actions)
9. [The host loop](#9-the-host-loop)
10. [Testing hooks](#10-testing-hooks)
11. [Implementation notes and limits](#11-implementation-notes-and-limits)
12. [API line map](#12-api-line-map)

---

## 1. What baslib is

baslib is a source-level control framework for newbasic. A host merges
`baslib.bas` after its own numbered lines and calls public procedures. The
library owns a control tree, per-control state, layout, painting, hit testing,
events, and action dispatch.

It is not an interpreter extension and not a native widget set. Controls are
drawn with `LINE`, `CIRCLE`, and `DRAWTEXT`; text is measured with `TEXTWIDTH`
and `TEXTHEIGHT`.

The public surface is deliberately small:

- `SUB` for operations that do not return a value.
- `FUNCTION` for creation, lookups, values, next-event, and event fields.
- `blStatus%` for the result code of the most recent API call.

No host value on `STACK` is touched. No frame protocol, class-router table, or
computed dispatch is used.

---

## 2. Quick start

```basic
10 SCREEN 720
20 CALL blInit(640, 400)
30 IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus% : END
40 PanelH% = blCreate%(1, 2, 0, 40, 40, 320, 200, "", 0, 0, 0)
50 CALL blSetLayout(PanelH%, 1, 12, 8)
60 ButtonH% = blCreate%(PanelH%, 3, 1, 0, 0, 180, 32, "Click me", 100, 0, 0)
70 CALL blSetActionLine(100, 1000)
80 CALL blPaintAll
90 ClickH% = blPollInput%()
100 WHILE blEventCount%() > 0
110   EventTyp% = blNextEvent%()
120   ActionId% = blEventAction%()
130   IF ActionId% <> blActionNone% THEN GOSUB blActionLine&(ActionId%)
140 WEND
150 KeyChar$ = INKEY$ : IF KeyChar$ = CHR$(27) THEN END
160 CALL blPaintAll
170 SLEEP 16
180 GOTO 90
1000 PRINT "button clicked"
1010 RETURN
```

`blCreate%` returns a handle, or `0` on failure. `blStatus%` says why.

---

## 3. Calling convention

### 3.1 SUB calls

Use `CALL` with parentheses:

```basic
CALL blSetText(ButtonH%, "Ready")
CALL blSetBounds(ButtonH%, 10, 20, 120, 28)
CALL blPaintAll
```

A `SUB` may return early with `EXIT SUB`. Expected failures do not raise BASIC
errors; they set `blStatus%`.

### 3.2 FUNCTION calls

Function names carry their return type suffix and require parentheses:

```basic
PanelH% = blCreate%(1, 2, 0, 10, 10, 300, 200, "", 0, 0, 0)
SliderVal# = blGetValue#(SliderH%)
ButtonText$ = blGetText$(ButtonH%)
EventTyp% = blNextEvent%()
```

### 3.3 Status

`blStatus%` is set by every public call:

- `0` — success.
- `3` — invalid or freed handle.
- `4` — wrong class for the call.
- `7` — invalid parent.
- `2` — control table full.
- `10` — invalid value or kind.
- `13` — `blNextEvent%` found an empty queue.

A failed `blCreate%` returns `0`. A failed `blGetText$` returns `""`; a failed
`blGetValue#` returns `0`. Check `blStatus%` when that matters.

### 3.4 No request/result globals

The 0.2 API's `blReq...` staging block and `blRet...` results are gone. The
only public global result is `blStatus%`. Event fields are read through
`blEvent...%()` functions after `blNextEvent%()`.

---

## 4. Constants

All constants are read-only and available before execution.

### 4.1 Version and limits

| Constant | Value | Meaning |
|---|---:|---|
| `blVersion$` | `"0.3"` | Version string |
| `blVersionNum#` | `0.3` | Numeric version |
| `blMaxControls%` | 255 | Handles 1–255; 0 is null |
| `blMaxClasses%` | 16 | Method-table rows |
| `blMaxVerbs%` | 20 | Method-table columns |
| `blMaxEvents%` | 64 | Event ring size |
| `blHandleRoot%` | 1 | Root handle |
| `blActionNone%` | 0 | No action |
| `blNoLine&` | 0 | No handler line |

`blMaxFrames%`, `blMaxArgs%`, and `blMaxStrArgs%` remain as compatibility
constants but are unused; there is no frame protocol.

### 4.2 Status codes

| Constant | Value | Meaning |
|---|---:|---|
| `blStatusOk%` | 0 | Success |
| `blStatusControlTableFull%` | 2 | No free handle |
| `blStatusInvalidHandle%` | 3 | Handle is 0, out of range, or free |
| `blStatusInvalidClass%` | 4 | Wrong class for the call |
| `blStatusInvalidParent%` | 7 | Parent handle is invalid or free |
| `blStatusCannotDestroyRoot%` | 8 | Root cannot be destroyed |
| `blStatusInvalidValue%` | 10 | Bad kind, value, action ID, or line |
| `blStatusEventQueueFull%` | 12 | Event was dropped |
| `blStatusEventQueueEmpty%` | 13 | No event to pop |

Codes 1, 5, 6, 9, 11, 14–17 are reserved or historical.

### 4.3 Classes

| Value | Name | Purpose |
|---:|---|---|
| 1 | Top | Root |
| 2 | Panel | Container with background, border, and child layout |
| 3 | User | Button, label, slider, gauge, or dial |
| 4 | ClickTarget | Invisible action rectangle |

### 4.4 User kinds

| Value | Kind |
|---:|---|
| 1 | Button |
| 2 | Slider |
| 3 | Gauge |
| 4 | Dial |
| 5 | Label |

### 4.5 Event types

| Value | Constant |
|---:|---|
| 1 | `blEvClick%` |
| 2 | `blEvChange%` |
| 3 | `blEvMove%` |
| 4 | `blEvEnter%` |
| 5 | `blEvLeave%` |
| 6 | `blEvDown%` |
| 7 | `blEvUp%` |
| 8 | `blEvWheel%` |

### 4.6 Flags

| Value | Constant | Meaning |
|---:|---|---|
| 1 | `blFlagVisible%` | Visible |
| 2 | `blFlagEnabled%` | Hit-testable and clickable |
| 4 | `blFlagClip%` | Control clips children |
| 8 | `blFlagDirty%` | Reserved |
| 16 | `blFlagHot%` | Hover hint |
| 32 | `blFlagPressed%` | Pressed hint |
| 64 | `blFlagClickable%` | Reserved |
| 128 | `blFlagFocusable%` | Reserved |
| 256 | `blFlagTransparent%` | Reserved |
| 512 | `blFlagLayoutDirty%` | Layout hint |
| 1024 | `blFlagCaptured%` | Reserved |

---

## 5. Public API

### 5.1 Lifecycle

#### `blInit(Width%, Height%)`

Initializes or reinitializes the library. A non-positive `Width%`/`Height%` uses
the current canvas size (`SCREENW`/`SCREENH`), falling back to 640×400. Root
handle is always 1. Clears controls, events, hover, capture, and status.

#### `blCreate%(Parent%, Class%, Kind%, PosX%, PosY%, Width%, Height%, Text$, Action%, Flags%, TagId%)`

Creates a control and returns its handle, or `0` on failure.

| Parameter | Meaning |
|---|---|
| `Parent%` | Parent handle; root is 1 |
| `Class%` | 2 panel, 3 user, 4 click target |
| `Kind%` | User kind 1–5; ignored for panels/click targets |
| `PosX%, PosY%, Width%, Height%` | Initial local rectangle |
| `Text$` | Caption/label text |
| `Action%` | Action ID stored on the control |
| `Flags%` | Extra flags; visible/enabled are always added |
| `TagId%` | Host tag stored on the control |

Panels get mode 0, pad 0, gap 0. User controls get default values: 0..100,
step 1, pad 2, centered, scale 1, border 0. Click targets default to button
mask 1.

#### `blDestroy(Handle%)`

Destroys `Handle%` and its subtree. Refuses root with status 8.

### 5.2 Geometry and layout

#### `blSetBounds(Handle%, PosX%, PosY%, Width%, Height%)`

Sets the local rectangle and marks layout/paint dirty. Absolute coordinates are
stale until the next layout pass.

#### `blSetLayout(Handle%, Mode%, PadSize%, GapSize%)`

Configures a panel or the root: mode 0 absolute, 1 vertical, 2 horizontal.
Mode 3 and other values behave as absolute.

#### `blLayoutAll`

Recomputes absolute rectangles and clip rectangles for the whole tree.

#### `blAutoSize(Handle%)`

Sizes a user control from its text, current scale, and pad. Usually used on a
label after `blSetText`.

### 5.3 Visibility and state

| Call | Effect |
|---|---|
| `blShow(Handle%)` | Set visible |
| `blHide(Handle%)` | Clear visible; clears hover/press/capture |
| `blSetEnabled(Handle%, Enabled%)` | Set enabled flag |
| `blInvalidate(Handle%)` | Mark for repaint |
| `blUpdate(Handle%)` | Repaint `Handle%` and its descendants |

### 5.4 Text and values

#### `blSetText(Handle%, Text$)`

Sets caption/label text. CR/LF is supported.

#### `blGetText$(Handle%)`

Returns the stored text, or `""` for a bad handle.

#### `blSetValue(Handle%, Value#)` / `blGetValue#(Handle%)`

Sets/reads a user-control value. The setter clamps to the control's current
minimum/maximum (`blUser#(h,3)` and `blUser#(h,4)`).

### 5.5 Actions

#### `blSetAction(Handle%, Action%)`

Changes the action ID stored on a control.

#### `blSetActionLine(ActionId%, Line&)`

Maps an action ID to a host line. `Line&` is non-negative; `0` means no
handler. This is the only numeric-line API because the host dispatches action
handlers with `GOSUB`.

#### `blSetClickMask(Handle%, Mask%)`

Sets the click-target button mask. The current click path records but does not
enforce the mask; host code can filter on `blEventButton%()`.

### 5.6 Painting

| Call | Effect |
|---|---|
| `blPaintAll` | Lay out if needed, clear paint flag, paint whole tree |
| `blUpdate(Handle%)` | Paint `Handle%` and descendants |

Painting preserves the host viewport through `PUSH VIEW`/`POP VIEW`.

### 5.7 Hit test and events

#### `blFindAt%(PosX%, PosY%)`

Returns the topmost visible, enabled control containing the absolute point, or
0.

#### `blPollInput%()`

Polls live mouse state, moves/presses/releases/wheels, and dispatches a click
when `MOUSEB` is nonzero. Returns the clicked control handle, or 0. Events are
queued.

#### `blNextEvent%()`

Pops one event and returns its type, or 0 when empty. Event fields are then
read with:

| Function | Field |
|---|---|
| `blEventCtl%()` | Control handle |
| `blEventAction%()` | Action ID |
| `blEventX%()` | X coordinate |
| `blEventY%()` | Y coordinate |
| `blEventButton%()` | Button |
| `blEventWheel%()` | Wheel delta |
| `blEventCount%()` | Number still queued |

### 5.8 Debug and extension

#### `blDump`

Prints allocated controls: handle, class, parent, absolute X/Y, width/height.

#### `blRegisterClass(Class%, Line&, Name$)`

Stores a name and fills a method-table row for class 5–16. There is no class
dispatch; this is metadata for host extensions.

### 5.9 Synthetic input

| Call | Effect |
|---|---|
| `blTestClickAt%(PosX%, PosY%, Button%)` | Click and return hit handle |
| `blTestMouseMove(PosX%, PosY%)` | Move hover/capture |
| `blTestMouseButton(Button%, Down%, PosX%, PosY%)` | Press/release |
| `blTestWheel(PosX%, PosY%, Delta%)` | Wheel at point |

These are deterministic and do not require a real mouse.

---

## 6. Controls

### 6.1 Top (class 1)

Root handle 1. Size comes from `blInit` or the canvas. Its panel fields control
layout and colors.

### 6.2 Panel (class 2)

Panel fields:

| Field | Meaning |
|---:|---|
| `blPanel%(h,1)` | Mode 0/1/2 |
| `blPanel%(h,2)` | Padding |
| `blPanel%(h,3)` | Gap |
| `blPanel%(h,4)` | Columns (stored, unused) |
| `blPanel%(h,6)` | Face color |
| `blPanel%(h,7)` | Border color; -1 hides border |

### 6.3 User controls (class 3)

| Kind | Behavior |
|---|---|
| Button (1) | Face/border/text; click action |
| Slider (2) | Track and thumb; drag/click sets value |
| Gauge (3) | Read-only value bar |
| Dial (4) | Circular value; click/drag sets value |
| Label (5) | Text; optional border/face/wrap |

User fields (`blUser#(h,field)`):

| Field | Meaning |
|---:|---|
| 1 | Kind |
| 2 | Value |
| 3 | Minimum |
| 4 | Maximum |
| 5 | Step (stored) |
| 8 | Face color |
| 9 | Accent/pressed color |
| 13 | Text padding |
| 14 | Align: 0 left, 1 center, 2 right |
| 15 | VAlign: 0 top, 1 middle, 2 bottom |
| 16 | Wrap flag |
| 17 | Border color; -1 hides |
| 18 | Transparent face (1) |
| 19 | Text color |
| 20 | Text scale |

Direct field writes are the advanced styling path; change them before painting.

### 6.4 Click target (class 4)

Invisible rectangle; posts its action when clicked. `blClick%(h,1)` stores the
button mask.

### 6.5 Common table

`blCtl%(h,field)`:

| Field | Meaning |
|---:|---|
| 1 | Class; 0 = free |
| 2 | Parent |
| 3 | First child |
| 4 | Last child |
| 5 | Previous sibling |
| 6 | Next sibling |
| 7/8 | Local X/Y |
| 9/10 | Width/height |
| 11/12 | Absolute X/Y |
| 13 | Flags |
| 14 | Reserved |
| 15 | Action |
| 16 | Tag |
| 17 | Reserved |

---

## 7. Layout and painting

### 7.1 Layout modes

- **0 — absolute.** Child X/Y are used directly; W/H stay as created.
- **1 — vertical.** Children get the client width, advance Y by height + gap.
- **2 — horizontal.** Children get the client height, advance X by width + gap.

Hidden children are skipped. Zero-size children still consume stack space.
Nested panels are supported; each layout activation has local state.

### 7.2 Paint order

The parent paints first, then children in creation order. Later siblings appear
on top. Each control uses `PUSH VIEW`/`POP VIEW`; baslib never changes the host
viewport rectangle.

Clip rectangles are cached by layout but paint does not apply per-control
clipping; keep children inside parents.

### 7.3 Text

`blDrawText` trims each line to the available width, centers/aligns it, and
calls `DRAWTEXT`. Labels use `blWrapText$` only when `blUser#(h,16)` is set.
Set `blUser#(h,13)` for padding, `14/15` for alignment, `20` for scale.

---

## 8. Input, events, and actions

### 8.1 Poll order

`blPollInput%()`:

1. Reads `MOUSEPOSX`/`MOUSEPOSY` and moves hover/capture.
2. Reads `MOUSEBUTTONS AND 1` and posts down/up.
3. Reads `MOUSEWHEEL` and posts a wheel event at the hovered control.
4. If `MOUSEB` is nonzero, reads `MOUSEX`/`MOUSEY`, hit-tests, updates
   slider/dial values, bubbles the action, and posts a click.

### 8.2 Event order

A move can post `Leave`, `Enter`, and `Move`; a press posts `Down`; a release
posts `Up`; a wheel posts `Wheel`; a click posts `Click`. A slider/dial click
posts `Change` before `Click`.

### 8.3 Click bubbling

The hit control's own nonzero action wins. Otherwise the search walks parents
until it finds a nonzero action. The event control is the action owner.

### 8.4 Actions

`blSetActionLine` maps an action ID to a numeric host line. The host dispatches:

```basic
ActionId% = blEventAction%()
IF ActionId% <> blActionNone% THEN GOSUB blActionLine&(ActionId%)
```

Check the handler line against `blNoLine&` first if the table may be sparse.

---

## 9. The host loop

```basic
CALL blPaintAll
Loop:
  ClickH% = blPollInput%()
  WHILE blEventCount%() > 0
    EventTyp% = blNextEvent%()
    GOSUB DispatchAction
  WEND
  KeyChar$ = INKEY$
  IF KeyChar$ = CHR$(27) THEN END
  CALL blPaintAll
  SLEEP 16
  GOTO Loop
```

`blPaintAll` every frame is simple and safe. `blUpdate(Handle%)` repaints one
subtree; call it after changing that subtree and before presenting.

---

## 10. Testing hooks

The synthetic hooks let host tests drive deterministic input:

```basic
CALL blInit(640, 400)
ButtonH% = blCreate%(1, 3, 1, 10, 10, 100, 24, "OK", 100, 0, 0)
CALL blTestMouseMove(20, 20)
CALL blTestMouseButton(1, 1, 20, 20)
CALL blTestMouseButton(1, 0, 20, 20)
ClickH% = blTestClickAt%(20, 20, 1)
EventTyp% = blNextEvent%()
```

`blTestClickAt%` returns the clicked control; when the action bubbles to a
parent, it returns the action owner. Read `blEventCtl%()` after
`blNextEvent%()` for the dispatched owner.

---

## 11. Implementation notes and limits

- **No frames or dispatch.** Internals are `SUB`/`FUNCTION`/`LOCAL`; no
  `STACK`, no frame depth, no class routers, no `blDispatch`/`blSend`.
- **No request globals.** `blReq...` and `blRet...` are removed. The only
  common result is `blStatus%`.
- **Nested panels are fixed.** `blLayoutChildren` keeps all cursor/mode state
  local to its activation.
- **Clipping is computed but not applied during paint.** Keep children inside
  parents.
- **No double buffering or damage compositor.** Repaint the frame or subtree.
- **Grid mode is not implemented.** Modes other than 1/2 behave absolute.
- **Word wrap is opt-in.** Set `blUser#(h,16)` and size the control.
- **Mouse polling tracks left button** for down/up; `MOUSEB` drives clicks.
- **Event ring holds 64.** Drain every frame.
- **Click-target masks are stored, not enforced.**
- **Disabled ancestors do not disable children.** Disable subtrees explicitly.
- **Keyboard focus is not implemented.** Use `INKEY$` in the host.
- **Class registration is metadata only.** There is no method dispatch.
- **Host `STACK` is untouched.** The old sentinel is now only a non-interference
  check.

---

## 12. API line map

Public procedures occupy `510000–510552`; the debug helper is at `512010`.

| Line | Procedure |
|---:|---|
| 510000 | `blInit` |
| 510060 | `blCreate%` |
| 510110 | `blDestroy` |
| 510150 | `blSetBounds` |
| 510170 | `blSetLayout` |
| 510190 | `blShow` |
| 510200 | `blHide` |
| 510220 | `blSetEnabled` |
| 510240 | `blSetText` |
| 510250 | `blGetText$` |
| 510260 | `blSetValue` |
| 510270 | `blGetValue#` |
| 510280 | `blSetAction` |
| 510290 | `blSetActionLine` |
| 510300 | `blSetClickMask` |
| 510310 | `blInvalidate` |
| 510320 | `blLayoutAll` |
| 510330 | `blPaintAll` |
| 510340 | `blUpdate` |
| 510350 | `blAutoSize` |
| 510370 | `blPollInput%` |
| 510390 | `blNextEvent%` |
| 510410 | `blEventCtl%` |
| 510420 | `blEventAction%` |
| 510430 | `blEventX%` |
| 510440 | `blEventY%` |
| 510450 | `blEventButton%` |
| 510460 | `blEventWheel%` |
| 510470 | `blEventCount%` |
| 510480 | `blFindAt%` |
| 510490 | `blDump` |
| 510500 | `blRegisterClass` |
| 510520 | `blTestClickAt%` |
| 510530 | `blTestMouseMove` |
| 510540 | `blTestMouseButton` |
| 510550 | `blTestWheel` |

Host lines must stay below 500000. The library stays inside 500000–519999.
