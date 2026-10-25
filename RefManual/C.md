# C

<a id="call-statement"></a>
## CALL Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `SUB`, `FUNCTION`, `LOCAL`, `EXIT`

### Purpose

Calls a `SUB` procedure. The call creates a fresh procedure frame with its own
parameters and `LOCAL` variables.

### Syntax

```basic
CALL name
CALL name(argument [, argument ...])
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | A `SUB` name, not a `FUNCTION` name. |
| `argument` | Depends | Expression passed by value, left to right. The count must match the `SUB` parameter list. |

### Comments

`CALL` is the newbasic BASIC-procedure call; it is not GW-BASIC's
machine-language `CALL`. There is no native-code or memory-address model.
`USR` remains unsupported.

The target `SUB` may be defined later in the program. The compiler pre-scans
procedure headers, so forward calls and exact argument-count checks work.

Arguments are evaluated in the caller's scope before the frame is created and
are converted to each parameter's declared type using normal assignment rules.
A string/numeric mismatch raises **Type mismatch**.

A procedure frame holds its parameters and `LOCAL` names. A callee cannot see
the caller's locals; undeclared names resolve to globals. See `LOCAL`.

A procedure may call itself or another procedure. The call depth is limited;
exceeding it raises **Call stack overflow** (newbasic error 79).

`CALL` is illegal in direct mode. `CALL` on a `FUNCTION` is a syntax error; use
the function in an expression instead.

### Examples

#### Example 1 — Call a sub with a parameter

```basic
10 X% = 5
20 CALL Show(X%)
30 END
100 SUB Show(N%)
110 LOCAL T%
120 T% = N% + 1
130 PRINT T%
140 END SUB
```

Output:

```text
 6
```

#### Example 2 — Forward call

```basic
10 CALL Later
20 END
100 SUB Later
110 PRINT "CALLED"
120 END SUB
```

Output:

```text
CALLED
```

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

Fractions are rounded to the nearest integer. The result is a signed 32-bit integer (`-2147483648` to `2147483647`). Values outside that range raise **Overflow**.

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
| `n` | No | `0` clears the text area and the whole graphics canvas; `1` clears only the graphics viewport; `2` clears only the text window. Default is `0`. |

### Comments

In text mode `CLS` fills the active text area with spaces, resets the text attribute to the default white-on-black, and homes the cursor to its top-left. `CLS 2` clears only the text window.

`CLS` and `CLS 0` clear the whole graphics canvas even when a `VIEW` is active. A graphics `VIEW` remains active after the clear, so subsequent drawing is still clipped to the viewport and uses viewport-relative coordinates.

`CLS 1` clears only the active graphics viewport; pixels outside it are untouched. The viewport is filled with its `VIEW` fill colour when one was specified, otherwise with the current background attribute. The viewport remains active and its coordinate mapping is unchanged. With no graphics viewport, `CLS 1` clears the whole graphics canvas.

When `VIEW PRINT` is active, the text side of `CLS` and `CLS 0` clears only that window and leaves rows outside it untouched. The cursor moves to the window’s first row, so subsequent printing stays inside the window instead of starting at screen row 1.

### Examples

#### Example 1 — Clear and print

```basic
10 CLS
20 PRINT "CLEAR"
30 END
```

#### Example 2 — Clear only the graphics viewport

```basic
10 SCREEN 1
20 PSET (10,10),2
30 VIEW (50,50)-(150,120)
40 PSET (5,5),3
50 CLS 1
60 VIEW
70 END
```

The pixel at (10,10) survives; the viewport interior is cleared. After `CLS`, drawing is still relative to the viewport until `VIEW` resets it.

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

The standard 16-colour CGA palette uses these attribute values:

| Value | Colour | RGB |
|---:|---|---|
| 0 | Black | 0, 0, 0 |
| 1 | Blue | 0, 0, 170 |
| 2 | Green | 0, 170, 0 |
| 3 | Cyan | 0, 170, 170 |
| 4 | Red | 170, 0, 0 |
| 5 | Magenta | 170, 0, 170 |
| 6 | Brown | 170, 85, 0 |
| 7 | Light gray | 170, 170, 170 |
| 8 | Dark gray | 85, 85, 85 |
| 9 | Light blue | 85, 85, 255 |
| 10 | Light green | 85, 255, 85 |
| 11 | Light cyan | 85, 255, 255 |
| 12 | Light red | 255, 85, 85 |
| 13 | Light magenta | 255, 85, 255 |
| 14 | Yellow | 255, 255, 85 |
| 15 | White | 255, 255, 255 |

The table describes the default CGA display. A graphics mode may remap its attributes: `COLOR ,n` selects the alternate three-colour set in `SCREEN 1`, and `PALETTE` can change any entry's displayed RGB value without changing the stored attribute numbers.

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

<a id="const-statement"></a>
## CONST Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `LET`, `DEFINT`

### Purpose

Declares a read-only global constant and initializes it before the program
starts.

### Syntax

```basic
CONST name = literal
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | A variable name, optionally with a type suffix. |
| `literal` | Yes | A number (optionally signed) or a quoted string. No expression. |

### Comments

If the name has no suffix, the suffix is inferred from the literal: an integer
literal makes a 32-bit integer (`%`), a decimal or `E` exponent makes a single
(`!`), a `D` exponent or `#` literal makes a double (`#`), and a string makes a
string (`$`). An
explicit suffix wins and converts the literal to that type; integer constants
round and range-check exactly like an assignment.

The value is installed before the first instruction, so a program may read a
constant textually before its `CONST` line. `CLEAR`, `RUN`, and `LOAD`
reinitialize the constants the same way.

Constants are read-only. Assigning to one is a compile error, including
`LET`/implicit assignment, `FOR`, `INPUT`, `READ`, `DIM`, `SWAP`,
`LSET`/`RSET`, `GET` image arrays, and `STACK POP`/`STACK TOP`. Duplicate
declarations and type/range/overflow errors are also reported at compile time.
A different type suffix is a different variable; use the declared spelling.

Put `CONST` at the start of its line so forward references can be collected.
The declaration itself emits no code and may share its line with other
statements after a colon. `CONST` is not allowed in direct mode.

### Examples

#### Example 1 — Numeric and string constants

```basic
10 CONST MAX% = 100
20 CONST TITLE$ = "COUNT"
30 PRINT TITLE$; MAX
40 PRINT ANSWER
50 CONST ANSWER% = 42
```

Output:

```text
COUNT 100
42
```

#### Example 2 — Type suffix

```basic
10 CONST BASE& = 40000
20 CONST RATE! = 1.5
30 PRINT BASE&; RATE!
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
