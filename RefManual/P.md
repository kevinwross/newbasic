# P

<a id="paint-statement"></a>
## PAINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `CIRCLE`, `LINE`, `PSET`

### Purpose

Flood-fills a graphics region up to a border colour.

### Syntax

```basic
PAINT [STEP] (x, y) [, [fill] [, [border]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Seed point. `STEP` makes it relative to the last graphics point. |
| `fill` | No | Fill attribute. Default is the current foreground. |
| `border` | No | Stop at this attribute. Default is the seed colour. |

### Comments

`PAINT` requires a graphics mode. A fill or border outside the current palette range raises **Illegal function call**. A border of `-1` is accepted as “stop at any colour different from the seed.”

### Examples

#### Example 1 — Fill a circle

```basic
10 SCREEN 1
20 CIRCLE (160,100),50,2
30 PAINT (160,100),3,2
40 END
```

<a id="palette-statement"></a>
## PALETTE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `COLOR`, `SCREEN`

### Purpose

Changes the display colour associated with a palette attribute.

### Syntax

```basic
PALETTE [attribute, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `attribute` | No | Palette slot to change. |
| `colour` | No | Display colour for that slot. |

### Comments

With no arguments, `PALETTE` restores the current mode’s default palette. In `SCREEN 13` and `SCREEN 720`, attributes range over 256 entries; earlier modes use the mode’s smaller palette. `PALETTE` changes the mapping, not the pixel’s stored attribute.

### Examples

#### Example 1 — Remap a colour

```basic
10 SCREEN 13
20 PALETTE 1, 63
30 PSET (10,10), 1
40 END
```

<a id="palette-using-statement"></a>
## PALETTE USING Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Sets many palette entries from an array in one operation.

### Syntax

```basic
PALETTE USING array(subscript)
```

### Newbasic status

Not implemented. Use repeated `PALETTE attribute, colour` statements instead.

<a id="pcopy-command"></a>
## PCOPY Command

**Type:** Command  
**Status:** Supported  
**See also:** `SCREEN`, `GET`, `PUT`

### Purpose

Copies one graphics page to another.

### Syntax

```basic
PCOPY source, destination
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `source` | Yes | Source page `0`–`7`. |
| `destination` | Yes | Destination page `0`–`7`. |

### Comments

Page `0` is the displayed canvas; pages `1`–`7` are snapshots. The source page must contain an image. Invalid pages raise **Illegal function call**.

### Examples

#### Example 1 — Save and restore a page

```basic
10 SCREEN 1
20 CIRCLE (160,100),40,2
30 PCOPY 0,1
40 CLS
50 PCOPY 1,0
60 END
```

<a id="peek-function"></a>
## PEEK Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Reads a byte from machine memory.

### Syntax

```basic
PEEK(address)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no emulated 64K address space.

<a id="pen-function"></a>
## PEN Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns light-pen coordinates and pen-down state.

### Syntax

```basic
PEN(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no light-pen device.

<a id="pen-statement"></a>
## PEN Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Enables, disables, or stops light-pen trapping.

### Syntax

```basic
PEN ON
PEN OFF
PEN STOP
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no light-pen device.

<a id="play-statement"></a>
## PLAY Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SOUND`, `BEEP`, `PLAY()`

### Purpose

Plays a music string.

### Syntax

```basic
PLAY command$
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `command$` | Yes | Music macro string: notes, octaves, tempo, length, rests, and articulation. |

### Comments

Notes play synchronously; `PLAY` blocks until the string finishes. `MB` is treated as `MF` (no background note queue). `Xstring;` is not supported. Tones are capped at five seconds. `PLAY` requires no graphics mode.

### Examples

#### Example 1 — Play a short scale

```basic
10 PLAY "T120 O4 L8 C D E F G A B"
20 END
```

<a id="play-function"></a>
## PLAY() Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the number of notes currently in the background music queue.

### Syntax

```basic
PLAY(n)
```

### Newbasic status

Not implemented because newbasic has no background note queue. `PLAY` is synchronous.

<a id="pmap-function"></a>
## PMAP Function

**Type:** Function  
**Status:** Supported  
**See also:** `WINDOW`, `POINT`

### Purpose

Maps a coordinate between world coordinates and screen pixels.

### Syntax

```basic
PMAP(coordinate, function)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `coordinate` | Yes | World or screen coordinate. |
| `function` | Yes | `0` maps x world→screen, `1` maps y world→screen, `2` maps x screen→world, `3` maps y screen→world. |

### Comments

`PMAP` is meaningful only while a `WINDOW` is active. Without one, coordinates are effectively identity mappings. The function number is required.

### Examples

#### Example 1 — Map a world coordinate

```basic
10 SCREEN 1
20 WINDOW (0,0)-(320,200)
30 PRINT PMAP(160,0)
40 PRINT PMAP(100,1)
50 END
```

<a id="point-function"></a>
## POINT Function

**Type:** Function  
**Status:** Supported  
**See also:** `PSET`, `PMAP`, `WINDOW`

### Purpose

Returns the colour attribute of a graphics pixel.

### Syntax

```basic
POINT(x, y)
POINT(function)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes (pixel form) | Pixel coordinates. |
| `function` | Yes (coordinate form) | `0` returns the last x, `1` the last y, `2` the last x under `WINDOW`, `3` the last y under `WINDOW`. |

### Comments

The pixel form returns `-1` for an out-of-range point. The coordinate form is useful after `DRAW` or `LINE` to query the current graphics point.

### Examples

#### Example 1 — Read a pixel

```basic
10 SCREEN 1
20 PSET (10,20),2
30 PRINT POINT(10,20)
40 PRINT POINT(0)
50 PRINT POINT(1)
60 END
```

<a id="poke-statement"></a>
## POKE Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Writes a byte to machine memory.

### Syntax

```basic
POKE address, value
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no addressable machine memory.

<a id="pos-function"></a>
## POS Function

**Type:** Function  
**Status:** Supported  
**See also:** `CSRLIN`, `LOCATE`, `PRINT`

### Purpose

Returns the current text cursor column.

### Syntax

```basic
POS(dummy)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `dummy` | Yes | Accepted for compatibility; the value is ignored. |

### Comments

The result is 1-based. `POS(0)` is conventional. `POS` reads the cursor; it does not move it.

### Examples

#### Example 1 — Column after printing

```basic
10 CLS
20 PRINT "ABC";
30 PRINT POS(0)
40 END
```

Output:

```text
 4
```

<a id="preset-statement"></a>
## PRESET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PSET`, `POINT`

### Purpose

Draws a point in the background colour.

### Syntax

```basic
PRESET [STEP] (x, y) [, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Point coordinates. `STEP` makes them relative. |
| `colour` | No | Attribute. Default is `0` (background). |

### Comments

`PRESET` is the inverse of `PSET`; it defaults to colour `0`. The point is clipped to the graphics canvas. `PRESET` updates the last graphics point.

### Examples

#### Example 1 — Erase a pixel

```basic
10 SCREEN 1
20 PSET (50,50),2
30 PRESET (50,50)
40 END
```

<a id="print-statement"></a>
## PRINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT USING`, `PRINT#`, `LPRINT`

### Purpose

Writes values to the screen.

### Syntax

```basic
PRINT [expression] [;|, ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | No | Value to print. |
| `;` | No | Concatenate the next item directly. |
| `,` | No | Advance to the next print zone. |

### Comments

Numbers are printed with a leading space for a positive sign and a trailing space. A trailing semicolon suppresses the newline. `PRINT` with no items prints a blank line. `?` is shorthand for `PRINT`.

### Examples

#### Example 1 — Print values

```basic
10 A=5
20 PRINT "A="; A
30 PRINT 1, 2, 3
40 PRINT
50 END
```

Output:

```text
A= 5
 1          2          3
```

<a id="print-using-statement"></a>
## PRINT USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT`, `LPRINT USING`

### Purpose

Prints values under the control of a format string.

### Syntax

```basic
PRINT USING format$; expression [, expression ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `format$` | Yes | Field format string. |
| `expression` | Yes | Values to format. |

### Comments

Numeric fields use `#`, decimal point, comma, `+`, `-`, `$$`, `**`, and `^^^^`. String fields use `&` for the whole string or `!` for one character. Literal text in the format is printed as-is. A field with no matching expression stops formatting.

### Examples

#### Example 1 — Format a number

```basic
10 PRINT USING "###.##"; 12.345
20 PRINT USING "$$###.##"; 12.5
30 PRINT USING "! ! !"; "ABC"
40 END
```

<a id="print-statement-2"></a>
## PRINT# Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `OPEN`, `WRITE#`, `INPUT#`

### Purpose

Writes formatted output to a sequential file.

### Syntax

```basic
PRINT #file number, [expression] [;|, ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `expression` | No | Values to write. |

### Comments

The file must be open for output or append. `PRINT#` does not add quotes around strings; use `WRITE#` for comma-separated quoted records. A trailing semicolon suppresses the newline.

### Examples

#### Example 1 — Write two lines

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "ONE"
30 PRINT #1, "TWO"
40 CLOSE #1
50 END
```

<a id="print-using-statement-2"></a>
## PRINT# USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT USING`, `PRINT#`

### Purpose

Writes formatted output to a sequential file.

### Syntax

```basic
PRINT #file number, USING format$; expression [, expression ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `format$` | Yes | Format string using `PRINT USING` rules. |
| `expression` | Yes | Values to format. |

### Comments

The file must be open for output. Formatting rules match `PRINT USING`; output goes to the file rather than the screen.

### Examples

#### Example 1 — Formatted file output

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, USING "###.##"; 12.345
30 CLOSE #1
40 END
```

<a id="pset-statement"></a>
## PSET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRESET`, `POINT`, `LINE`

### Purpose

Draws a point in a graphics colour.

### Syntax

```basic
PSET [STEP] (x, y) [, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Point coordinates. `STEP` makes them relative. |
| `colour` | No | Attribute. Default is the current foreground. |

### Comments

`PSET` requires a graphics mode; points are clipped to the canvas. The last graphics point becomes `(x,y)` for later `STEP` operations.

### Examples

#### Example 1 — Plot a point

```basic
10 SCREEN 1
20 PSET (160,100),2
30 PRINT POINT(160,100)
40 END
```

<a id="put-statement"></a>
## PUT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GET`, `FIELD`, `OPEN`, `PSET`

### Purpose

Writes a random-access file record, or places a graphics image on the screen.

### Syntax

```basic
PUT #file number [, record number]
PUT [STEP](x, y), array [, action]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes (file form) | Open random-access file. |
| `record number` | No | Record to write. Default is the next record. |
| `x`, `y` | Yes (graphics form) | Destination for the image. |
| `array` | Yes (graphics form) | Image array produced by `GET`. |
| `action` | No | `PSET`, `PRESET`, `AND`, `OR`, or `XOR`. Default `PSET`. |

### Comments

The file form writes the current `FIELD` buffer. The graphics form restores an image; the array layout is width, height, then row-major palette indices. `STEP` makes the destination relative.

### Examples

#### Example 1 — Write a record

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 LSET A$="HELLO"
40 PUT #1, 1
50 CLOSE #1
60 END
```

#### Example 2 — Restore a graphics image

```basic
10 SCREEN 1
20 CIRCLE (160,100),30,2
30 DIM G(6000)
40 GET (120,70)-(200,130), G
50 CLS
60 PUT (20,20), G
70 END
```
