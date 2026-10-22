# C

<a id="call-statement"></a>
## CALL Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Transfers control to a machine-language subroutine.

### Syntax

```basic
CALL address[(variable [, variable ...])]
```

### Newbasic status

Recognised but deliberately refused with **Advanced Feature**. Newbasic has no machine-memory or native-call model. Use ordinary BASIC subroutines with `GOSUB`/`RETURN`.

<a id="cdbl-function"></a>
## CDBL Function

**Type:** Function  
**Status:** Supported  
**See also:** `CSNG`, `CINT`

### Purpose

Converts a numeric expression to double precision.

### Syntax

```basic
CDBL(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

Newbasic stores numeric values in host floating point; `CDBL` is accepted for compatibility and returns the same numeric value.

### Examples

#### Example 1 — Convert and print

```basic
10 PRINT CDBL(12.5)
20 END
```

<a id="chain-statement"></a>
## CHAIN Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `COMMON`, `LOAD`, `RUN`

### Purpose

Loads and runs another BASIC program, optionally preserving selected variables.

### Syntax

```basic
CHAIN filename [, line]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | String expression naming the program to load. |
| `line` | No | Line at which to start the new program. |

### Comments

The basic `CHAIN` form is supported. `CHAIN MERGE` and `CHAIN ...,ALL` are not; `COMMON` of arrays is also refused. `COMMON` of scalars is supported. A chained program does not inherit `DEF FN` definitions or default type declarations; restate them in the new program.

### Examples

#### Example 1 — Chain with a start line

```basic
10 CHAIN "PART2.BAS", 100
20 END
```

<a id="chdir-command"></a>
## CHDIR Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Changes the current working directory.

### Syntax

```basic
CHDIR path
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `path` | Yes | Directory to make current. |

### Comments

Implemented as a direct-mode command. It is deliberately not accepted inside a stored program, because a program-wide directory change would affect every later relative file operation.

### Examples

```text
CHDIR "samples"
FILES
```

<a id="chr-function"></a>
## CHR$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `ASC`

### Purpose

Returns the one-character string for a character code.

### Syntax

```basic
CHR$(code)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `code` | Yes | Numeric expression in the byte range `0`–`255`. |

### Comments

Values outside `0`–`255` raise **Illegal function call**. `CHR$(13)` is carriage return and `CHR$(10)` is line feed. CP437 bytes are preserved.

### Examples

#### Example 1 — Build a quoted line

```basic
10 A$ = CHR$(34) + "HI" + CHR$(34)
20 PRINT A$
30 END
```

Output:

```text
"HI"
```

<a id="cint-function"></a>
## CINT Function

**Type:** Function  
**Status:** Supported  
**See also:** `FIX`, `INT`, `CSNG`

### Purpose

Converts a numeric expression to an integer by rounding.

### Syntax

```basic
CINT(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

Fractions are rounded to the nearest integer. Values outside the integer range can raise **Overflow**.

### Examples

#### Example 1 — Round values

```basic
10 PRINT CINT(2.4)
20 PRINT CINT(2.6)
30 END
```

Output:

```text
 2
 3
```

<a id="circle-statement"></a>
## CIRCLE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SCREEN`, `COLOR`, `PSET`, `PAINT`

### Purpose

Draws a circle, ellipse, or arc on a graphics screen.

### Syntax

