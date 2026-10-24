# M

<a id="merge-command"></a>
## MERGE Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `LOAD`, `SAVE`, `LIST`

### Purpose

Merges stored lines from a program file into the current program.

### Syntax

```basic
MERGE filename
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | Text or tokenized program file to merge. |

### Comments

Matching line numbers in the current program are replaced. Unlike `LOAD`, `MERGE` keeps lines that are not present in the incoming file. The merged program is recompiled; a compile error leaves the old program available for inspection.

### Examples

```text
MERGE "PART2.BAS"
LIST
```

<a id="mid-function"></a>
## MID$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `LEFT$`, `RIGHT$`, `INSTR`

### Purpose

Returns a substring starting at a given position.

### Syntax

```basic
MID$(string$, start [, length])
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Source string. |
| `start` | Yes | 1-based starting position. |
| `length` | No | Number of characters. Default is the rest of the string. |

### Comments

If `start` is beyond the end, the result is empty. A non-positive start raises **Illegal function call**. If `length` exceeds the remaining string, the rest is returned.

The assignment form `MID$(A$, start, length) = B$` is not implemented; use concatenation with `LEFT$` and `MID$` to rebuild the string.

### Examples

#### Example 1 — Extract a substring

```basic
10 A$="NEWBASIC"
20 PRINT MID$(A$, 4, 3)
30 PRINT MID$(A$, 4)
40 END
```

Output:

```text
BAS
BASIC
```

<a id="mkd-function"></a>
## MKD$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `CVD`, `MKI$`, `MKS$`

### Purpose

Packs a double-precision number into an 8-byte string.

### Syntax

```basic
MKD$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric value. |

### Comments

The result is suitable for `LSET` into an 8-byte `FIELD`, or for `CVD` to unpack. It is a byte representation, not a text representation.

### Examples

#### Example 1 — Pack and unpack

```basic
10 A# = 1.5
20 S$ = MKD$(A#)
30 PRINT LEN(S$)
40 PRINT CVD(S$)
50 END
```

<a id="mkdir-command"></a>
## MKDIR Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Creates a directory.

### Syntax

```basic
MKDIR path
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `path` | Yes | Directory to create. |

### Comments

Implemented as a direct-mode command, not a program statement. The parent directory must exist; this is not a recursive create. Use `CHDIR` to enter it and `RMDIR` to remove it.

### Examples

```text
MKDIR "DATA"
CHDIR "DATA"
```

<a id="mki-function"></a>
## MKI$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `CVI`, `MKD$`, `MKS$`

### Purpose

Packs an integer into a 2-byte string.

### Syntax

```basic
MKI$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric value in the integer range. |

### Comments

`CVI` is the inverse. The string is a binary field representation and may contain non-printable bytes.

### Examples

#### Example 1 — Pack an integer

```basic
10 A% = 1234
20 S$ = MKI$(A%)
30 PRINT LEN(S$)
40 PRINT CVI(S$)
50 END
```

<a id="mks-function"></a>
## MKS$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `CVS`, `MKD$`, `MKI$`

### Purpose

Packs a single-precision number into a 4-byte string.

### Syntax

```basic
MKS$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric value. |

### Comments

`CVS` is the inverse. The result is a binary four-byte field value.

### Examples

#### Example 1 — Pack a single

```basic
10 A! = 2.5
20 S$ = MKS$(A!)
30 PRINT LEN(S$)
40 PRINT CVS(S$)
50 END
```

<a id="motor-statement"></a>
## MOTOR Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Turns the cassette tape motor on or off.

### Syntax

```basic
MOTOR [ON|OFF]
```

### Newbasic status

Not implemented; there is no cassette interface.

<a id="mouse-function"></a>
## MOUSE Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns one field of the most recent click.

### Syntax

```basic
MOUSE(n)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `n` | Yes | `0` button, `1` x coordinate, `2` y coordinate. |

### Comments

Like `MOUSEB`, a call with a queued click consumes one and latches its button and coordinates. `MOUSE(0)` returns that button, while `MOUSE(1)` and `MOUSE(2)` return the latched coordinates. They are the same fields that `MOUSEB`, `MOUSEX`, and `MOUSEY` read. A field outside `0`–`2` raises **Illegal function call**.

<a id="mouseb-function"></a>
## MOUSEB Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Consumes one queued click and returns its button.

### Syntax

```basic
MOUSEB
```

### Comments

Returns `1` for the left button, `2` for the right button, `3` for the middle button, or `0` when no click is queued. The coordinates of the consumed click are available through `MOUSEX` and `MOUSEY`. Backends without a pointing device always return `0`.

<a id="mousebuttons-function"></a>
## MOUSEBUTTONS Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns the live mouse button state as a bit mask.

### Syntax

```basic
MOUSEBUTTONS
```

### Comments

Bit `1` is the left button, bit `2` the right button, and bit `4` the middle button. Unlike `MOUSEB`, this does not consume a click. The value is current for the last message pump; backends without a pointing device return `0`.

<a id="mouseposx-function"></a>
## MOUSEPOSX Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns the live mouse x position in canvas pixels.

### Syntax

```basic
MOUSEPOSX
```

### Comments

This is the current pointer position, not the last click position. A window backend returns `-1` when the pointer is outside the graphics canvas. A backend without a pointing device returns `0`.

<a id="mouseposy-function"></a>
## MOUSEPOSY Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns the live mouse y position in canvas pixels.

### Syntax

```basic
MOUSEPOSY
```

### Comments

This is the current pointer position, not the last click position. A window backend returns `-1` when the pointer is outside the graphics canvas. A backend without a pointing device returns `0`.

<a id="mousewheel-function"></a>
## MOUSEWHEEL Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns and consumes accumulated wheel movement.

### Syntax

```basic
MOUSEWHEEL
```

### Comments

Positive values scroll up/away from the user and negative values scroll down/toward the user, following the host backend's wheel direction. The value is a count of wheel steps accumulated since the previous call; the call resets it to zero. Backends without a wheel return `0`.

<a id="mousex-function"></a>
## MOUSEX Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns the x coordinate of the most recent click.

### Syntax

```basic
MOUSEX
```

### Comments

Coordinates are canvas pixels. The value is latched when a mouse function consumes a click and does not follow pointer movement; use `MOUSEPOSX` for the live position.

<a id="mousey-function"></a>
## MOUSEY Function

**Type:** Function  
**Status:** Supported (newbasic extension)

### Purpose

Returns the y coordinate of the most recent click.

### Syntax

```basic
MOUSEY
```

### Comments

Coordinates are canvas pixels. The value is latched when a mouse function consumes a click and does not follow pointer movement; use `MOUSEPOSY` for the live position.
