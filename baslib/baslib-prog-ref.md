# baslib Programmer's Reference and User Guide

**Library version:** 0.1 (`blVersion$ = "0.1"`, `blVersionNum# = 0.1`)  
**Target dialect:** newbasic (GW-BASIC-compatible, plus newbasic extensions)  
**Implementation:** [`baslib.bas`](baslib.bas)  
**Design:** [`baslib-plan.md`](baslib-plan.md)  
**Implementation plan:** [`baslib-imp.md`](baslib-imp.md)  
**Example host:** [`samplehost.bas`](samplehost.bas), [`demo.bas`](demo.bas), [`demo-controls.bas`](demo-controls.bas)  
**Other library sources:** [`tests/`](tests/)

> **Read this first.** This guide documents the **shipped implementation** in
> `baslib.bas`, not only the intended design in `baslib-plan.md`. The library
> still calls itself 0.1, although some plan-0.2 value controls are present.
> Section 15 lists every place where the current build differs from the plan.
> When the two disagree, the implementation is what runs.

---

## Contents

1. [What baslib is](#1-what-baslib-is)
2. [Quick start](#2-quick-start)
3. [Host integration contract](#3-host-integration-contract)
4. [Programming model](#4-programming-model)
5. [Calling convention](#5-calling-convention)
6. [Constants](#6-constants)
7. [API reference](#7-api-reference)
8. [Control reference](#8-control-reference)
9. [Layout and painting](#9-layout-and-painting)
10. [Input, events, and actions](#10-input-events-and-actions)
11. [How to use baslib](#11-how-to-use-baslib)
12. [Tutorial: one of every control](#12-tutorial-one-of-every-control)
13. [Complete host program](#13-complete-host-program)
14. [Diagnostics and debugging](#14-diagnostics-and-debugging)
15. [Current implementation notes and limitations](#15-current-implementation-notes-and-limitations)
16. [Performance notes](#16-performance-notes)
17. [Glossary](#17-glossary)

Appendices:

- [Appendix A — API line and label map](#appendix-a--api-line-and-label-map)
- [Appendix B — Control table field maps](#appendix-b--control-table-field-maps)

---

## 1. What baslib is

baslib is a **BASIC source library** that turns a newbasic graphics screen into a
small control framework. A host program merges `baslib.bas` into its own source
and calls public entry points with `GOSUB`. baslib is not an interpreter
extension, not a DLL, and not a separate process. There is no `INCLUDE`
statement in newbasic, so merging or concatenating the source is the integration
mechanism.

The framework provides:

- a single top-level control (the **root**) that owns the screen;
- **panels** that contain children and arrange them;
- **user controls** — button, label, slider, gauge, and dial — selected by a
  *kind* number;
- invisible **click targets**;
- a back-to-front paint pass;
- hit testing and click delivery with parent bubbling;
- a bounded event queue, hover/press/wheel handling, and mouse capture;
- an **action-line table** that lets the host dispatch control actions to its
  own numbered lines without function pointers;
- a `blStatus%` return code instead of BASIC runtime errors for expected
  failures.

baslib deliberately does **not** provide:

- multiple windows or a window manager;
- keyboard focus, tab order, or text entry;
- a constraint solver, CSS-style layout, or word wrapping through the public
  API (the internal wrap field exists but is not populated by `blCreate`);
- double buffering or a damage-region compositor;
- themes as public variables (the current build uses fixed palette indices);
- native-looking widgets.

The library is synchronous and single-threaded. The host owns the main loop,
the timer, and the keyboard.

### 1.1 The two-layer picture

```text
Host program (lines 1-499999)
    |
    +-- calls public entry points      GOSUB blApiInit / blApiCreate / ...
    |       request globals  --->  baslib
    |       result globals   <---  baslib
    |
    +-- owns: SCREEN, main loop, ON TIMER, INKEY$, action handlers, data

baslib (lines 500000-519999)
    |
    +-- control tables, tree, layout, paint, hit test, event ring
    +-- class implementation and shared drawing helpers
    +-- public API wrappers at 510000-511959
```

### 1.2 Stability promise

The public API is a **label contract** plus a numeric line contract:

- Call `GOSUB blApi...`; never call a public routine by its number.
- The numbers in [Appendix A](#appendix-a--api-line-and-label-map) are stable
  for this major version and are useful to tools and debuggers.
- `LABEL` survives `RENUM`; numeric method/action tables do not. Do not renumber
  a merged program if anything stores line numbers in `blActionLine&` or a
  method table.

`CONST` names are read-only and installed before the first instruction. Host
code may read them freely.

### 1.3 newbasic facilities baslib relies on

The library is written in the same dialect as the host. These are the newbasic
features that matter when reading baslib or porting a host; the links point to
the project's reference manual.

| Facility | Use in baslib | Reference |
|---|---|---|
| `SCREEN` / graphics canvas | `SCREEN 720` (1280x720) is the normal host mode; any graphics mode works | [SCREEN](../RefManual/S.md#screen-statement) |
| `DRAWTEXT` | All control text, at pixel coordinates | [DRAWTEXT](../RefManual/D.md#drawtext-statement) |
| `TEXTWIDTH`, `TEXTHEIGHT` | Centering, truncation, auto-size | [TEXTWIDTH](../RefManual/T.md#textwidth-function), [TEXTHEIGHT](../RefManual/T.md#textheight-function) |
| `PUSH VIEW` / `POP VIEW` | Preserve the host viewport across paint | [PUSH VIEW](../RefManual/P.md#push-view-statement), [POP VIEW](../RefManual/P.md#pop-view-statement) |
| `SCREENW`, `SCREENH` | Root size when the host does not supply one | [SCREENW](../RefManual/S.md#screenw-function), [SCREENH](../RefManual/S.md#screenh-function) |
| `STACK` | Internal frame protocol and recursion sentinels | [STACK](../RefManual/S.md#stack-statement) |
| `LABEL` | Public API entry points; host labels | [LABEL](../RefManual/L.md#label-statement) |
| `CONST` | Read-only limits, statuses, IDs, flags | [CONST](../RefManual/C.md#const-statement) |
| `MOUSEB`, `MOUSEX`, `MOUSEY` | Queued click and latched coordinates | [MOUSEB](../RefManual/M.md#mouseb-function), [MOUSEX](../RefManual/M.md#mousex-function), [MOUSEY](../RefManual/M.md#mousey-function) |
| `MOUSEPOSX`, `MOUSEPOSY` | Live pointer position | [MOUSEPOSX](../RefManual/M.md#mouseposx-function), [MOUSEPOSY](../RefManual/M.md#mouseposy-function) |
| `MOUSEBUTTONS` | Live left/right/middle button mask | [MOUSEBUTTONS](../RefManual/M.md#mousebuttons-function) |
| `MOUSEWHEEL` | Accumulated wheel movement | [MOUSEWHEEL](../RefManual/M.md#mousewheel-function) |
| `INKEY$` | Host keyboard polling (baslib has no key events) | [INKEY$](../RefManual/I.md#inkey-function) |
| `ON TIMER`, `TIMER ON/OFF` | Host clock flag; never used inside baslib | [ON TIMER](../RefManual/O.md#on-timern-statement), [TIMER statement](../RefManual/T.md#timer-statement) |
| `TIMER`, `TIME$` | Timestamps and on-screen clock text | [TIMER](../RefManual/T.md#timer-function), [TIME$](../RefManual/T.md#time-function) |
| `SLEEP` | Host loop pacing in milliseconds | [SLEEP](../RefManual/S.md#sleep-statement) |

`STACK` deserves one warning: the library uses it, and so may the host. baslib
balances its own use, but a host that leaves an extra value on the stack will
eventually cause a type mismatch or an underflow in its own code. The sentinel
test in §3.5 catches this.

---

## 2. Quick start

### 2.1 Merge and run

At the newbasic `Ok` prompt:

```text
LOAD "baslib/samplehost.bas"
MERGE "baslib/baslib.bas"
RUN
```

Or, from a build script, concatenate the host and the library in line-number
order (`samplehost.bas` first, then `baslib.bas`) and pass the merged file to
newbasic.

For the two smaller demos:

```text
LOAD "baslib/demo.bas"
MERGE "baslib/baslib.bas"
RUN
```

```text
LOAD "baslib/demo-controls.bas"
MERGE "baslib/baslib.bas"
RUN
```

Press `Esc` to exit a demo.

### 2.2 Smallest useful program

This creates a button and prints a message when it is clicked. It is deliberately
minimal; it does not reset the request block before `blApiCreate`, so do not use
it as a template for multi-control programs.

```basic
10  SCREEN 720
20  blReqW% = 0 : blReqH% = 0
30  GOSUB blApiInit
40  IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus%: END
50  blReqClass% = blClassUser% : blReqParent% = blRoot% : blReqKind% = blKindButton%
60  blReqX% = 20 : blReqY% = 20 : blReqW% = 180 : blReqH% = 32
70  blReqText$ = "Click me" : blReqAction% = 100
80  GOSUB blApiCreate
90  IF blStatus% <> blStatusOk% THEN PRINT "create failed"; blStatus%: END
100 GOSUB blApiPaintAll
110 blReqActionId% = 100 : blReqLine& = 2010 : GOSUB blApiSetActionLine
120 GOSUB blApiPollInput
130 WHILE blEventCount% > 0
140   GOSUB blApiNextEvent
150   IF blEventAction% <> blActionNone% THEN IF blActionLine&(blEventAction%) <> blNoLine& THEN GOSUB blActionLine&(blEventAction%)
160 WEND
170 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
180 IF blNeedsPaint% THEN GOSUB blApiPaintAll
190 SLEEP 16
200 GOTO 120
2000 REM button action handler (the table must point at an executable line)
2010 PRINT "button clicked"
2020 RETURN
```

The action table stores **2010**, not 2000. A line containing only `REM` has no
executable instruction to receive a computed `GOSUB`; point action entries at a
line that does real work.

### 2.3 What a real host needs

The quick example omits three things a robust host must do:

1. reset the shared request block before every `blApiCreate`;
2. check `blStatus%` after every API call that can fail;
3. repaint after text or geometry changes, and drain events every frame.

Sections 11 and 12 show the complete pattern.

---

## 3. Host integration contract

### 3.1 Line ownership

| Lines | Owner |
|---|---|
| 1-499999 | Host program |
| 500000 | baslib `END` guard |
| 500010-519999 | baslib |
| 520000+ | Other libraries (do not overlap baslib) |

`baslib.bas` begins with:

```basic
500000 END
```

If host execution falls through line 499999, it stops at the guard instead of
running library subroutines. A host must still end its own flow with `END`, a
`GOTO` loop, or an explicit termination path. The guard is a safety net, not a
substitute for program structure.

Do not put a host line at 500000: `MERGE` replaces the host line with the
library guard.

### 3.2 Merge and renumber

- `MERGE` is a direct-mode command. It merges by line number.
- Concatenation in ascending line-number order is equivalent for a build script.
- `RENUM` renumbers the merged program and preserves labels, but any numeric
  value already stored in `blActionLine&` or built into a method table becomes
  stale. Renumber the host **before** merging, or leave the merged program
  unrenumbered.

### 3.3 Namespace and typing

Every baslib symbol starts with lowercase `bl` (case-insensitive in BASIC, so
`BL` is the same namespace). The host must not create variables, arrays, or
labels beginning with `bl`.

Suffix conventions used by baslib:

| Suffix | Type | Used for |
|---|---|---|
| `%` | 32-bit integer | handles, coordinates, flags, counts, class/verb/kind IDs, field indices |
| `&` | 32-bit long | line numbers in tables and request globals |
| `#` | double | values, text metrics, per-depth numeric args |
| `$` | string | captions, text, names |

BASIC identifiers are case-insensitive. `blTW%` and `blTw%` are the same
variable; this has bitten the implementation before. Use distinct descriptive
names rather than relying on case.

The host should use matching types in request globals. `blReqValue#` is double;
`blReqHandle%` is integer; `blReqLine&` is long.

### 3.4 Hard host rules

| Rule | Why |
|---|---|
| Select a graphics screen before `blApiInit` (for example `SCREEN 720`) | text mode has no graphics canvas; the root would fall back to 640x400 and painting would fail |
| Do not use `OPTION BASE 1` | baslib arrays are base 0; index 0 is reserved as "null" in several tables |
| Do not define names beginning with `bl` | silent collision with library state |
| Do not assign to `bl...` `CONST` names | they are read-only; assignment is a compile error |
| Do not rely on default numeric types in host request variables | use `%`, `&`, `#`, `$` explicitly |
| Call `blApiInit` before any other API | root and tables must exist |
| Call `blApiInit` again after `RUN`, `LOAD`, `NEW`, `CLEAR`, or `CHAIN` | those clear variables and `STACK` |
| Do not call baslib from an `ON TIMER` handler | a timer can interrupt a library call and clobber shared scratch state |
| Drain events every frame | `blApiPollInput` adds events; the ring holds only 64 |
| Repaint after geometry changes | there is no damage compositor |
| Keep host values on `STACK` balanced | baslib preserves them, but a host leak is still a host bug |

### 3.5 STACK coexistence

baslib uses `STACK` as a recursion/work sentinel in layout, paint, and destroy
paths, and the internal dispatch protocol uses it too. Public API calls are
balanced: a host value pushed before a call is still there afterward.

A useful integration test is:

```basic
STACK PUSH 12345
GOSUB blApiInit
GOSUB blApiPaintAll
STACK POP X%
IF X% <> 12345 THEN PRINT "STACK leak": END
```

`CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN` clear `STACK`. Re-run `blApiInit`
after any of them.

### 3.6 Status versus runtime errors

Expected failures (bad handle, invalid class, full table, empty event queue) set
`blStatus%` and return normally. They do not raise a BASIC error, so an
`ON ERROR` handler is not used as control flow.

An **unexpected** runtime error inside baslib (for example, an out-of-range
direct table write in host code that corrupts an index) can leave library state
inconsistent. The recovery is `CLEAR` followed by re-running the host from
initialization.

### 3.7 Timers

`ON TIMER(n) GOSUB line` runs the handler between statements. A timer handler
must:

- set a host flag, and
- `RETURN`.

It must not call baslib. The main loop processes the flag at the top, outside
any baslib call. This is the pattern used by the sample host's on-screen clock
(§12.4).

`SLEEP` blocks through the host and timer events are not serviced during the
sleep. Keep sleeps short; the sample uses `SLEEP 16`.

---

## 4. Programming model

### 4.1 Handles

- A **handle** is an integer from 1 to `blMaxControls%` (255).
- Handle `0` means "no control".
- Handle `1` is always the root (`blRoot%`, `blHandleRoot%`).
- `blApiCreate` returns a new handle in `blRetHandle%`, or 0 on failure.
- `blApiDestroy` frees the handle and all descendants. Reuse is allowed.
- There is no generation counter: a stale handle can be reused by a new
  control. Discard handles you have destroyed.

Handles are not z-order. Creation order is sibling order, and paint order is
front-to-back by sibling position.

### 4.2 Classes and kinds

Classes (`blClass...%` constants):

| ID | Class | Created by | Purpose |
|---:|---|---|---|
| 1 | `blClassTop%` | `blApiInit` | Root container; owns the screen and the default click target |
| 2 | `blClassPanel%` | `blApiCreate` | Container; background, border, child layout |
| 3 | `blClassUser%` | `blApiCreate` | One class for five kinds (button, slider, gauge, dial, label) |
| 4 | `blClassClickTarget%` | `blApiCreate` | Invisible rectangular hotspot |
| 5-16 | reserved | `blApiRegisterClass` (no public dispatch) | Host method-table rows |

User kinds (`blKind...%` constants):

| ID | Kind | Constant | Interactive? |
|---:|---|---|---|
| 1 | Button | `blKindButton%` | Yes, posts its action |
| 2 | Slider | `blKindSlider%` | Yes, click/drag changes value |
| 3 | Gauge | `blKindGauge%` | No, read-only display |
| 4 | Dial | `blKindDial%` | Yes, click/drag changes value |
| 5 | Label | `blKindLabel%` | No value; text only |

`blClassUser%` covers all five kinds. A label and a button are the same *class*
and different *kinds*.

### 4.3 Tree and z-order

- The root has parent 0.
- Each control has one parent and a doubly linked child list.
- `blApiCreate` appends the new control as the **last** child, which is the
  **front-most** sibling.
- Paint order: parent first, then children from first (back) to last (front).
- Hit test scans handles from 255 down to 1 and takes the first eligible
  rectangle. Because handles increase with creation, later-created siblings
  generally win, matching the paint order.
- Hidden controls and disabled controls are skipped by layout, paint, and hit
  test.
- A hidden or disabled **parent does not automatically disable its children** in
  the current hit-test implementation. Disable or hide the whole subtree
  explicitly if that is the desired behavior.

### 4.4 Geometry

- Units are integer pixels.
- A control's `X`/`Y` are **local to its parent's outer top-left**, not its
  padded client area.
- The absolute rectangle is computed by the layout pass: `AbsX = parent.AbsX +
  X`, `AbsY = parent.AbsY + Y`.
- `W`/`H` are the control's size. A zero or negative size is legal but is not
  painted; hit testing still uses the rectangle, so avoid negative sizes.
- The root's local rectangle is `(0, 0, rootW, rootH)`. `blApiInit` uses
  `blReqW%`/`blReqH%` when positive, otherwise `SCREENW`/`SCREENH` when the
  canvas is available, otherwise 640x400.

### 4.5 Instance storage

All instances share four fixed tables plus two string arrays:

| Table | Shape | Contents |
|---|---|---|
| `blCtl%(handle, field)` | 255 x 17 | class, parent, links, local/absolute bounds, flags, action, tag |
| `blPanel%(handle, field)` | 255 x 8 | panel layout and colors |
| `blUser#(handle, field)` | 255 x 22 | kind, value, range, visual properties, text layout |
| `blClick%(handle, field)` | 255 x 4 | click-target mask and cached state |
| `blCtlText$(handle)` | 255 | caption/label text |
| `blCtlName$(handle)` | 255 | debug name |

The full field maps are in [Appendix B](#appendix-b--control-table-field-maps).
Host code may **read** these tables for diagnostics. Writing them directly is
an advanced escape hatch: it bypasses validation and dirty-flag bookkeeping.
After a direct write that changes appearance, set `blNeedsPaint% = 1` and call
`blApiPaintAll`.

### 4.6 Flags

| Bit | Constant | Meaning |
|---:|---|---|
| 1 | `blFlagVisible%` | Control and subtree are visible |
| 2 | `blFlagEnabled%` | Accepts hit tests and clicks |
| 4 | `blFlagClip%` | Reserved for clip handling; panels set it, paint does not currently clip |
| 8 | `blFlagDirty%` | Per-control dirty hint; public API uses `blNeedsPaint%` instead |
| 16 | `blFlagHot%` | Pointer-over hint; input does not currently set it |
| 32 | `blFlagPressed%` | Pressed hint; input does not currently set it |
| 64 | `blFlagClickable%` | Advisory only |
| 128 | `blFlagFocusable%` | Reserved for future focus |
| 256 | `blFlagTransparent%` | Advisory; paint does not consult it |
| 512 | `blFlagLayoutDirty%` | Per-control layout-dirty hint |
| 1024 | `blFlagCaptured%` | Advisory; capture is held in `blCapture%` |

`blApiCreate` always adds visible and enabled. To create a hidden or disabled
control, create it and then call `blApiHide` or `blApiSetEnabled` with 0.

---

## 5. Calling convention

### 5.1 The shape of every call

baslib uses newbasic's global variables as a parameter block. There are no
function arguments and no `CALL` statement.

```basic
<set blReq... input globals>
GOSUB blApi<Something>          ' label call, not a function call
IF blStatus% <> blStatusOk% THEN <handle failure>
<read blRet... / blEvent... output globals>
```

Rules:

1. **Set every input the routine reads.** Request globals are ordinary globals
   and retain their old values.
2. **Check `blStatus%` immediately.** The next API call overwrites it.
3. **Read outputs before the next call.** `blRetHandle%`, `blRetValue#`,
   `blRetN%`, and the `blEvent...` globals are shared.
4. **Do not call baslib from a timer handler.** Set a flag instead.
5. **Calls are not reentrant.** Do not call an API from inside an action
   handler while another baslib operation is in progress. (An action handler
   runs from the host loop, outside baslib, so normal API calls there are fine.)

### 5.2 Request globals

| Global | Type | Read by | Meaning |
|---|---|---|---|
| `blReqClass%` | int | Create, RegisterClass | Class ID |
| `blReqParent%` | int | Create | Parent handle |
| `blReqHandle%` | int | most handle APIs | Target handle |
| `blReqX%`, `blReqY%` | int | Create, SetBounds, FindAt, test hooks | Local or absolute coordinates, depending on call |
| `blReqW%`, `blReqH%` | int | Init, Create, SetBounds | Size; Init also accepts 0 for "use canvas" |
| `blReqFlags%` | int | Create | Extra flag bits (visible and enabled are always added) |
| `blReqAction%` | int | Create, SetAction | Host action ID (0-255 for the action table) |
| `blReqTag%` | int | Create | Host-owned numeric tag, stored in the control row |
| `blReqKind%` | int | Create | User kind 1-5; required for class 3 |
| `blReqText$` | string | Create, SetText, RegisterClass | Caption, label text, or class name |
| `blReqValue#` | double | Create, SetValue | Initial or new value |
| `blReqMode%` | int | Create, SetLayout | Panel mode 0/1/2 (3 is stored but behaves as absolute) |
| `blReqPad%` | int | Create, SetLayout | Panel padding; also user-control text padding |
| `blReqGap%` | int | Create, SetLayout | Panel gap between children |
| `blReqCols%` | int | Create | Stored in `blPanel%(...,4)`; layout ignores it |
| `blReqButtonMask%` | int | Create | Stored in `blClick%(...,1)`; click dispatch does not enforce it |
| `blReqEnabled%` | int | SetEnabled | 1 = enable, 0 = disable |
| `blReqActionId%` | int | SetActionLine | Action ID 0-255 |
| `blReqLine&` | long | SetActionLine, RegisterClass | Host line number |
| `blReqButton%` | int | TestClickAt, TestMouseButton | Button code 1/2/3 |
| `blReqDown%` | int | TestMouseButton | 1 = press, 0 = release |
| `blReqWheel%` | int | TestWheel | Wheel delta |

The following request globals exist in the plan but are **ignored by the
current `blCreate`**: `blReqFace%`, `blReqAccent%`, `blReqTextColor%`,
`blReqTextScale%`, `blReqTransparent%`, `blReqAlign%`, `blReqVAlign%`,
`blReqBorderStyle%`, `blReqWrap%`, `blReqAutoSize%`. See §15 for the direct-table
workarounds.

### 5.3 Result globals

| Global | Type | Filled by | Meaning |
|---|---|---|---|
| `blStatus%` | int | every API | 0 = success, non-zero = error |
| `blError%` | int | reserved | Always 0 in the current build; use `blStatus%` |
| `blRetHandle%` | int | Create, FindAt, TestClickAt | Control handle, or 0 |
| `blRetValue#` | double | GetValue | Current user-control value |
| `blRetN%` | int | PollInput, internal ClickAt | Click-consumed count (1) or 0 |
| `blRoot%` | int | Init | Root handle (1) |
| `blNeedsPaint%` | int | many calls | 1 means a repaint is requested |
| `blNeedsLayout%` | int | geometry calls | 1 means layout is stale |
| `blEventType%` | int | NextEvent | Event type, or 0 when empty |
| `blEventCtl%` | int | NextEvent | Control that generated the event |
| `blEventAction%` | int | NextEvent | Action ID copied from the control |
| `blEventX%`, `blEventY%` | int | NextEvent | Event coordinates (absolute canvas pixels) |
| `blEventButton%` | int | NextEvent | Button code, or 0 |
| `blEventWheel%` | int | NextEvent | Wheel delta |
| `blEventCount%` | int | NextEvent/PollInput | Events still queued |

### 5.4 Stale request globals: the most common host bug

`blApiCreate` reads many request globals. If you set `blReqAction% = 100` for a
button and then create a label without resetting it, the label inherits action
100. If you set `blReqText$ = "Click me"` and then create a panel without
resetting it, the panel stores the caption. Some controls do not paint that
text, so the bug can be invisible until it fires an action.

Use a reset helper before every create:

```basic
7000 LABEL ResetReq
7010 blReqClass% = 0 : blReqParent% = 0 : blReqHandle% = 0
7020 blReqX% = 0 : blReqY% = 0 : blReqW% = 0 : blReqH% = 0
7030 blReqFlags% = 0 : blReqAction% = 0 : blReqTag% = 0
7040 blReqKind% = 0 : blReqMode% = 0 : blReqPad% = 0 : blReqGap% = 0 : blReqCols% = 0
7050 blReqButtonMask% = 1 : blReqText$ = "" : blReqValue# = 0 : blReqEnabled% = 1
7060 RETURN
```

Then:

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindButton%
blReqX% = 0 : blReqY% = 0 : blReqW% = 180 : blReqH% = 32
blReqText$ = "OK" : blReqAction% = 100
GOSUB blApiCreate
```

### 5.5 Numeric lines versus labels

- `GOSUB blApiCreate` is a compile-time label call. It survives `RENUM`.
- `blSetActionLine` and `blRegisterClass` store **numeric** lines because
  later dispatch uses a computed `GOSUB`.
- A host action-handler table entry must point at an executable line, not a
  `REM`-only line.
- In newbasic, a type-suffixed identifier in a line-target position is a
  computed line number: `GOSUB ActLine&` works. An unsuffixed computed variable
  is parenthesized: `GOTO (Line)`.

### 5.6 Two BASIC control-flow traps

**Colon statements after `THEN` belong to the `THEN` branch.** This is correct:

```basic
IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
```

Both assignments/calls happen only when the flag is set. This is a bug:

```basic
IF N% > 100 THEN END : GOTO MainLoop
```

When `N% <= 100`, the `GOTO` does **not** run, and execution falls through into
whatever follows (often an action handler, producing `RETURN without GOSUB`).
Write it as:

```basic
IF N% > 100 THEN END
GOTO MainLoop
```

**Do not let the main flow fall into handler lines.** Action handlers and
subroutines normally live after the main loop. End the main flow with `END` or a
`GOTO` loop before the first handler line. The library's `500000 END` guard only
protects the host/library boundary, not the host's internal layout.

---

## 6. Constants

All constants are declared in `baslib.bas` lines 500010-500088. They are
read-only, available before the first instruction, and safe to read in host code.

### 6.1 Version and limits

| Constant | Value | Meaning |
|---|---:|---|
| `blVersion$` | `"0.1"` | Library version string |
| `blVersionNum#` | `0.1` | Numeric version |
| `blMaxControls%` | 255 | Handles 1-255; index 0 is null |
| `blMaxFrames%` | 32 | Maximum internal dispatch depth |
| `blMaxArgs%` | 8 | Numeric args per internal frame |
| `blMaxStrArgs%` | 2 | String args per internal frame |
| `blMaxClasses%` | 16 | Method-table rows |
| `blMaxVerbs%` | 20 | Method-table columns |
| `blMaxKinds%` | 8 | User-kind method rows |
| `blMaxEvents%` | 64 | Event ring size |
| `blHandleRoot%` | 1 | Root handle |
| `blActionNone%` | 0 | "No action" |
| `blNoLine&` | 0 | "No handler line" |

### 6.2 Status codes

| Constant | Value | Meaning |
|---|---:|---|
| `blStatusOk%` | 0 | Success |
| `blStatusNotInit%` | 1 | Not initialized (defined; current public API does not return it) |
| `blStatusControlTableFull%` | 2 | No free handle |
| `blStatusInvalidHandle%` | 3 | Handle is 0, out of range, or free |
| `blStatusInvalidClass%` | 4 | Class ID is not 2-4 for create, or wrong class for the call |
| `blStatusInvalidVerb%` | 5 | Internal dispatch only |
| `blStatusFrameOverflow%` | 6 | Internal dispatch depth exceeded |
| `blStatusInvalidParent%` | 7 | Parent handle is invalid or free |
| `blStatusCannotDestroyRoot%` | 8 | Refused to destroy the root |
| `blStatusInvalidGeometry%` | 9 | Reserved; current API does not validate geometry |
| `blStatusInvalidValue%` | 10 | Bad kind, value, action ID, or line |
| `blStatusInvalidLayout%` | 11 | Reserved; modes are not validated |
| `blStatusEventQueueFull%` | 12 | Event ring full; the event was dropped |
| `blStatusEventQueueEmpty%` | 13 | `blApiNextEvent` called with no events |
| `blStatusNoGraphics%` | 14 | Reserved; `blInit` falls back to 640x400 |
| `blStatusFrameProtocol%` | 15 | Internal frame protocol only |
| `blStatusMissingActionHandler%` | 16 | Reserved |
| `blStatusClassTableFull%` | 17 | Reserved |

### 6.3 Classes, kinds, and verbs

| Constant | Value |
|---|---:|
| `blClassTop%` | 1 |
| `blClassPanel%` | 2 |
| `blClassUser%` | 3 |
| `blClassClickTarget%` | 4 |
| `blKindButton%` | 1 |
| `blKindSlider%` | 2 |
| `blKindGauge%` | 3 |
| `blKindDial%` | 4 |
| `blKindLabel%` | 5 |

Verb IDs (`blVerbPaint%` = 1 through `blVerbFocus%` = 17) are used by the
internal method table only. Public host code never dispatches verbs directly.

### 6.4 Event types

| Constant | Value | Meaning |
|---|---:|---|
| `blEvClick%` | 1 | A control's action fired on a click |
| `blEvChange%` | 2 | Slider/dial value changed |
| `blEvMove%` | 3 | Live pointer position update |
| `blEvEnter%` | 4 | Pointer entered a control |
| `blEvLeave%` | 5 | Pointer left a control |
| `blEvDown%` | 6 | Live left button pressed |
| `blEvUp%` | 7 | Live left button released |
| `blEvWheel%` | 8 | Wheel movement |
| `blEvKey%` | 9 | Reserved; library does not generate key events |
| `blEvFocus%` | 10 | Reserved |
| `blEvCaptureLost%` | 11 | Reserved; current hide/destroy does not post it |

### 6.5 Flags

See §4.6. The constants are `blFlagVisible%` (1), `blFlagEnabled%` (2),
`blFlagClip%` (4), `blFlagDirty%` (8), `blFlagHot%` (16), `blFlagPressed%` (32),
`blFlagClickable%` (64), `blFlagFocusable%` (128), `blFlagTransparent%` (256),
`blFlagLayoutDirty%` (512), and `blFlagCaptured%` (1024).

---

## 7. API reference

Every public routine is called with `GOSUB blApi...`. Inputs are the request
globals in §5.2; results are the globals in §5.3. The numeric line is shown for
tooling; use the label in source.

### 7.1 API map

| Line | Label | Routine | One-line summary |
|---:|---|---|---|
| 510000 | `blApiInit` | `blInit` | Reset library state and create the root |
| 510100 | `blApiCreate` | `blCreate` | Create a panel, user control, or click target |
| 510200 | `blApiDestroy` | `blDestroy` | Destroy a control and its descendants |
| 510300 | `blApiSetBounds` | `blSetBounds` | Move/resize a control |
| 510400 | `blApiShow` | `blShow` | Make a control visible |
| 510500 | `blApiHide` | `blHide` | Hide a control |
| 510600 | `blApiSetText` | `blSetText` | Set caption/label text |
| 510700 | `blApiSetValue` | `blSetValue` | Set a user-control value (clamped) |
| 510800 | `blApiGetValue` | `blGetValue` | Read a user-control value |
| 510900 | `blApiSetLayout` | `blSetLayout` | Set panel mode, padding, gap |
| 511000 | `blApiSetAction` | `blSetAction` | Set a control's action ID |
| 511100 | `blApiInvalidate` | `blInvalidate` | Request a repaint |
| 511200 | `blApiLayoutAll` | `blLayoutAll` | Recompute absolute bounds |
| 511300 | `blApiPaintAll` | `blPaintAll` | Lay out if needed and repaint everything |
| 511400 | `blApiUpdate` | `blUpdate` | Repaint a control and its subtree |
| 511500 | `blApiPollInput` | `blPollInput` | Poll mouse, dispatch clicks, queue events |
| 511550 | `blApiPollMouse` | `blPollMouse` | Alias for `blApiPollInput` |
| 511600 | `blApiNextEvent` | `blNextEvent` | Pop one event into the `blEvent...` globals |
| 511700 | `blApiFindAt` | `blFindAt` | Hit test an absolute point |
| 511800 | `blApiTestClickAt` | `blTestClickAt` | Synthetic click at absolute coordinates |
| 511810 | `blApiTestMouseMove` | `blTestMouseMove` | Synthetic live move |
| 511820 | `blApiTestMouseButton` | `blTestMouseButton` | Synthetic button down/up |
| 511830 | `blApiTestWheel` | `blTestWheel` | Synthetic wheel |
| 511900 | `blApiDump` | `blDump` | Print allocated controls (debug) |
| 511910 | `blApiAutoSize` | `blAutoSize` | Size a user control to its text |
| 511920 | `blApiSetActionLine` | `blSetActionLine` | Map an action ID to a host line |
| 511930 | `blApiRegisterClass` | `blRegisterClass` | Fill a host method-table row |
| 511950 | `blApiSetEnabled` | `blSetEnabled` | Enable/disable a control |

Routines with names like `blCreate`, `blPaintAll`, `blFramePush`, `blDispatch`,
and `blSend` are **internal**. Host code calls only the `blApi...` labels.

---

### 7.2 Lifecycle

#### `blApiInit` — initialize or reinitialize the library

**Line:** 510000  
**Inputs:** `blReqW%`, `blReqH%` (0 or negative means "use the canvas")  
**Outputs:** `blRoot% = 1`, `blStatus% = blStatusOk%`

Resets all control tables, method tables, action lines, event ring, and input
state, then creates the root control at handle 1.

Details:

- Root size: `blReqW%`/`blReqH%` if positive; otherwise `SCREENW`/`SCREENH` if
  the canvas is available; otherwise 640x400.
- Root class is `blClassTop%`, flags are visible + enabled + clip.
- Root colors are fixed: face 7, border 8.
- Old handles are invalid after re-init. Re-register action lines and recreate
  controls.
- `STACK` is not cleared by `blInit`; host values survive.
- The current build does not check that a graphics canvas exists and does not
  return `blStatusNoGraphics%`.

```basic
10 SCREEN 720
20 blReqW% = 0 : blReqH% = 0
30 GOSUB blApiInit
40 IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus%: END
```

#### `blApiCreate` — create a control

**Line:** 510100  
**Inputs:** class, parent, local x/y/w/h, optional flags/action/tag, plus
class-specific inputs  
**Outputs:** `blRetHandle%` (0 on failure), `blStatus%`

Accepted classes and required extra inputs:

| Class | Required extra inputs | Notes |
|---|---|---|
| `blClassPanel%` (2) | `blReqMode%`, `blReqPad%`, `blReqGap%`; `blReqCols%` optional | Container |
| `blClassUser%` (3) | `blReqKind%` (1-5), `blReqText$`, `blReqValue#`, `blReqPad%` | Button/label/slider/gauge/dial |
| `blClassClickTarget%` (4) | `blReqButtonMask%` (stored only) | Invisible |

Creation details:

- The new control is appended as the last (front-most) child of the parent.
- `blFlagVisible%` and `blFlagEnabled%` are always added to `blReqFlags%`.
  A control cannot be created hidden or disabled; call `blApiHide` or
  `blApiSetEnabled` afterward.
- `blReqAction%` is stored in the common action field; `blReqTag%` is stored for
  host use.
- The current build does **not** validate width, height, coordinates, or panel
  mode. Negative sizes are legal but do not paint.
- Class-specific defaults are listed in §8.9.
- On success, `blNeedsLayout%` and `blNeedsPaint%` are set.

Status:

| Status | Cause |
|---|---|
| 0 | Success |
| 2 | Control table full |
| 4 | Class is not 2, 3, or 4 |
| 7 | Parent is invalid/free |
| 10 | User kind is outside 1-5 |

```basic
GOSUB ResetReq
blReqClass% = blClassPanel% : blReqParent% = blRoot%
blReqX% = 40 : blReqY% = 24 : blReqW% = 640 : blReqH% = 440
blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
GOSUB blApiCreate
IF blStatus% <> blStatusOk% THEN PRINT "panel"; blStatus%: END
MainH% = blRetHandle%

GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindButton%
blReqX% = 0 : blReqY% = 0 : blReqW% = 180 : blReqH% = 28
blReqText$ = "Click me" : blReqAction% = 100
GOSUB blApiCreate
IF blStatus% <> blStatusOk% THEN PRINT "button"; blStatus%: END
ButtonH% = blRetHandle%
```

#### `blApiDestroy` — destroy a control and its subtree

**Line:** 510200  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

Destroys children first, detaches the control from its parent, clears its class
and text, and marks layout/paint dirty.

- Destroying the root is refused with `blStatusCannotDestroyRoot%` (8).
- An invalid or already-free handle returns 3.
- The current build does **not** clear `blHover%`, `blPressed%`, `blCapture%`,
  or pending event references to the destroyed handle. Destroy a control while
  the pointer is not over it, then poll again. Calling `blApiHide` first clears
  capture if that control owned it.

```basic
blReqHandle% = ButtonH%
GOSUB blApiDestroy
IF blStatus% <> blStatusOk% THEN PRINT "destroy"; blStatus%
```

---

### 7.3 Geometry and layout

#### `blApiSetBounds` — set local position and size

**Line:** 510300  
**Inputs:** `blReqHandle%`, `blReqX%`, `blReqY%`, `blReqW%`, `blReqH%`  
**Outputs:** `blStatus%`

Stores the local rectangle and marks the tree layout-dirty. It performs **no**
validation of coordinates or size. For a child of a vertical or horizontal
panel, the parent's next layout pass may overwrite `X`, `Y`, `W`, or `H`
according to the panel mode.

The absolute coordinates used by hit testing are stale until the next layout
pass. Call `blApiLayoutAll` or `blApiPaintAll` before `blApiFindAt` or a click
after a bounds change.

**Status:** 3 for an invalid handle, otherwise 0.

```basic
blReqHandle% = ButtonH%
blReqX% = 20 : blReqY% = 20 : blReqW% = 200 : blReqH% = 32
GOSUB blApiSetBounds
GOSUB blApiPaintAll
```

#### `blApiSetLayout` — configure a panel

**Line:** 510900  
**Inputs:** `blReqHandle%`, `blReqMode%`, `blReqPad%`, `blReqGap%`  
**Outputs:** `blStatus%`

Valid targets are `blClassPanel%` and `blClassTop%`. `blReqCols%` is not read by
this routine (use `blApiCreate` if you need to store it).

Modes:

| Mode | Meaning |
|---:|---|
| 0 | Absolute: child X/Y are relative to the panel's outer corner; size unchanged |
| 1 | Vertical stack: children at `X = Pad`, `Y` advancing by `H + Gap`, width `clientW` |
| 2 | Horizontal row: children at `Y = Pad`, `X` advancing by `W + Gap`, height `clientH` |
| 3 | Stored, but layout falls through to absolute because there is no grid branch |

The mode is **not validated**. Any value other than 1 or 2 behaves as absolute.

**Status:** 3 for an invalid handle, 4 if the target is not a Top or Panel
control, otherwise 0.

```basic
blReqHandle% = PanelH%
blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
GOSUB blApiSetLayout
GOSUB blApiPaintAll
```

#### `blApiLayoutAll` — recompute absolute rectangles

**Line:** 511200  
**Inputs:** none  
**Outputs:** `blStatus% = 0`

Walks the tree from the root, asks each container to reposition its children,
computes absolute positions and cached clip rectangles, and clears
`blNeedsLayout%`. It does not paint.

Call it directly when you need positions for hit testing or diagnostics, or let
`blApiPaintAll` do it automatically.

```basic
GOSUB blApiLayoutAll
PRINT "button at"; blCtl%(ButtonH%,11); blCtl%(ButtonH%,12)
```

#### `blApiAutoSize` — size a user control to its text

**Line:** 511910  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

For a class-3 control, sets:

```text
W = TEXTWIDTH(text$, textScale) + 2*Pad + 2
H = TEXTHEIGHT(text$, textScale) + 2*Pad + 2
```

and marks layout and paint dirty. It does not paint.

- Only class 3 (User) is accepted; other classes return status 10.
- There is no auto-size-on-text-change in the current build. Call this
  explicitly after `blApiSetText` if you want the control to grow.
- `blReqAutoSize%` is ignored by `blApiCreate`.

**Status:** 3 for an invalid handle, 10 if the target is not class 3, otherwise
0.

```basic
blReqHandle% = LabelH%
blReqText$ = "A longer caption"
GOSUB blApiSetText
GOSUB blApiAutoSize
GOSUB blApiPaintAll
```

---

### 7.4 Visibility, enabled state, and invalidation

#### `blApiShow` — make visible

**Line:** 510400  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

Sets the visible flag, marks layout and paint dirty. Showing a control does not
change its z-order or focus. **Status:** 3 for an invalid handle, otherwise 0.

#### `blApiHide` — hide

**Line:** 510500  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

Clears the visible flag, clears `blCapture%` if this control owned it, and marks
layout and paint dirty. A hidden control is skipped by layout, paint, and hit
test. Hiding a parent does not clear the hover handle of a child; poll again
after hiding. **Status:** 3 for an invalid handle, otherwise 0.

#### `blApiSetEnabled` — enable or disable

**Line:** 511950  
**Inputs:** `blReqHandle%`, `blReqEnabled%` (non-zero = enabled)  
**Outputs:** `blStatus%`

Sets or clears the enabled flag and marks paint dirty. Disabled controls are
skipped by hit test. The current build does not render a disabled appearance.
A disabled parent does not disable an enabled child. **Status:** 3 for an
invalid handle, otherwise 0.

```basic
blReqHandle% = ButtonH%
blReqEnabled% = 0
GOSUB blApiSetEnabled
```

#### `blApiInvalidate` — request a repaint

**Line:** 511100  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

Sets `blNeedsPaint% = 1`. It does not paint and does not set a per-control dirty
flag. Use `blApiUpdate` for a targeted repaint or `blApiPaintAll` for the frame.
**Status:** 3 for an invalid handle, otherwise 0.

```basic
blReqHandle% = LabelH%
GOSUB blApiInvalidate
GOSUB blApiPaintAll
```

---

### 7.5 Text and values

#### `blApiSetText` — set caption or label text

**Line:** 510600  
**Inputs:** `blReqHandle%`, `blReqText$`  
**Outputs:** `blStatus%`

Stores the text and sets `blNeedsPaint%`. It does not resize the control and
does not truncate to 255 characters (newbasic strings are already limited to
255). Panels store text but do not paint it. **Status:** 3 for an invalid
handle, otherwise 0.

```basic
blReqHandle% = ClockH%
blReqText$ = "Clock: " + TIME$
GOSUB blApiSetText
GOSUB blApiPaintAll
```

#### `blApiSetValue` — set a user-control value

**Line:** 510700  
**Inputs:** `blReqHandle%`, `blReqValue#`  
**Outputs:** `blStatus%`

- Requires class 3 (User); other classes return 10.
- Clamps to the control's `Min`/`Max` fields (default 0..100).
- Does not quantize to `Step` (the field exists but is not used).
- Does not restrict itself to sliders/gauges/dials; buttons and labels are also
  class 3 and will accept a value.
- Sets `blNeedsPaint%`; it does not paint.
- **Status:** 3 for an invalid handle, 10 for a non-User class, otherwise 0.

```basic
blReqHandle% = SliderH%
blReqValue# = 42
GOSUB blApiSetValue
GOSUB blApiPaintAll
```

#### `blApiGetValue` — read a user-control value

**Line:** 510800  
**Inputs:** `blReqHandle%`  
**Outputs:** `blRetValue#`, `blStatus%`

Requires class 3. Returns the stored value as a double. The value is the source
control's current value; a gauge is read-only only because click handling does
not change it. **Status:** 3 for an invalid handle, 10 for a non-User class,
otherwise 0.

```basic
blReqHandle% = SliderH%
GOSUB blApiGetValue
IF blStatus% = blStatusOk% THEN PRINT "value"; blRetValue#
```

---

### 7.6 Actions

#### `blApiSetAction` — change a control's action ID

**Line:** 511000  
**Inputs:** `blReqHandle%`, `blReqAction%`  
**Outputs:** `blStatus%`

Stores the action ID in the control's common row. Action 0 clears the action.
IDs 0-255 are the intended range because the action-line table has 256 entries;
the API does not range-check, so use `blActionNone%`/0-255 deliberately.

Click delivery bubbles from the hit control to the nearest ancestor with a
non-zero action. Setting the root's action therefore catches every click that
no nearer control handles. **Status:** 3 for an invalid handle, otherwise 0.

```basic
blReqHandle% = TargetH%
blReqAction% = ActTarget%
GOSUB blApiSetAction
```

#### `blApiSetActionLine` — map an action ID to a host line

**Line:** 511920  
**Inputs:** `blReqActionId%` (0-255), `blReqLine&` (0-2147483647)  
**Outputs:** `blStatus%`

Stores a **numeric line**. A stored value of `blNoLine&` (0) means "no
handler". The library never calls the line; the host does the computed `GOSUB`
after reading an event.

**Status:** 10 if the action ID is outside 0-255 or the line is negative,
otherwise 0.

```basic
blReqActionId% = 100 : blReqLine& = 2010 : GOSUB blApiSetActionLine
blReqActionId% = 200 : blReqLine& = 2110 : GOSUB blApiSetActionLine
```

Dispatch pattern:

```basic
GOSUB blApiNextEvent
IF blEventAction% <> blActionNone% THEN GOSUB DispatchAction

7300 LABEL DispatchAction
7310 IF blEventAction% = blActionNone% THEN RETURN
7320 ActLine& = blActionLine&(blEventAction%)
7330 IF ActLine& = blNoLine& THEN RETURN
7340 GOSUB ActLine&
7350 RETURN
```

Always guard `blNoLine&`; action 0 or an unregistered ID would otherwise
`GOSUB 0`.

---

### 7.7 Painting and the event loop

#### `blApiPaintAll` — paint the whole tree

**Line:** 511300  
**Inputs:** none  
**Outputs:** `blStatus% = 0`

1. Runs `blLayoutAll` if `blNeedsLayout%` is set.
2. Clears `blNeedsPaint%`.
3. Paints the root, then every visible child in sibling order (back to front).

The current paint pass uses absolute canvas coordinates and wraps each control
in `PUSH VIEW`/`POP VIEW` but does not set a per-control `VIEW`. It therefore
preserves a host viewport but does not rebase coordinates or clip children to
their parent. Use the full canvas as the viewport while baslib is painting.

```basic
GOSUB blApiPaintAll
IF blStatus% <> blStatusOk% THEN PRINT "paint"; blStatus%
```

#### `blApiUpdate` — repaint one subtree

**Line:** 511400  
**Inputs:** `blReqHandle%`  
**Outputs:** `blStatus%`

Repaints the control and its descendants starting at the handle. It does not
clear the parent's background and does not clear dirty flags. Use it for a text
or value change whose size and position did not change; use `blApiPaintAll`
after a move, resize, show/hide, auto-size, or any text-length change that could
leave ghost pixels. **Status:** 3 for an invalid handle, otherwise 0.

```basic
blReqHandle% = ClockH%
GOSUB blApiUpdate
```

#### `blApiPollInput` — poll the mouse and dispatch clicks

**Line:** 511500  
**Inputs:** none  
**Outputs:** `blRetN%` (1 if a click was consumed), `blStatus% = 0`

Order inside the call:

1. `MOUSEB` — consume at most one queued click.
2. `MOUSEPOSX`/`MOUSEPOSY` — if the pointer is over the canvas, run the live
   move/hover path.
3. `MOUSEBUTTONS AND 1` — track only the left button for live down/up and
   capture.
4. `MOUSEWHEEL` — post a wheel event if non-zero.
5. If a click was consumed, read `MOUSEX`/`MOUSEY` and run `blClickAt`.

Consequences:

- Hover, enter/leave, move, down, up, and wheel events are queued before the
  click event from the same poll.
- A slider or dial click queues a `blEvChange%` event carrying the control's
  own action (0 if none). If the click resolves to a non-zero action, a
  `blEvClick%` event follows. Make action handlers idempotent.
- The live path tracks only the left button. Right and middle clicks still
  arrive through `MOUSEB` and can fire actions.
- The current build posts a `blEvMove%` event on every poll while a control is
  hovered, even if the pointer did not move. Drain events every frame.
- The pointer leaving the canvas (`MOUSEPOSX < 0`) is not converted into a
  leave event; hover is left as it was.

```basic
GOSUB blApiPollInput
WHILE blEventCount% > 0
  GOSUB blApiNextEvent
  GOSUB DispatchAction
WEND
```

#### `blApiPollMouse` — alias

**Line:** 511550  
**Inputs:** none  
**Outputs:** same as `blApiPollInput`

This is exactly the same poll; use it if your source reads better with the
"mouse" name.

#### `blApiNextEvent` — pop one event

**Line:** 511600  
**Inputs:** none  
**Outputs:** `blEventType%`, `blEventCtl%`, `blEventAction%`, `blEventX%`,
`blEventY%`, `blEventButton%`, `blEventWheel%`, `blEventCount%`, `blStatus%`

- Non-empty queue: fills the fields, removes the event, status 0.
- Empty queue: status 13 (`blStatusEventQueueEmpty%`), event type 0, all event
  fields zeroed, count 0.

Always loop on `blEventCount%` rather than calling into an empty queue.

```basic
WHILE blEventCount% > 0
  GOSUB blApiNextEvent
  IF blEventType% = blEvClick% THEN PRINT "click on"; blEventCtl%
WEND
```

#### `blApiFindAt` — hit test an absolute point

**Line:** 511700  
**Inputs:** `blReqX%`, `blReqY%` (absolute canvas coordinates)  
**Outputs:** `blRetHandle%` (0 if none), `blStatus% = 0`

Hit-test rules in the current build:

- scans handles 255 down to 1;
- requires allocated class, visible flag, and enabled flag;
- tests the absolute rectangle only;
- ignores cached clips, transparent flags, and disabled ancestors;
- returns the root if the point is inside the root and no child wins.

Because the root covers the canvas, `blRetHandle%` is normally 1 rather than 0
for a point on empty background.

```basic
blReqX% = 200 : blReqY% = 120
GOSUB blApiFindAt
PRINT "hit"; blRetHandle%
```

---

### 7.8 Synthetic input (test hooks)

The test hooks run the same internal paths as real input. They are the reliable
way to test a baslib UI without a mouse.

#### `blApiTestClickAt` — click at an absolute point

**Line:** 511800  
**Inputs:** `blReqX%`, `blReqY%`, `blReqButton%` (1 left, 2 right, 3 middle)  
**Outputs:** `blRetHandle%` (hit or action owner), `blRetN%` (set to 1 when a
control handled the click), `blStatus% = 0`

Performs hit test, value mapping for slider/dial, action bubbling, and event
posting. It does **not** update hover or capture, and it does not enforce a
button mask.

```basic
blReqX% = 360 : blReqY% = 186 : blReqButton% = 1
GOSUB blApiTestClickAt
GOSUB blApiNextEvent
```

#### `blApiTestMouseMove` — move the live pointer

**Line:** 511810  
**Inputs:** `blReqX%`, `blReqY%`  
**Outputs:** `blStatus% = 0`

Updates `blHover%`, posts leave/enter events on a hover change, posts a move
event when hovering a control, and updates a captured slider/dial value if the
left button is down.

```basic
blReqX% = 200 : blReqY% = 120
GOSUB blApiTestMouseMove
```

#### `blApiTestMouseButton` — press or release

**Line:** 511820  
**Inputs:** `blReqX%`, `blReqY%`, `blReqButton%`, `blReqDown%` (1 press, 0 release)  
**Outputs:** `blStatus% = 0`

Uses the **current hover** to choose the target; call `blApiTestMouseMove`
first. On press it sets `blPressed%` and, if hovering, sets `blCapture%`. On
release it posts the up event to the captured control and clears both states.

```basic
blReqX% = 200 : blReqY% = 120 : GOSUB blApiTestMouseMove
blReqButton% = 1 : blReqDown% = 1 : GOSUB blApiTestMouseButton
blReqX% = 260 : GOSUB blApiTestMouseMove
blReqButton% = 1 : blReqDown% = 0 : GOSUB blApiTestMouseButton
```

#### `blApiTestWheel` — inject wheel movement

**Line:** 511830  
**Inputs:** `blReqX%`, `blReqY%`, `blReqWheel%`  
**Outputs:** `blStatus% = 0`

Posts a wheel event only if the current hover is non-zero. Call
`blApiTestMouseMove` first.

```basic
blReqX% = 200 : blReqY% = 120 : blReqWheel% = 3
GOSUB blApiTestWheel
```

---

### 7.9 Debug and extension

#### `blApiDump` — print the control table

**Line:** 511900  
**Inputs:** none (the plan's optional handle filter is not implemented)  
**Outputs:** `blStatus% = 0`; text on the current PRINT device

For every allocated control, prints one line:

```text
H<handle> C<class> P<parent> X<absX> Y<absY> W<width> H<height>
```

This is a debugging aid. In a graphics program the output goes to the terminal
or text surface, not into a control.

```basic
GOSUB blApiDump
```

#### `blApiRegisterClass` — install a host method-table row

**Line:** 511930  
**Inputs:** `blReqClass%` (5-16), `blReqLine&` (positive), `blReqText$` (name)  
**Outputs:** `blStatus%`

Fills every verb slot of the class row with the same line and stores the class
name. **Status:** 4 if the class is outside 5-16, 10 if the line is not
positive, otherwise 0.

Important: the public API does **not** dispatch class methods. Paint, hit test,
and click handling are implemented by numeric class/kind checks in the core,
not by `blMethod&`. Registering a class therefore has no visible effect on
normal host code. It is useful only if the host itself calls the internal
`blDispatch`/`blSend` protocol. See §15.

```basic
blReqClass% = 5 : blReqLine& = 500990 : blReqText$ = "Extra"
GOSUB blApiRegisterClass
```

---

## 8. Control reference

### 8.1 Top (root) — class 1

Created only by `blApiInit`, always at handle 1. There is exactly one root.

| Property | Current behavior |
|---|---|
| Bounds | `0, 0, rootW, rootH`; `rootW/H` from request or canvas |
| Flags | visible + enabled + clip |
| Layout | no automatic child positioning (mode 0); children keep local X/Y |
| Paint | fill with panel face 7, border 8 |
| Hit test | the root rectangle matches; it is the fallback for empty space |
| Action | 0 by default; `blApiSetAction` can install a desktop action |

If you set a root action, every click that no nearer control handles will bubble
to the root and fire that action. That is a convenient way to implement
"clicked the background" behavior, but it also means stray clicks are no longer
silent.

The root's panel fields are initialized to mode 0, pad 0, gap 0, face 7,
border 8. `blApiSetLayout` accepts the root, so you can give root children a
vertical or horizontal arrangement.

### 8.2 Panel — class 2

A panel is a container with a background, a one-pixel border, optional padding,
and a child layout mode. It does not paint a title even if `blReqText$` is set.

Layout modes:

| Mode | Placement of each visible child |
|---:|---|
| 0 Absolute | `child.AbsX = panel.AbsX + child.X`, `child.AbsY = panel.AbsY + child.Y`; width/height unchanged; padding ignored |
| 1 Vertical | `child.X = Pad`; `child.Y` starts at `Pad` and advances by `child.H + Gap`; `child.W = panel.W - 2*Pad`; height unchanged |
| 2 Horizontal | `child.Y = Pad`; `child.X` starts at `Pad` and advances by `child.W + Gap`; `child.H = panel.H - 2*Pad`; width unchanged |
| 3 Grid | Stored in `blPanel%(...,1)` but layout falls through to absolute |

Important details:

- Only **hidden** children are skipped. A zero-height child still advances the
  vertical cursor and consumes a gap.
- The panel border is drawn when `blPanel%(h,7) >= 0`; default 8. Set it to -1
  to remove the border.
- The panel face is drawn unconditionally with `blPanel%(h,6)`; default 7.
- Padding applies inside the panel's outer rectangle, not inside the border
  separately.
- **Nested panels are unsafe in the current build** unless the nested panel is
  the last child of its parent. The layout routine stores its mode, padding, gap,
  and running cursor in shared globals; a nested layout overwrites them, so the
  parent's remaining children are placed with the nested panel's settings. See
  §9.2 and §15. The sample host uses one flat panel for this reason.

```basic
GOSUB ResetReq
blReqClass% = blClassPanel% : blReqParent% = blRoot%
blReqX% = 40 : blReqY% = 24 : blReqW% = 640 : blReqH% = 440
blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
GOSUB blApiCreate
PanelH% = blRetHandle%
```

### 8.3 Button — user kind 1

A beveled-looking rectangle with a centered caption. Clicking it posts the
control's action.

| Behavior | Current implementation |
|---|---|
| Paint | fill face (default 7), draw border (default 0), centered one-line text at scale 1, padding 2 |
| Pressed visual | The pressed branch reads `blFlagPressed%`, but input never sets that flag, so there is no visible press state |
| Hover visual | None |
| Click | Any mouse button can fire the action; there is no button-mask check |
| Disabled visual | None; disabled only blocks hit testing |
| Keyboard | No Space/Enter activation; handle keys in host code |
| Auto-size | `blApiAutoSize` works (class 3) |
| Text | Truncated to the control width by pixel measurement |

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindButton%
blReqX% = 0 : blReqY% = 0 : blReqW% = 180 : blReqH% = 28
blReqText$ = "Click me" : blReqAction% = 100 : blReqPad% = 6
GOSUB blApiCreate
ButtonH% = blRetHandle%
```

### 8.4 Label — user kind 5

Static multiline text. Labels are the workhorse for status, clocks, headings,
and value readouts.

| Behavior | Current implementation |
|---|---|
| Paint | optional background fill (transparent by default), optional border, text |
| Alignment | default horizontal center, vertical middle |
| Multiline | yes: `CHR$(13)`, `CHR$(10)`, and CRLF start a new line |
| Truncation | each line is trimmed character-by-character until it fits `W - 2*Pad` |
| Word wrap | field 16 exists, but `blCreate` always writes 0 and `blReqWrap%` is ignored |
| Border | drawn when `blUser#(h,17) >= 0`; default 0, so a border is drawn |
| Background | transparent by default (`blUser#(h,18) = 1`) |
| Click-through | **no**; labels participate in hit testing |
| Action | a label can have an action and will fire it when clicked |
| Auto-size | explicit `blApiAutoSize`; no automatic resize on `blApiSetText` |
| Value | labels accept `blApiSetValue` because they are class 3, but nothing uses it |

Because labels are transparent by default, updating a label to a shorter string
and calling `blApiUpdate` can leave ghost glyphs. Call `blApiPaintAll` after a
text change unless you know the new text occupies every old glyph cell. The
sample clock uses a fixed-width `TIME$`, but still repaints the frame.

All labels are center/middle by default. To left-align or shrink the border,
write the user table directly (advanced; see §8.9):

```basic
blUser#(LabelH%,14) = 0        ' horizontal: 0 left, 1 center, 2 right
blUser#(LabelH%,15) = 0        ' vertical:   0 top, 1 middle, 2 bottom
blUser#(LabelH%,17) = -1       ' no border
blUser#(LabelH%,19) = 1        ' text palette index
blUser#(LabelH%,20) = 2        ' text scale 1-8
blNeedsPaint% = 1
GOSUB blApiPaintAll
```

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindLabel%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 60
blReqText$ = "Ready" + CHR$(13) + "second line" : blReqPad% = 4
GOSUB blApiCreate
LabelH% = blRetHandle%
```

### 8.5 Slider — user kind 2

A horizontal value control. Default range 0-100, initial value from
`blReqValue#`.

Behavior:

- Paint: a horizontal track line at mid-height plus an accent-colored thumb.
  There is no background fill or border.
- Click: maps the pointer's **X** position within the control rectangle to a
  value: `min + (x - AbsX) * (max - min) / W`, clamped.
- Drag: while the left button is held and the slider owns capture, a move
  updates the value the same way.
- Event: a value change posts `blEvChange%` with the slider's action (0 if
  none); if the click resolves to a non-zero action, `blEvClick%` follows.
- Step/orientation: fields exist but are not used; quantization and vertical
  sliders are not implemented.
- Range: there is no public setter for min/max; defaults are 0/100. Change
  `blUser#(h,3)` and `blUser#(h,4)` directly if you need another range.

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindSlider%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
blReqValue# = 25 : blReqAction% = 150 : blReqPad% = 6
GOSUB blApiCreate
SliderH% = blRetHandle%
```

### 8.6 Gauge — user kind 3

A read-only bar display. Default range 0-100.

- Paint: fills the face (default 7), then draws an accent bar from the left edge
  proportional to the value.
- Click: does not change the value.
- Action: if you assign one, a click still posts a click event (the core action
  path is kind-independent), so leave the action at 0 for a true read-only
  display.
- There is no value text; draw a separate label if you want a numeric readout.

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindGauge%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
blReqValue# = 60 : blReqPad% = 6
GOSUB blApiCreate
GaugeH% = blRetHandle%
```

### 8.7 Dial — user kind 4

A circular display with a needle.

Behavior:

- Paint: circle in the face color, inner circle in the accent color, needle at
  `angle = pi * (1 - (value - min) / (max - min))`. At minimum the needle points
  left; at maximum it points right; the middle points up.
- Click/drag: maps the pointer's **X** position linearly to the value, exactly
  like a slider. It does **not** compute an angle from the pointer. This is a
  known limitation of the current build.
- Event: change (dial's action, possibly 0), then a click if the dial or an
  ancestor resolves a non-zero action.
- Size: the circle radius is `min(W, H)/2 - 2`, so a wide, short control
  produces a small dial centered in the rectangle.

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindDial%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 96
blReqValue# = 35 : blReqAction% = 160 : blReqPad% = 6
GOSUB blApiCreate
DialH% = blRetHandle%
```

### 8.8 Click target — class 4

An invisible rectangle used as a hotspot. It paints nothing.

- Hit test: the rectangle, subject to the usual visible/enabled rules.
- Click: posts its action when clicked, with no visual feedback.
- Button mask: `blReqButtonMask%` is stored in `blClick%(h,1)` (1 left, 2 right,
  4 middle) but the current click path does not test it. Any button fires the
  action.
- Capture/hover: the stored `blClick%(h,2..4)` fields are not maintained.
- Useful as an overlay hotspot or as the minimal control for testing the event
  pipeline.

```basic
GOSUB ResetReq
blReqClass% = blClassClickTarget% : blReqParent% = PanelH%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 40
blReqAction% = 200 : blReqButtonMask% = 1
GOSUB blApiCreate
TargetH% = blRetHandle%
```

### 8.9 Class-specific defaults

These are the values written by `blApiCreate` in the current build. Request
globals not listed here are ignored.

**Common row** (`blCtl%`):

| Field | Value after create |
|---:|---|
| 1 class | request |
| 2 parent | request |
| 3/4 first/last child | 0 |
| 5/6 prev/next sibling | linked |
| 7/8 local X/Y | request |
| 9/10 W/H | request |
| 11/12 absolute X/Y | request initially; layout overwrites |
| 13 flags | `blReqFlags% OR visible OR enabled`; panels add clip |
| 14 state | 0 (unused) |
| 15 action | request |
| 16 tag | request |
| 17 capture | 0 (unused; capture is global) |

**Panel row** (`blPanel%`):

| Field | Value |
|---:|---|
| 1 mode | `blReqMode%` |
| 2 pad | `blReqPad%` |
| 3 gap | `blReqGap%` |
| 4 cols | `blReqCols%` |
| 5 reserved | 0 |
| 6 background | 7 |
| 7 border | 8 |
| 8 reserved | 0 |

**User row** (`blUser#`):

| Field | Meaning | Default |
|---:|---|---:|
| 1 | kind | `blReqKind%` |
| 2 | value | `blReqValue#` |
| 3 | min | 0 |
| 4 | max | 100 |
| 5 | step | 1 (unused) |
| 6 | orientation | 0 (unused) |
| 7 | style | 0 |
| 8 | face | 7 |
| 9 | accent | 4 |
| 10 | drag state | 0 |
| 11 | decimals | 0 |
| 12 | reserved | 0 |
| 13 | text padding | `blReqPad%` |
| 14 | horizontal align | 1 (center) |
| 15 | vertical align | 1 (middle) |
| 16 | wrap | 0 |
| 17 | border | 0 (drawn; set -1 to remove) |
| 18 | transparent | 1 |
| 19 | text color | 0 |
| 20 | text scale | 1 |
| 21 | auto-size flag (unused) | 0 |
| 22 | line count (unused; stays 1) | 1 |

**Click row** (`blClick%`):

| Field | Value |
|---:|---|
| 1 button mask | `blReqButtonMask%` (stored only) |
| 2 hot | 0 |
| 3 pressed | 0 |
| 4 wheel | 0 |

The direct-table route is the supported escape hatch for visuals that the
current `blCreate` does not copy from request globals. After changing any visual
field, set `blNeedsPaint% = 1` and call `blApiPaintAll`.

---

## 9. Layout and painting

### 9.1 Coordinate flow

1. `blApiCreate` stores local X/Y/W/H.
2. `blApiLayoutAll` walks from the root:
   - the root's absolute origin is `(0,0)`;
   - each container positions its children's local rectangles;
   - each child gets `AbsX/AbsY` from the parent origin plus local X/Y;
   - a cached clip rectangle is computed from the intersection of the parent
     clip and the child rectangle.
3. `blApiPaintAll` lays out if `blNeedsLayout%` is set, then paints in tree order.
4. `blApiFindAt` and click dispatch use the cached absolute rectangles.

The clip rectangles are computed but the current paint pass does not apply them
per control. Do not rely on a child being clipped by its parent.

### 9.2 Layout modes in practice

Vertical mode:

```text
clientW = panel.W - 2*Pad
y = panel.AbsY + Pad
for each visible child:
    child.X = Pad
    child.Y = (y - panel.AbsY)
    child.W = clientW
    y = y + child.H + Gap
```

Horizontal mode:

```text
clientH = panel.H - 2*Pad
x = panel.AbsX + Pad
for each visible child:
    child.X = (x - panel.AbsX)
    child.Y = Pad
    child.H = clientH
    x = x + child.W + Gap
```

Absolute mode uses the child's stored local X/Y and does not modify width or
height. Hidden children are skipped entirely; zero-size children are not.

**Nested-container defect.** `blLayoutChildren` keeps `blMode%`, `blPad%`,
`blGap%`, `blPX%`, `blPY%`, `blCW%`, `blCH%`, `blCX%`, and `blCY%` in shared
globals. `blChild%` is saved across the recursive call, but the layout settings
are not. After a nested panel lays out, the outer panel's remaining children use
the nested mode/pad/gap and the wrong cursor. The visible symptom is that
siblings after the nested panel overlap it or are arranged as a row.

Workarounds in the current build:

- use a **flat** panel (the sample host does this);
- make the nested container the **last** child of its parent;
- or bypass container layout for that area with absolute coordinates and no
  deeper panels.

### 9.3 Paint order, viewport, and clipping

- Paint order is parent first, then children in creation order (back to front).
- `blApiCreate` appends, so a later control paints over an earlier one.
- Each control's paint is wrapped in `PUSH VIEW`/`POP VIEW`.
- The paint pass does **not** issue a `VIEW` for the control. Controls draw at
  absolute canvas coordinates.
- A host viewport is preserved, but it is not rebased. If the host sets
  `VIEW (100,100)-(500,300)` before painting, baslib's absolute drawing is
  interpreted relative to that viewport and controls appear offset. Run with the
  default full-screen viewport while baslib is painting.
- There is no per-control clipping. A child can draw outside its parent.
- `blCtlZ%` is set to the handle at create and is not maintained as a paint
  order. Hit testing uses the reverse-handle scan, not `blCtlZ%`.

### 9.4 Text measurement and drawing

Text helpers use the newbasic metrics:

- `TEXTWIDTH(text$, scale)` — widest line in pixels, `8 * scale` per character.
- `TEXTHEIGHT(text$, scale)` — total height, `16 * scale` per line.
- `DRAWTEXT (x, y), text$, colour, scale` — CP437 glyphs at an absolute
  graphics anchor, with CR/LF line breaks and transparent ink.

`blDrawText` truncates each line to `W - 2*Pad` by removing characters from the
right until it fits. It then centers or aligns the resulting block and draws it.
Truncation is per character, not per word. Multiline text is explicitly
supported; word wrapping is not reachable through the public API in this build.

A label's background is transparent by default. The panel behind it shows
through, and `DRAWTEXT` only writes ink pixels. If the text gets shorter, old
ink can remain unless the frame is repainted. Prefer `blApiPaintAll` after text
changes.

### 9.5 Repaint strategy

| Situation | Recommended call |
|---|---|
| First frame after creating controls | `blApiPaintAll` |
| Label text changed, same size and position | `blApiUpdate` on the label, or `blApiPaintAll` |
| Button/slider/gauge/dial value changed | `blApiUpdate` on that control, or `blApiPaintAll` |
| Control moved/resized/shown/hidden/auto-sized | `blApiPaintAll` |
| Many controls changed | `blApiPaintAll` |
| Timer tick updating one label | `blApiUpdate` is enough if the text length is stable; `blApiPaintAll` is safest |
| Hover/press visuals | Not implemented; no repaint needed |

`blApiPaintAll` clears `blNeedsPaint%`; `blApiUpdate` does not. A host loop that
checks `blNeedsPaint%` after an update may repaint the frame again. That is
harmless but wasteful; either clear the flag yourself or use `blApiPaintAll`
consistently.

---

## 10. Input, events, and actions

### 10.1 Mouse functions used by baslib

| Function | Meaning | Consumes? |
|---|---|---|
| `MOUSEB` | queued click button: 1 left, 2 right, 3 middle, 0 none | yes, one click |
| `MOUSEX`, `MOUSEY` | coordinates latched to the most recent consumed click | no |
| `MOUSEPOSX`, `MOUSEPOSY` | live pointer position; -1 outside the canvas | no |
| `MOUSEBUTTONS` | live bit mask: 1 left, 2 right, 4 middle | no |
| `MOUSEWHEEL` | accumulated wheel steps | yes, resets to 0 |

`blApiPollInput` reads these once per call. The host may still read them for its
own purposes, but remember that `MOUSEB` and `MOUSEWHEEL` consume.

### 10.2 Poll order and event order

For one `blApiPollInput`:

1. consume a queued click with `MOUSEB`;
2. read the live position; if over the canvas, run hover/move processing;
3. read the live left button; generate a down or up transition;
4. read the wheel; generate a wheel event;
5. if a click was consumed, run click delivery.

Events posted during the call are queued in that order. The click event is last.
A slider/dial click with a non-zero action produces:

```text
blEvChange%   (value updated; carries the control's action, possibly 0)
blEvClick%    (carries the resolved action, possibly an ancestor's)
```

In the sample the slider/dial actions are non-zero, so both events carry the
same ID and the handler runs twice. An action handler that refreshes a readout
is naturally idempotent; a handler that increments a counter twice is not. Check
`blEventType%` if you need to distinguish them.

### 10.3 Hit testing

Current algorithm:

```text
for handle = 255 down to 1:
    if allocated and visible and enabled:
        if point is inside (AbsX, AbsY, W, H):
            return handle
return 0
```

Consequences:

- The front-most control is approximated by the highest handle, not by sibling
  order explicitly. For normal creation order this matches paint order.
- Cached clips are ignored.
- A disabled parent does not exclude its children.
- A label is not click-through. It can block a button underneath it.
- The root always matches a point inside the root, so `blApiFindAt` normally
  returns 1 for empty background.
- The hit test is O(255) per call. That is trivial at this scale.

### 10.4 Click delivery and bubbling

`blClickAt(x, y, button)`:

1. hit-test the point;
2. if the target is a user control, map the pointer X to a slider/dial value
   (posting a change event);
3. read the target's action; if it is 0, walk up the parent chain to the nearest
   ancestor with a non-zero action;
4. if an action was found, post a click event with that action and the action
   owner as `blEventCtl%`;
5. if no action was found, post nothing.

The in-between control does not need its own handler. A panel or root action
catches clicks that bubble out of a leaf. The current implementation does not
dispatch a per-class `Click` method; the action field and bubbling are the whole
click path.

`button` is recorded in `blEventButton%`, but it does not filter delivery. A
button or click target fires on left, right, or middle clicks.

### 10.5 Event queue

- Fixed ring of 64 events.
- FIFO order.
- No coalescing. In particular, a move event is posted on every poll while the
  pointer is over a control.
- When the ring is full, `blPostEvent` sets `blStatusEventQueueFull%` (12) and
  drops the new event. `blApiPollInput` then sets `blStatus% = 0`, so the host
  may not see the error. Drain every frame.
- `blApiNextEvent` on an empty queue sets status 13 and zeros the event fields.
- `blEventCount%` mirrors the live count.

Event types are listed in §6.4. The library does not generate `blEvKey%`,
`blEvFocus%`, or `blEvCaptureLost%` in the current build.

### 10.6 Action dispatch

The library stores an action ID on the control, copies it to the event, and
stops. The host owns dispatch. The action-line table is the intended mechanism:

```basic
' setup
blReqActionId% = 100 : blReqLine& = 2010 : GOSUB blApiSetActionLine

' loop
WHILE blEventCount% > 0
  GOSUB blApiNextEvent
  IF blEventAction% <> blActionNone% THEN GOSUB DispatchAction
WEND

7300 LABEL DispatchAction
7310 IF blEventAction% = blActionNone% THEN RETURN
7320 ActLine& = blActionLine&(blEventAction%)
7330 IF ActLine& = blNoLine& THEN RETURN
7340 GOSUB ActLine&
7350 RETURN
```

Rules:

- Action 0 is "no action" by convention; use `blActionNone%`.
- Action IDs should be 0-255 because the table has 256 slots.
- The table stores numeric lines, not labels.
- The target line must contain an executable statement.
- Register action lines again after `blApiInit`.

You can also use a static `ON blEventAction% GOSUB ...` if your IDs are dense,
but the table avoids a giant selector and lets controls share handlers.

### 10.7 Keyboard

baslib has no keyboard focus, no key events, and no text entry. The host polls
`INKEY$` itself:

```basic
K$ = INKEY$
IF K$ = "" THEN <no key>
IF K$ = CHR$(27) THEN <quit>
IF K$ = "R" OR K$ = "r" THEN <reset>
```

`INKEY$` returns at most one character per call and does not block. Printable
keys and `Esc` are reliable. Arrow-key encodings depend on the backend; do not
build essential behavior on them. The `blVerbKey%` and `blEvKey%` slots are
reserved for a future revision.

### 10.8 Timers and the clock flag

Use the newbasic timer only to set a flag:

```basic
10 ON TIMER(1) GOSUB 6010
...
120 TIMER ON
...
6010 ClockFlag% = 1
6020 RETURN
```

At the top of the main loop:

```basic
1300 LABEL MainLoop
1310 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
...
```

`UpdateClock` calls `blApiSetText` and repaints. Why the flag?

- The timer handler runs between statements, potentially in the middle of a
  baslib call.
- A baslib call uses shared scratch globals.
- Calling baslib from the handler can corrupt an in-progress operation.
- A single integer assignment is safe and deferred work happens in the loop.

`SLEEP` does not service timer events, so the clock may tick just after a sleep.
That is normal. Keep the sleep short (16 ms in the sample).

### 10.9 Test hooks

Synthetic input is the deterministic way to test a baslib UI. The order matters:

```basic
' 1. move so hover is set
blReqX% = 200 : blReqY% = 120
GOSUB blApiTestMouseMove

' 2. press
blReqButton% = 1 : blReqDown% = 1
GOSUB blApiTestMouseButton

' 3. drag
blReqX% = 260
GOSUB blApiTestMouseMove

' 4. release
blReqButton% = 1 : blReqDown% = 0
GOSUB blApiTestMouseButton

' 5. drain
WHILE blEventCount% > 0
  GOSUB blApiNextEvent
WEND
```

For a click without live state, `blApiTestClickAt` is enough. For a wheel,
move first so `blHover%` is non-zero.

---

## 11. How to use baslib

This section is the practical recipe. Section 12 applies it to the complete
sample.

### 11.1 Choose the screen and the host layout

Select a graphics screen before initialization:

```basic
SCREEN 720            ' 1280x720, 256-color palette
CLS
```

`SCREEN 720` is the recommended mode. Any graphics canvas works. Do not create
controls and then change `SCREEN`; reinitialize instead.

Decide whether the host uses the full canvas as its viewport. The current paint
pass assumes full-canvas coordinates. Do not leave a `VIEW` active unless you
have tested the offset.

### 11.2 Initialize

```basic
blReqW% = 0 : blReqH% = 0      ' 0 = use SCREENW/SCREENH
GOSUB blApiInit
IF blStatus% <> blStatusOk% THEN PRINT "init"; blStatus%: END
```

`blRoot%` is now 1. The root covers the canvas (or the requested size).

### 11.3 Define a request-reset helper

This is not optional for a multi-control program. Put it near the end of the
host source and call it before every create:

```basic
7000 LABEL ResetReq
7010 blReqClass% = 0 : blReqParent% = 0 : blReqHandle% = 0
7020 blReqX% = 0 : blReqY% = 0 : blReqW% = 0 : blReqH% = 0
7030 blReqFlags% = 0 : blReqAction% = 0 : blReqTag% = 0
7040 blReqKind% = 0 : blReqMode% = 0 : blReqPad% = 0 : blReqGap% = 0 : blReqCols% = 0
7050 blReqButtonMask% = 1 : blReqText$ = "" : blReqValue# = 0 : blReqEnabled% = 1
7060 RETURN
```

If you prefer a shorter reset, at minimum clear `blReqAction%`, `blReqText$`,
`blReqKind%`, `blReqButtonMask%`, `blReqMode%`, `blReqPad%`, `blReqGap%`, and
`blReqValue#`, because those leak most visibly.

### 11.4 Create a container

```basic
GOSUB ResetReq
blReqClass% = blClassPanel% : blReqParent% = blRoot%
blReqX% = 40 : blReqY% = 24 : blReqW% = 640 : blReqH% = 440
blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
GOSUB blApiCreate
IF blStatus% <> blStatusOk% THEN PRINT "panel"; blStatus%: END
PanelH% = blRetHandle%
```

A vertical panel with padding 12 and gap 8 is the easiest layout for a form: it
gives every child the client width and stacks them in creation order. Remember
that vertical mode overrides child width, and horizontal mode overrides child
height.

### 11.5 Create controls

Each create follows the same pattern:

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindLabel%
blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24
blReqText$ = "Ready" : blReqPad% = 4
GOSUB blApiCreate
IF blStatus% <> blStatusOk% THEN PRINT "label"; blStatus%: END
LabelH% = blRetHandle%
```

Order matters: children are appended, so the creation order is the paint order
and the reverse hit-test priority. Create a background label before a button
that must appear on top of it.

### 11.6 Paint the first frame

```basic
GOSUB blApiPaintAll
IF blStatus% <> blStatusOk% THEN PRINT "paint"; blStatus%: END
```

This also runs layout. After this call, `blCtl%(handle,11)` and
`(handle,12)` hold absolute coordinates, and `blApiFindAt` is meaningful.

### 11.7 Register action lines

```basic
blReqActionId% = ActButton% : blReqLine& = 2010 : GOSUB blApiSetActionLine
blReqActionId% = ActSlider% : blReqLine& = 2210 : GOSUB blApiSetActionLine
blReqActionId% = ActDial%   : blReqLine& = 2310 : GOSUB blApiSetActionLine
blReqActionId% = ActTarget% : blReqLine& = 2110 : GOSUB blApiSetActionLine
```

Point at an executable line. Add status checks if you register many lines.

### 11.8 Write the main loop

```basic
1300 LABEL MainLoop
1310 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
1320 GOSUB blApiPollInput
1330 WHILE blEventCount% > 0
1340   GOSUB blApiNextEvent
1350   GOSUB DispatchAction
1360 WEND
1370 K$ = INKEY$
1380 IF K$ <> "" THEN GOSUB HandleKey
1390 IF blNeedsPaint% <> 0 THEN GOSUB blApiPaintAll
1395 SLEEP 16
1398 GOTO MainLoop
```

Why this order?

- **Timer flag first:** deferred timer work happens before new input.
- **Poll input:** updates hover/press/capture and queues events.
- **Drain events:** processes clicks and value changes.
- **Keyboard:** host-only, non-blocking.
- **Paint once:** if anything marked the tree dirty.
- **Sleep:** yields CPU and lets the window pump messages.

The sample repaints inside its action handlers for simplicity. A larger
application may prefer to set a dirty flag in the handler and let the
once-per-frame paint at 1390 do the work. Do not issue a separate full paint for
every event in a burst.

### 11.9 Update content

Text:

```basic
blReqHandle% = LabelH%
blReqText$ = "New text"
GOSUB blApiSetText
GOSUB blApiPaintAll
```

Value:

```basic
blReqHandle% = SliderH%
blReqValue# = 42
GOSUB blApiSetValue
GOSUB blApiPaintAll
```

Read value:

```basic
blReqHandle% = SliderH%
GOSUB blApiGetValue
IF blStatus% = blStatusOk% THEN PRINT blRetValue#
```

### 11.10 Handle keyboard input

```basic
7400 LABEL HandleKey
7410 IF K$ = CHR$(27) THEN TIMER OFF : END
7420 IF K$ = "R" OR K$ = "r" THEN GOSUB ResetCounters
7430 IF K$ = "+" OR K$ = "=" THEN GOSUB NudgeUp
7440 IF K$ = "-" OR K$ = "_" THEN GOSUB NudgeDown
7460 RETURN
```

`INKEY$` returns one character. If the user holds a key, the host will see
repeats; that is acceptable for nudging a value.

### 11.11 Timer clock

```basic
120 ON TIMER(1) GOSUB 6010
...
1295 TIMER ON
...
6010 ClockFlag% = 1
6020 RETURN
...
7200 LABEL UpdateClock
7210 blReqHandle% = ClockH%
7220 blReqText$ = "Clock: " + TIME$
7230 GOSUB blApiSetText
7240 GOSUB blApiPaintAll
7250 RETURN
```

The handler only sets the flag. The loop clears the flag and updates the label.
`TIMER OFF` before ending the program.

### 11.12 Shut down and reinitialize

Normal exit:

```basic
TIMER OFF
END
```

After a re-run (`RUN`), a `LOAD`, a `NEW`, or `CLEAR`:

```basic
GOSUB blApiInit
' recreate controls, repaint, re-register action lines
```

Every old handle is invalid. Do not keep handles across a re-init.

### 11.13 Common recipes

**Create a hidden control:** create it, then hide it.

```basic
GOSUB ResetReq
blReqClass% = blClassUser% : blReqParent% = PanelH% : blReqKind% = blKindLabel%
blReqX% = 0 : blReqY% = 0 : blReqW% = 200 : blReqH% = 24
blReqText$ = "Hidden"
GOSUB blApiCreate : HiddenH% = blRetHandle%
blReqHandle% = HiddenH% : GOSUB blApiHide
```

**Show it again:**

```basic
blReqHandle% = HiddenH% : GOSUB blApiShow
GOSUB blApiPaintAll
```

**Disable a button:**

```basic
blReqHandle% = ButtonH% : blReqEnabled% = 0 : GOSUB blApiSetEnabled
```

**Change an action ID:**

```basic
blReqHandle% = ButtonH% : blReqAction% = 101 : GOSUB blApiSetAction
```

**Destroy a subtree:**

```basic
blReqHandle% = PanelH% : GOSUB blApiDestroy
GOSUB blApiPaintAll
```

Destroying the panel destroys all of its children and frees their handles.

**Make a custom-colored label:**

```basic
blUser#(LabelH%,19) = 1        ' text palette index
blUser#(LabelH%,20) = 2        ' text scale
blUser#(LabelH%,17) = -1       ' no border
blNeedsPaint% = 1
GOSUB blApiPaintAll
```

**Left-align all labels** (there is no request-global path in this build):

```basic
blUser#(LabelH%,14) = 0
blUser#(ClockH%,14) = 0
blUser#(StatusH%,14) = 0
blNeedsPaint% = 1
GOSUB blApiPaintAll
```

**Give a slider a different range:**

```basic
blUser#(SliderH%,3) = -50      ' min
blUser#(SliderH%,4) = 50       ' max
blUser#(SliderH%,2) = 0        ' current value, clamped by later SetValue
blReqHandle% = SliderH% : blReqValue# = 0 : GOSUB blApiSetValue
GOSUB blApiPaintAll
```

### 11.14 Host checklist

Before shipping a host:

- [ ] `SCREEN` selected before `blApiInit`.
- [ ] Every create preceded by a request reset.
- [ ] Every fallible API followed by a status check.
- [ ] Initial `blApiPaintAll` before the first poll.
- [ ] Action lines registered at executable lines.
- [ ] `blNoLine&` guard in action dispatch.
- [ ] Event queue drained every frame.
- [ ] Timer handler sets a flag and does not call baslib.
- [ ] `TIMER OFF` on exit.
- [ ] Repaint after text/geometry changes.
- [ ] No host names beginning with `bl`.
- [ ] No `OPTION BASE 1`.
- [ ] Host lines below 500000.
- [ ] Host `STACK` sentinel still passes.
- [ ] `Esc` and a normal exit path tested.

---

## 12. Tutorial: one of every control

This tutorial builds [`samplehost.bas`](samplehost.bas). The finished program
uses the root, a panel, a button, three labels, a slider, a gauge, a dial, and an
invisible click target. It has a mouse/keyboard control loop and a one-second
timer that updates an on-screen clock.

The complete listing is in §13. This section explains why each part exists.

### 12.1 The layout

One vertical panel holds everything. The panel is 640 x 480 at `(40, 24)`.
Padding is 12 and the gap is 8, so each child receives the 616-pixel client
width and is stacked in creation order.

```text
+------------------------------------------------------+  panel border
| baslib sample host - one of every control            |  header label
| Clock: 15:09:49                                      |  clock label
| Button clicks: 0                                     |  status label
| Target hits: 0                                       |
| Slider: 25   Dial: 35                                |
| Click me                                             |  button
| ====[]=============================                  |  slider
| [##########------------------------]                 |  gauge
|              ( dial )                                |  dial
| (invisible click target)                             |
+------------------------------------------------------+
```

The vertical panel is doing real layout work: all X/Y values are created as 0,
and the layout pass assigns each child its Y and width. The dial keeps its
created height (96) and gets the full client width; its circle is centered by
the paint code.

### 12.2 Constants and state

```basic
50 CONST ActButton% = 100
60 CONST ActSlider% = 150
70 CONST ActDial% = 160
80 CONST ActTarget% = 200
90 ButtonCount% = 0 : TargetCount% = 0 : ClockFlag% = 0
```

Action IDs are ordinary integers. 0 is reserved for "no action" by convention.
The three host counters/flags use plain names, not the `bl` namespace.

### 12.3 Screen, timer, and initialization

```basic
100 SCREEN 720
110 CLS
120 ON TIMER(1) GOSUB 6010
130 blReqW% = 0 : blReqH% = 0
140 GOSUB blApiInit
150 IF blStatus% <> blStatusOk% THEN GOTO 9000
```

The timer is armed but not enabled until the first frame is painted. That avoids
a clock update racing with setup. `blReqW%/H% = 0` means "use the canvas", which
is 1280x720 for `SCREEN 720`.

### 12.4 The panel and labels

The panel is created first. The header, clock, and status labels are direct
children, so vertical layout gives them full width and stacked Y positions.
Each create starts with `GOSUB ResetReq` so no action/text/mode leaks from the
previous control.

The clock label is initialized to the current `TIME$`; the timer will refresh it
every second:

```basic
430 blReqText$ = "Clock: " + TIME$ : blReqPad% = 4
```

The status label is three explicit lines joined with `CHR$(13)`:

```basic
530 blReqText$ = "Button clicks: 0" + CHR$(13) + "Target hits: 0" + CHR$(13) + "Slider: 25   Dial: 35"
```

`TEXTHEIGHT` sees three lines, so a height of 60 with padding 4 fits.

### 12.5 The button, slider, gauge, and dial

The button gets action 100. The slider starts at 25 and gets action 150. The
gauge starts at 60 with action 0, so it is read-only. The dial starts at 35 and
gets action 160.

Remember that vertical layout overrides width to the panel client width and
leaves height alone. That is why the created widths are nominal (100) and the
heights are real (28, 28, 96).

### 12.6 The click target

The click target is invisible and created last among the panel's children. It
sits below the dial in the vertical stack, so it does not overlap anything. It
has action 200. Clicking that empty strip increments the target counter.

### 12.7 First paint and action registration

```basic
1200 GOSUB blApiPaintAll
1220 blReqActionId% = ActButton% : blReqLine& = 2010 : GOSUB blApiSetActionLine
...
1295 TIMER ON
1298 GOTO MainLoop
```

The action table points at 2010/2110/2210/2310, the first executable line of
each handler. The preceding 20xx lines are `REM` comments and are not valid
computed-`GOSUB` targets.

### 12.8 The control loop

```basic
1300 LABEL MainLoop
1310 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
1320 GOSUB blApiPollInput
1330 WHILE blEventCount% > 0
1340   GOSUB blApiNextEvent
1350   GOSUB DispatchAction
1360 WEND
1370 K$ = INKEY$
1380 IF K$ <> "" THEN GOSUB HandleKey
1390 IF blNeedsPaint% <> 0 THEN GOSUB blApiPaintAll
1395 SLEEP 16
1398 GOTO MainLoop
```

The timer flag is checked at the **top** of the loop, exactly as requested:

- `ClockFlag%` is set by the timer handler at 6010.
- The loop clears it and calls `UpdateClock`.
- `UpdateClock` sets the label text to `"Clock: " + TIME$` and repaints.
- The handler never calls baslib.

Mouse handling is entirely inside `blApiPollInput`. The loop then drains the
events. A button or target click produces a click event; a slider or dial click
produces a change event and a click event. `DispatchAction` maps the event's
action ID through `blActionLine&` and `GOSUB`s the host handler.

Keyboard handling is host-side:

| Key | Action |
|---|---|
| `Esc` | `TIMER OFF`, end program |
| `R` or `r` | reset both counters, refresh status |
| `+` or `=` | slider value +5, refresh |
| `-` or `_` | slider value -5, refresh |
| `C` or `c` | refresh the clock immediately |

The nudge helpers read the slider with `blApiGetValue`, clamp to 0-100, write it
with `blApiSetValue`, and refresh the status label.

### 12.9 The action handlers

```basic
2010 ButtonCount% = ButtonCount% + 1
2020 GOSUB RefreshStatus
2030 RETURN

2110 TargetCount% = TargetCount% + 1
2120 GOSUB RefreshStatus
2130 RETURN

2210 GOSUB RefreshStatus
2220 RETURN

2310 GOSUB RefreshStatus
2320 RETURN
```

The slider and dial handlers do not need to read the value themselves because
`RefreshStatus` re-reads both values. A click produced two events, so the
handler may run twice; refreshing a readout twice is harmless.

`RefreshStatus` builds the three-line status string in `StatusText$` and avoids
one enormous line:

```basic
7140 StatusText$ = "Button clicks: " + MID$(STR$(ButtonCount%),2)
7142 StatusText$ = StatusText$ + CHR$(13) + "Target hits: " + MID$(STR$(TargetCount%),2)
7144 StatusText$ = StatusText$ + CHR$(13) + "Slider: " + MID$(STR$(INT(SliderV#)),2)
7146 StatusText$ = StatusText$ + "   Dial: " + MID$(STR$(INT(DialV#)),2)
```

`MID$(STR$(n),2)` removes the leading space that `STR$` puts in front of a
positive number.

### 12.10 Running and verifying

```text
LOAD "baslib/samplehost.bas"
MERGE "baslib/baslib.bas"
RUN
```

Expected behavior:

- The panel appears with the header, live clock, status, button, slider, gauge,
  and dial.
- The clock changes once per second.
- Clicking the button increments "Button clicks".
- Clicking the strip below the dial increments "Target hits".
- Clicking the slider changes its value; the status readout updates.
- Clicking the dial changes its value; the needle moves.
- Clicking the gauge does nothing (read-only).
- `+`/`-` nudge the slider; `R` resets counters; `C` refreshes the clock;
  `Esc` exits.

The implementation of this sample was verified with the real interpreter using
`--gpu-shot` and synthetic input; see §14 for the technique.

### 12.11 Why one of every control?

| Control | Where |
|---|---|
| Top (root) | Created by `blApiInit`; owns the screen |
| Panel | Main vertical container |
| Button | "Click me" |
| Label | Header, clock, and status readout |
| Slider | Value control with change events |
| Gauge | Read-only bar |
| Dial | Circular value control |
| Click target | Invisible strip below the dial |

That covers every class and every user kind in the current `blCreate`.

---

## 13. Complete host program

The following is the complete contents of [`samplehost.bas`](samplehost.bas).
It contains **no baslib source**; every library reference is a public API call
or a public `bl` constant. Merge it with `baslib.bas` before running.

```basic
10 REM baslib complete host sample: one of every control, mouse + keyboard loop.
20 REM Host lines stay below 500000. Merge baslib.bas before RUN:
30 REM   LOAD "baslib/samplehost.bas" : MERGE "baslib/baslib.bas" : RUN
40 REM No baslib source is included here; only public blApi... calls and bl constants.
50 CONST ActButton% = 100
60 CONST ActSlider% = 150
70 CONST ActDial% = 160
80 CONST ActTarget% = 200
90 ButtonCount% = 0 : TargetCount% = 0 : ClockFlag% = 0
100 SCREEN 720
110 CLS
120 ON TIMER(1) GOSUB 6010
130 blReqW% = 0 : blReqH% = 0
140 GOSUB blApiInit
150 IF blStatus% <> blStatusOk% THEN GOTO 9000
200 GOSUB ResetReq
210 blReqClass% = blClassPanel% : blReqParent% = blRoot%
220 blReqX% = 40 : blReqY% = 24 : blReqW% = 640 : blReqH% = 480
230 blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
240 GOSUB blApiCreate : MainH% = blRetHandle%
250 IF blStatus% <> blStatusOk% THEN GOTO 9000
300 GOSUB ResetReq
310 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
320 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24
330 blReqText$ = "baslib sample host - one of every control" : blReqPad% = 4
340 GOSUB blApiCreate : HeaderH% = blRetHandle%
350 IF blStatus% <> blStatusOk% THEN GOTO 9000
400 GOSUB ResetReq
410 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
420 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
430 blReqText$ = "Clock: " + TIME$ : blReqPad% = 4
440 GOSUB blApiCreate : ClockH% = blRetHandle%
450 IF blStatus% <> blStatusOk% THEN GOTO 9000
500 GOSUB ResetReq
510 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
520 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 60
530 blReqText$ = "Button clicks: 0" + CHR$(13) + "Target hits: 0" + CHR$(13) + "Slider: 25   Dial: 35" : blReqPad% = 4
540 GOSUB blApiCreate : StatusH% = blRetHandle%
550 IF blStatus% <> blStatusOk% THEN GOTO 9000
700 GOSUB ResetReq
710 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindButton%
720 blReqX% = 0 : blReqY% = 0 : blReqW% = 180 : blReqH% = 28
730 blReqText$ = "Click me" : blReqAction% = ActButton% : blReqPad% = 6
740 GOSUB blApiCreate : ButtonH% = blRetHandle%
750 IF blStatus% <> blStatusOk% THEN GOTO 9000
800 GOSUB ResetReq
810 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindSlider%
820 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
830 blReqValue# = 25 : blReqAction% = ActSlider% : blReqPad% = 6
840 GOSUB blApiCreate : SliderH% = blRetHandle%
850 IF blStatus% <> blStatusOk% THEN GOTO 9000
900 GOSUB ResetReq
910 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindGauge%
920 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
930 blReqValue# = 60 : blReqPad% = 6
940 GOSUB blApiCreate : GaugeH% = blRetHandle%
950 IF blStatus% <> blStatusOk% THEN GOTO 9000
1000 GOSUB ResetReq
1010 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindDial%
1020 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 96
1030 blReqValue# = 35 : blReqAction% = ActDial% : blReqPad% = 6
1040 GOSUB blApiCreate : DialH% = blRetHandle%
1050 IF blStatus% <> blStatusOk% THEN GOTO 9000
1100 GOSUB ResetReq
1110 blReqClass% = blClassClickTarget% : blReqParent% = MainH%
1120 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 40
1130 blReqAction% = ActTarget% : blReqButtonMask% = 1
1140 GOSUB blApiCreate : TargetH% = blRetHandle%
1150 IF blStatus% <> blStatusOk% THEN GOTO 9000
1200 GOSUB blApiPaintAll
1210 IF blStatus% <> blStatusOk% THEN GOTO 9000
1220 blReqActionId% = ActButton% : blReqLine& = 2010 : GOSUB blApiSetActionLine
1230 IF blStatus% <> blStatusOk% THEN GOTO 9000
1240 blReqActionId% = ActSlider% : blReqLine& = 2210 : GOSUB blApiSetActionLine
1250 IF blStatus% <> blStatusOk% THEN GOTO 9000
1260 blReqActionId% = ActDial% : blReqLine& = 2310 : GOSUB blApiSetActionLine
1270 IF blStatus% <> blStatusOk% THEN GOTO 9000
1280 blReqActionId% = ActTarget% : blReqLine& = 2110 : GOSUB blApiSetActionLine
1290 IF blStatus% <> blStatusOk% THEN GOTO 9000
1295 TIMER ON
1298 GOTO MainLoop
1300 LABEL MainLoop
1310 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
1320 GOSUB blApiPollInput
1330 WHILE blEventCount% > 0
1340   GOSUB blApiNextEvent
1350   GOSUB DispatchAction
1360 WEND
1370 K$ = INKEY$
1380 IF K$ <> "" THEN GOSUB HandleKey
1390 IF blNeedsPaint% <> 0 THEN GOSUB blApiPaintAll
1395 SLEEP 16
1398 GOTO MainLoop
2000 REM action 100: button click
2010 ButtonCount% = ButtonCount% + 1
2020 GOSUB RefreshStatus
2030 RETURN
2100 REM action 200: invisible click target
2110 TargetCount% = TargetCount% + 1
2120 GOSUB RefreshStatus
2130 RETURN
2200 REM action 150: slider value changed
2210 GOSUB RefreshStatus
2220 RETURN
2300 REM action 160: dial value changed
2310 GOSUB RefreshStatus
2320 RETURN
6000 REM 1-second timer tick: set a flag and return. Do not call baslib here.
6010 ClockFlag% = 1
6020 RETURN
7000 LABEL ResetReq
7010 blReqClass% = 0 : blReqParent% = 0 : blReqHandle% = 0
7020 blReqX% = 0 : blReqY% = 0 : blReqW% = 0 : blReqH% = 0
7030 blReqFlags% = 0 : blReqAction% = 0 : blReqTag% = 0
7040 blReqKind% = 0 : blReqMode% = 0 : blReqPad% = 0 : blReqGap% = 0 : blReqCols% = 0
7050 blReqButtonMask% = 1 : blReqText$ = "" : blReqValue# = 0 : blReqEnabled% = 1
7060 RETURN
7100 LABEL RefreshStatus
7110 blReqHandle% = SliderH% : GOSUB blApiGetValue : SliderV# = blRetValue#
7120 blReqHandle% = DialH% : GOSUB blApiGetValue : DialV# = blRetValue#
7130 blReqHandle% = StatusH%
7140 StatusText$ = "Button clicks: " + MID$(STR$(ButtonCount%),2)
7142 StatusText$ = StatusText$ + CHR$(13) + "Target hits: " + MID$(STR$(TargetCount%),2)
7144 StatusText$ = StatusText$ + CHR$(13) + "Slider: " + MID$(STR$(INT(SliderV#)),2)
7146 StatusText$ = StatusText$ + "   Dial: " + MID$(STR$(INT(DialV#)),2)
7148 blReqText$ = StatusText$
7150 GOSUB blApiSetText
7160 GOSUB blApiPaintAll
7170 RETURN
7200 LABEL UpdateClock
7210 blReqHandle% = ClockH%
7220 blReqText$ = "Clock: " + TIME$
7230 GOSUB blApiSetText
7240 GOSUB blApiPaintAll
7250 RETURN
7300 LABEL DispatchAction
7310 IF blEventAction% = blActionNone% THEN RETURN
7320 ActLine& = blActionLine&(blEventAction%)
7330 IF ActLine& = blNoLine& THEN RETURN
7340 GOSUB ActLine&
7350 RETURN
7400 LABEL HandleKey
7410 IF K$ = CHR$(27) THEN TIMER OFF : END
7420 IF K$ = "R" OR K$ = "r" THEN ButtonCount% = 0 : TargetCount% = 0 : GOSUB RefreshStatus
7430 IF K$ = "+" OR K$ = "=" THEN GOSUB NudgeUp
7440 IF K$ = "-" OR K$ = "_" THEN GOSUB NudgeDown
7450 IF K$ = "C" OR K$ = "c" THEN GOSUB UpdateClock
7460 RETURN
7500 LABEL NudgeUp
7510 blReqHandle% = SliderH% : GOSUB blApiGetValue
7520 NewV# = blRetValue# + 5
7530 IF NewV# > 100 THEN NewV# = 100
7540 blReqValue# = NewV# : GOSUB blApiSetValue
7550 GOSUB RefreshStatus
7560 RETURN
7600 LABEL NudgeDown
7610 blReqHandle% = SliderH% : GOSUB blApiGetValue
7620 NewV# = blRetValue# - 5
7630 IF NewV# < 0 THEN NewV# = 0
7640 blReqValue# = NewV# : GOSUB blApiSetValue
7650 GOSUB RefreshStatus
7660 RETURN
9000 LABEL Fail
9010 TIMER OFF
9020 PRINT "baslib sample host failed; status ="; blStatus%
9030 END
9040 END
```

### 13.1 Host symbols used

| Symbol | Meaning |
|---|---|
| `ActButton%`, `ActSlider%`, `ActDial%`, `ActTarget%` | Action IDs |
| `ButtonCount%`, `TargetCount%` | Click counters |
| `ClockFlag%` | Set by the timer handler, consumed at loop top |
| `MainH%`, `HeaderH%`, `ClockH%`, `StatusH%` | Control handles |
| `ButtonH%`, `SliderH%`, `GaugeH%`, `DialH%`, `TargetH%` | Control handles |
| `SliderV#`, `DialV#`, `NewV#` | Value scratch |
| `ActLine&` | Resolved action handler line |
| `StatusText$` | Built status caption |
| `K$` | `INKEY$` result |

None of these begins with `bl`.

---

## 14. Diagnostics and debugging

### 14.1 Always check status

```basic
GOSUB blApiCreate
IF blStatus% <> blStatusOk% THEN PRINT "create"; blStatus%: GOTO Fail
```

A status check immediately after the call is the only reliable diagnostic;
`blStatus%` is overwritten by the next API call. `blError%` is currently unused
and stays 0.

### 14.2 Common symptoms

| Symptom | Likely cause | Fix |
|---|---|---|
| `blApiCreate` returns 4 | Class not 2/3/4 | Use a class constant; the root is created by init |
| `blApiCreate` returns 7 | Parent handle is 0, out of range, or free | Create the parent first and keep its result handle |
| `blApiCreate` returns 10 | User kind outside 1-5 | Use `blKind...%` |
| `blApiCreate` returns 2 | 255 controls already allocated | Destroy unused subtrees or reuse |
| Button action never fires | Table not registered, table points at a `REM` line, or action is 0 | Register the executable line, check the ID |
| Click hits the wrong control | A later-created label/control overlaps it | Reorder creation, move the control, or hide the blocker |
| Click does nothing on a label | Label has no action and no ancestor has one | Put the action on the label, its panel, or the root |
| Slider/dial handler runs twice | Change event plus click event | Make the handler idempotent or check `blEventType%` |
| Old text remains after an update | Transparent label plus shorter text | Use `blApiPaintAll` |
| Timer never fires | Forgot `TIMER ON`, armed after the loop, or handler line is wrong | Arm after setup, point at an executable line |
| Clock updates only occasionally | `SLEEP` blocks timer servicing | Keep sleeps short (16 ms) |
| Program seems to ignore keys | `INKEY$` not polled every frame, or key is a non-printable | Poll every frame; use printable keys and `Esc` |
| Event queue seems stuck | Not draining every frame; move events fill the ring | Drain `WHILE blEventCount% > 0` |
| Widgets overlap after adding a panel | Nested-panel layout defect | Keep the panel tree flat or nest last (§9.2) |
| Host stack sentinel fails | A host `STACK` push/pop is unbalanced | Check the host, not baslib; run the sentinel test |

### 14.3 Dump the tree

```basic
GOSUB blApiDump
```

Output looks like:

```text
H 1 C 1 P 0 X 0 Y 0 W 1280 H 720
H 2 C 2 P 1 X 40 Y 24 W 640 H 480
H 3 C 3 P 2 X 52 Y 36 W 616 H 24
...
```

Use it to confirm class, parent, absolute position, and size after layout.

### 14.4 Stack sentinel

```basic
STACK PUSH 12345
GOSUB blApiInit
GOSUB blApiPaintAll
STACK POP X%
IF X% <> 12345 THEN PRINT "STACK leak": END
```

Run this after adding new API calls. It catches a library or host imbalance
quickly.

### 14.5 Headless and synthetic testing

The repository's fixture tests run with a headless screen. For a manual check:

```text
newbasic --gpu --gpu-shot shot.ppm merged.bas
```

Then inspect the PPM. For input determinism, replace real mouse use with the
test hooks:

```basic
blReqX% = 360 : blReqY% = 186 : blReqButton% = 1
GOSUB blApiTestClickAt
WHILE blEventCount% > 0
  GOSUB blApiNextEvent
  GOSUB DispatchAction
WEND
```

This exercises the same hit-test, bubble, and event code as a real click.

### 14.6 Reading a failed run

- `blStatus%` is the primary code.
- `ERL` reports the BASIC line. Library lines are in 500000-519999, so an `ERL`
  in that range points into baslib.
- `ERR` is the BASIC error number; expected baslib failures should not raise
  one.
- If a runtime error occurred inside a library call, the safest recovery is
  `CLEAR`, then re-run the host from initialization.

---

## 15. Current implementation notes and limitations

The design in [`baslib-plan.md`](baslib-plan.md) describes a larger v0.2
framework. The shipped build has the following concrete differences. Treat this
table as part of the API contract until the implementation changes.

### 15.1 Dispatch and extension

| Issue | Current behavior | Workaround |
|---|---|---|
| Class method dispatch | Public APIs use direct class/kind checks. `blMethod&` is not used for paint/hit/click. | Do not rely on class methods |
| `blApiRegisterClass` | Fills a method-table row but has no effect on normal UI behavior | Use it only with the internal `blDispatch`/`blSend` protocol |
| Class routers | `blTopRouter`, `blPanelRouter`, `blUserRouter`, `blClickRouter` are `RETURN` stubs | None needed for host code |
| Version string | `blVersion$` is `"0.1"` even though some value controls are present | Report 0.1 behavior in bug reports |

### 15.2 Visual properties

| Issue | Current behavior | Workaround |
|---|---|---|
| Per-control colors | `blReqFace%`, `blReqAccent%`, `blReqTextColor%` are ignored | Write `blUser#`/`blPanel%` fields directly |
| Text scale | `blReqTextScale%` is ignored | Set `blUser#(h,20)` |
| Alignment | `blReqAlign%`, `blReqVAlign%` are ignored; labels default center/middle | Set `blUser#(h,14/15)` |
| Transparency | `blReqTransparent%` is ignored; labels are transparent | Set `blUser#(h,18)` |
| Border style | `blReqBorderStyle%` is ignored; labels/buttons draw a border by default | Set `blUser#(h,17) = -1` |
| Word wrap | `blReqWrap%` is ignored; wrap field is always 0 | No public path; direct field only |
| Auto-size request | `blReqAutoSize%` is ignored | Call `blApiAutoSize` |
| Theme globals | The plan's `blTheme...%` variables are not defined | Use direct color fields |

### 15.3 Layout and painting

| Issue | Current behavior | Workaround |
|---|---|---|
| Nested panels | Shared layout scratch globals corrupt the parent's remaining children | Flat panel tree, or nested container last |
| Grid mode | Mode 3 is stored but behaves as absolute | Use modes 0/1/2 |
| Per-control clipping | Clip rectangles are computed but not applied during paint | Keep children inside parents |
| Host viewport | Paint preserves `VIEW` but does not rebase coordinates | Paint with the full-canvas viewport |
| Double buffering | None | Use `blApiUpdate` for small changes; accept possible flicker |
| Dirty compositor | None | Repaint the frame or the control subtree |
| `blApiUpdate` background | Does not clear the parent area | Use `blApiPaintAll` after geometry/text-length changes |
| Zero-size children | Legal but paint is skipped; layout still reserves space in stacks | Hide them instead |

### 15.4 Input and events

| Issue | Current behavior | Workaround |
|---|---|---|
| Label click-through | Labels hit-test and can block controls underneath | Move labels or put the action on the label |
| Click-target mask | Stored but not enforced | Filter by `blEventButton%` in the host |
| Button mask on buttons | Not supported; any button fires | Filter by `blEventButton%` in the host |
| Live right/middle | Only left is tracked for hover/press/capture | Use `MOUSEB`/click events for right/middle |
| Move coalescing | A move event is posted every poll | Drain events each frame |
| Pointer leaving canvas | No leave event is synthesized | Track `MOUSEPOSX < 0` in the host if needed |
| Hover/pressed visuals | Flags are not set by input; paint has no hover state | No workaround short of custom paint |
| Disabled ancestor | Does not disable enabled children | Disable the subtree explicitly |
| Capture lost event | Not posted on hide/destroy | Re-poll after destroying |
| Keyboard focus/keys | Not implemented | Poll `INKEY$` in the host |
| Timer reentrancy | Handler must not call baslib | Use the flag pattern |

### 15.5 API edge behavior

| Issue | Current behavior | Workaround |
|---|---|---|
| `blError%` | Never populated; remains 0 | Use `blStatus%` |
| Status 1/5/6/9/11/14/15/16/17 | Defined but not returned by public API | Do not branch on them |
| Geometry validation | `Create`/`SetBounds` do not validate W/H or coordinates | Validate in the host |
| Hidden/disabled at create | Visible/enabled flags are always added | Create then `Hide`/`SetEnabled` |
| Panel mode validation | Any mode is stored | Use 0/1/2 |
| Action range | `blApiSetAction` does not range-check | Keep IDs 0-255 |
| Value kind check | `SetValue`/`GetValue` accept any class-3 control | Keep a separate handle registry |
| `blApiAutoSize` | Class 3 only; includes padding and border allowance | Use for labels/buttons |
| `blApiDump` | Prints all controls; no handle filter | Grep/scan the output |
| `blCtlZ%` | Set at create, not maintained | Do not use for ordering |
| `blApiDestroy` | Does not clear hover/capture/pending events | Hide first and re-poll |
| Re-init | Invalidates handles and action lines | Recreate and re-register |

---

## 16. Performance notes

baslib targets a few hundred controls on one canvas, not thousands.

- `blApiFindAt`/`blApiPollInput` hit test is O(255): a loop over every handle.
  This is negligible at the intended scale.
- `blApiPaintAll` visits every visible control once. `blApiUpdate` visits one
  subtree.
- Text measurement (`TEXTWIDTH`/`TEXTHEIGHT`) runs during every paint of a text
  control. Large text scales multiply pixel work.
- Layout is O(visible controls) and only runs when `blNeedsLayout%` is set.
- The event ring is fixed at 64. Move events are not coalesced, so a busy loop
  can fill it if the host does not drain.
- `PUSH VIEW`/`POP VIEW` per control is a small constant cost.
- Avoid `blApiPaintAll` more than once per frame. Batch changes, then paint.
- Use `SLEEP 16` (about 60 fps) or `SLEEP 8` for smoother input. Do not busy-loop
  without a sleep; newbasic needs to pump the window and timer.
- Direct table writes avoid API overhead but bypass dirty flags; set
  `blNeedsPaint%` yourself.

The largest practical cost is full-frame repainting with large text or many
controls. `blApiUpdate` on the changed control is the intended optimization when
the geometry is stable.

---

## 17. Glossary

| Term | Meaning |
|---|---|
| Action | Host integer stored on a control; copied to a click/change event |
| Action line | Host line number stored in `blActionLine&` for an action ID |
| Absolute rectangle | Control position in canvas pixels after layout |
| Bubble | Offer an unhandled click to the parent |
| Capture | Control that receives move/up while the left button is held |
| Class | Handler category: Top, Panel, User, ClickTarget |
| Client area | Panel interior after padding |
| Clip rectangle | Cached intersection of a control with its ancestors |
| Control | One row in the tables, identified by a handle |
| Frame | Internal `STACK` dispatch record (not used by host API calls) |
| Handle | Integer 1-255 identifying a control; 0 is null |
| Hit test | Find the control under an absolute point |
| Kind | User-control subtype: button, slider, gauge, dial, or label |
| Layout pass | Compute each child's local position and each control's absolute rectangle |
| Local rectangle | X/Y relative to the parent's outer corner |
| Paint pass | Draw the tree back-to-front |
| Root | Handle 1; the top-level control |
| Text scale | `DRAWTEXT` glyph scale, 1-8 |
| Viewport | Graphics `VIEW` rectangle; baslib preserves but does not rebase it |
| Z-order | Sibling creation order; later siblings paint in front |

---

## Appendix A — API line and label map

| Line | Label |
|---:|---|
| 510000 | `blApiInit` |
| 510100 | `blApiCreate` |
| 510200 | `blApiDestroy` |
| 510300 | `blApiSetBounds` |
| 510400 | `blApiShow` |
| 510500 | `blApiHide` |
| 510600 | `blApiSetText` |
| 510700 | `blApiSetValue` |
| 510800 | `blApiGetValue` |
| 510900 | `blApiSetLayout` |
| 511000 | `blApiSetAction` |
| 511100 | `blApiInvalidate` |
| 511200 | `blApiLayoutAll` |
| 511300 | `blApiPaintAll` |
| 511400 | `blApiUpdate` |
| 511500 | `blApiPollInput` |
| 511550 | `blApiPollMouse` |
| 511600 | `blApiNextEvent` |
| 511700 | `blApiFindAt` |
| 511800 | `blApiTestClickAt` |
| 511810 | `blApiTestMouseMove` |
| 511820 | `blApiTestMouseButton` |
| 511830 | `blApiTestWheel` |
| 511900 | `blApiDump` |
| 511910 | `blApiAutoSize` |
| 511920 | `blApiSetActionLine` |
| 511930 | `blApiRegisterClass` |
| 511950 | `blApiSetEnabled` |

Internal labels used by baslib and tests but not part of the public contract:
`blInit`, `blCreate`, `blDestroy`, `blPaintAll`, `blLayoutAll`, `blFramePush`,
`blFrameLoad`, `blFrameClose`, `blFrameDrop`, `blDispatch`, `blSend`, `blNoop`,
and the class routers.

---

## Appendix B — Control table field maps

### B.1 Common table `blCtl%(handle, field)`

| Field | Meaning |
|---:|---|
| 1 | Class ID; 0 means free |
| 2 | Parent handle; 0 for root |
| 3 | First (back-most) child |
| 4 | Last (front-most) child |
| 5 | Previous sibling |
| 6 | Next sibling |
| 7 | Local X |
| 8 | Local Y |
| 9 | Width |
| 10 | Height |
| 11 | Absolute X |
| 12 | Absolute Y |
| 13 | Flags |
| 14 | State (unused) |
| 15 | Action ID |
| 16 | Host tag |
| 17 | Capture (unused; capture is global) |

### B.2 Panel table `blPanel%(handle, field)`

| Field | Meaning | Default |
|---:|---|---:|
| 1 | Layout mode | request |
| 2 | Padding | request |
| 3 | Gap | request |
| 4 | Grid columns | request (unused) |
| 5 | Reserved | 0 |
| 6 | Background palette index | 7 |
| 7 | Border palette index | 8 |
| 8 | Reserved | 0 |

### B.3 User table `blUser#(handle, field)`

| Field | Meaning | Default |
|---:|---|---:|
| 1 | Kind | request |
| 2 | Value | request |
| 3 | Minimum | 0 |
| 4 | Maximum | 100 |
| 5 | Step | 1 |
| 6 | Orientation | 0 |
| 7 | Style | 0 |
| 8 | Face color | 7 |
| 9 | Accent color | 4 |
| 10 | Drag state | 0 |
| 11 | Decimals | 0 |
| 12 | Reserved | 0 |
| 13 | Text padding | request |
| 14 | Horizontal align (0 left, 1 center, 2 right) | 1 |
| 15 | Vertical align (0 top, 1 middle, 2 bottom) | 1 |
| 16 | Word wrap (0/1) | 0 |
| 17 | Border (>=0 drawn, -1 none) | 0 |
| 18 | Transparent (0 opaque, 1 transparent) | 1 |
| 19 | Text color | 0 |
| 20 | Text scale 1-8 | 1 |
| 21 | Auto-size flag (unused) | 0 |
| 22 | Line count (unused; stays 1) | 1 |

### B.4 Click-target table `blClick%(handle, field)`

| Field | Meaning | Default |
|---:|---|---:|
| 1 | Button mask (stored only) | request |
| 2 | Hot | 0 |
| 3 | Pressed | 0 |
| 4 | Wheel opt-in | 0 |

### B.5 Direct-table examples

Change a label's alignment, scale, and border, then repaint:

```basic
blUser#(LabelH%,14) = 0
blUser#(LabelH%,15) = 0
blUser#(LabelH%,17) = -1
blUser#(LabelH%,20) = 2
blNeedsPaint% = 1
GOSUB blApiPaintAll
```

Change a panel's colors:

```basic
blPanel%(PanelH%,6) = 8       ' background
blPanel%(PanelH%,7) = 15      ' border
blNeedsPaint% = 1
GOSUB blApiPaintAll
```

Change a slider's range before using it:

```basic
blUser#(SliderH%,3) = -50
blUser#(SliderH%,4) = 50
blReqHandle% = SliderH% : blReqValue# = 0 : GOSUB blApiSetValue
GOSUB blApiPaintAll
```

The direct route bypasses validation and does not set dirty flags. Always set
`blNeedsPaint% = 1` after a visual change and repaint.

---

*End of guide.*