```basic
CIRCLE [STEP] (x, y), radius [, [color] [, [start] [, end] [, aspect]]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Centre coordinates. With `STEP`, relative to the last graphics point. |
| `radius` | Yes | Radius. Must be zero or greater. |
| `color` | No | Palette/attribute number. Default is `1`. |
| `start` | No | Start angle in radians. Default is `0`. |
| `end` | No | End angle in radians. Default is `2π`. |
| `aspect` | No | Ratio of the y radius to the x radius. Default `0` uses the screen mode’s display aspect. |

### Comments

`CIRCLE` requires a graphics mode, for example `SCREEN 1`. An invalid colour, a negative radius, or a non-positive aspect raises **Illegal function call**.

`start` and `end` are radians. A negative endpoint draws a radius line to that endpoint and is then treated as positive. If `end` is less than or equal to `start`, a full turn is added to `end`.

`aspect` below `1` measures the radius in x pixels; above `1` measures it in y pixels. `STEP` uses the last graphics point as the centre. After `CIRCLE`, the centre becomes the last point.

### Examples

#### Example 1 — Smallest runnable circle

```basic
10 SCREEN 1
20 CIRCLE (160,100),50
30 END
```

#### Example 2 — Colour and arc

```basic
10 SCREEN 1
20 CIRCLE (160,100),50,2
30 CIRCLE (160,100),50,3,0,1.5707963
40 END
```

#### Example 3 — Explicit aspect ratio

```basic
10 SCREEN 1
20 CIRCLE (160,100),50,2,,,1
30 END
```

#### Example 4 — Relative centre with STEP

```basic
10 SCREEN 1
20 PSET (80,80),1
30 CIRCLE STEP(0,0),40,2
40 END
```

<a id="clear-statement"></a>
## CLEAR Statement

**Type:** Statement / Command  
**Status:** Partial  
**See also:** `DIM`, `ERASE`, `NEW`

### Purpose

Clears program variables, arrays, stacks, open files, and print state.

### Syntax

```basic
CLEAR
CLEAR [, numeric expression]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| numeric expression | No | Accepted syntactically, but not supported by newbasic. |

### Comments

The no-argument form is supported both in a program and in direct mode. It clears variables, arrays, `DATA` state, open files, and print state, then continues at the next instruction. The argument form raises **Advanced Feature**.

### Examples

#### Example 1 — Clear variables mid-program

```basic
10 A=7
20 CLEAR
30 PRINT A
40 END
```

Output:

```text
 0
```

<a id="close-statement"></a>
## CLOSE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `OPEN`, `LOC`, `LOF`

### Purpose

Closes one or all open files.

### Syntax

```basic
CLOSE [#file number [, #file number ...]]
CLOSE
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | No | File number to close. With no numbers, all files are closed. |

### Comments

`CLOSE` flushes and closes the file. Closing an already closed file is harmless. Use `CLOSE` before reopening a file for a different mode.

### Examples

#### Example 1 — Write and close

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 END
```

<a id="cls-statement"></a>
## CLS Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SCREEN`, `VIEW`, `COLOR`

### Purpose

Clears the text screen or graphics screen.

### Syntax

```basic
CLS [n]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `n` | No | `0` clears the whole screen; `1` clears the graphics viewport; `2` clears only the text window. Default is `0`. |

### Comments

In text mode `CLS` fills the text grid with spaces and homes the cursor. In graphics mode it clears the graphics canvas and the text overlay. `CLS 2` clears only the text window.

### Examples

#### Example 1 — Clear and print

```basic
10 CLS
20 PRINT "CLEAR"
30 END
```

<a id="color-statement"></a>
## COLOR Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SCREEN`, `PALETTE`

### Purpose

Sets the current text or graphics colour.

### Syntax

```basic
COLOR [foreground] [, [background] [, border]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `foreground` | No | Text foreground or graphics attribute. |
| `background` | No | Text background. |
| `border` | No | Accepted for compatibility; text screens ignore it. |

### Comments

With no arguments, `COLOR` restores the mode defaults. In text mode, values are text attributes. In graphics mode, the colour is the drawing attribute. Border colour is parsed but ignored on the text renderer.

### Examples

#### Example 1 — Coloured text

```basic
10 COLOR 14, 1
20 PRINT "YELLOW ON BLUE"
30 COLOR 7, 0
40 END
```

<a id="comn-statement"></a>
## COM(n) Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Enables, disables, or stops trapping for a communications port.

### Syntax

```basic
COM(n) ON
COM(n) OFF
COM(n) STOP
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no COM port, serial device, or communications trap in newbasic. `ON COM(n)` is likewise unsupported.

<a id="common-statement"></a>
## COMMON Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `CHAIN`

### Purpose

Declares variables that survive a `CHAIN`.

### Syntax

```basic
COMMON variable [, variable ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable` | Yes | Scalar variable names to carry across a chain. |

### Comments

Scalar `COMMON` works. `COMMON` of arrays raises **Advanced Feature**. The carried variables are copied into the new program; restate type declarations and `DEF FN` definitions in the chained program.

### Examples

#### Example 1 — Carry a scalar

```basic
10 COMMON TOTAL
20 TOTAL = 42
30 CHAIN "PART2.BAS"
```

<a id="cont-command"></a>
## CONT Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Continues execution after a `STOP`, `END`, or Ctrl-Break.

### Syntax

```basic
CONT
```

### Comments

`CONT` is a direct-mode command. It resumes at the saved statement if the stopped program is still loaded. It cannot continue after a program has been edited or after an error that cleared the continue state.

### Examples

```text
RUN
STOP
CONT
```

<a id="cos-function"></a>
## COS Function

**Type:** Function  
**Status:** Supported  
**See also:** `SIN`, `TAN`, `ATN`

### Purpose

Returns the cosine of an angle in radians.

### Syntax

```basic
COS(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Angle in radians. |

### Comments

Use `ATN(1)*4` or the `PI` constant in your own program if you need π. `COS(0)` returns `1`.

### Examples

#### Example 1 — Cosine values

```basic
10 PRINT COS(0)
20 PRINT COS(3.14159265)
30 END
```

Output:

```text
 1
-1
```

<a id="csng-function"></a>
## CSNG Function

**Type:** Function  
**Status:** Supported  
**See also:** `CDBL`, `CINT`

### Purpose

Converts a numeric expression to single precision.

### Syntax

```basic
CSNG(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

Newbasic uses host floating point. `CSNG` is accepted for compatibility; it does not change the stored floating-point representation.

### Examples

#### Example 1 — Convert and print

```basic
10 PRINT CSNG(1.25)
20 END
```

<a id="csrlin-function"></a>
## CSRLIN Function

**Type:** Function  
**Status:** Supported  
**See also:** `POS`, `LOCATE`

### Purpose

Returns the current text cursor row.

### Syntax

```basic
CSRLIN
```

### Arguments

None.

### Comments

The row is 1-based. `CSRLIN` is a function, so it can be used in expressions and assignments; it does not move the cursor.

### Examples

#### Example 1 — Read the cursor row

```basic
10 LOCATE 5, 10
20 PRINT CSRLIN
30 END
```

Output:

```text
 5
```

<a id="cvd-function"></a>
## CVD Function

**Type:** Function  
**Status:** Supported  
**See also:** `MKD$`, `CVI`, `CVS`

### Purpose

Converts an 8-byte packed string to a double-precision number.

### Syntax

```basic
CVD(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Eight-byte string produced by `MKD$`. |

### Comments

`CVD` is the inverse of `MKD$`. The string must contain at least eight bytes; a short string raises **Illegal function call**.

### Examples

#### Example 1 — Round-trip a double

```basic
10 A# = 1.5
20 S$ = MKD$(A#)
30 PRINT CVD(S$)
40 END
```

<a id="cvi-function"></a>
## CVI Function

**Type:** Function  
**Status:** Supported  
**See also:** `MKI$`, `CVD`, `CVS`

### Purpose

Converts a 2-byte packed string to an integer.

### Syntax

```basic
CVI(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Two-byte string produced by `MKI$`. |

### Comments

`CVI` is the inverse of `MKI$`. The string must contain at least two bytes. The value is signed, as in GW-BASIC.

### Examples

#### Example 1 — Round-trip an integer

```basic
10 A% = 1234
20 S$ = MKI$(A%)
30 PRINT CVI(S$)
40 END
```

<a id="cvs-function"></a>
## CVS Function

**Type:** Function  
**Status:** Supported  
**See also:** `MKS$`, `CVD`, `CVI`

### Purpose

Converts a 4-byte packed string to a single-precision number.

### Syntax

```basic
CVS(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Four-byte string produced by `MKS$`. |

### Comments

`CVS` is the inverse of `MKS$`. The string must contain at least four bytes.

### Examples

#### Example 1 — Round-trip a single

```basic
10 A! = 2.5
20 S$ = MKS$(A!)
30 PRINT CVS(S$)
40 END
```
