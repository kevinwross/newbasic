# S

<a id="save-command"></a>
## SAVE Command

**Type:** Command  
**Status:** Supported (text only)  
**See also:** `LOAD`, `MERGE`

### Purpose

Saves the stored program to disk as plain text.

### Syntax

```basic
SAVE filename [, A]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | Destination file. |
| `A` | No | ASCII/plain-text form. This is the only supported form. |

### Comments

`SAVE` writes a plain-text `.BAS` file. The old `,B` (tokenized) and `,P`
(protected) forms are no longer supported and report **Bad file mode**.
Line numbers in the text file are signed 32-bit values.

### Examples

```text
SAVE "HELLO.BAS"
SAVE "HELLO.BAS", A
```

<a id="screen-function"></a>
## SCREEN Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the character or colour attribute at a text-screen position.

### Syntax

```basic
SCREEN(row, column [, colour flag])
```

### Newbasic status

Not implemented as a function. Newbasic exposes cursor position through `CSRLIN` and `POS`; text-grid access is not part of the dialect.

<a id="screen-statement"></a>
## SCREEN Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `COLOR`, `PALETTE`, `CLS`, `WIDTH`

### Purpose

Selects a text or graphics screen mode.

### Syntax

```basic
SCREEN [mode] [, [colour switch] [, [active page] [, [visible page]]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `mode` | No | `0`–`13`, or the newbasic extension `720`. Default is text mode. |
| `colour switch` | No | Accepted for compatibility; newbasic uses the mode’s palette. |
| `active page` | No | Accepted for compatibility; page flipping is not implemented. |
| `visible page` | No | Accepted for compatibility; page flipping is not implemented. |

### Comments

Modes `0`–`13` keep an 80×25 text grid. Mode `720` is newbasic’s extension: 1280×720, 160×45 text, and a 256-entry palette. A mode change resets the palette and viewport. Invalid modes raise **Illegal function call**.

### Examples

#### Example 1 — Graphics circle

```basic
10 SCREEN 1
20 CIRCLE (160,100),50
30 END
```

#### Example 2 — Wide extension mode

```basic
10 SCREEN 720
20 PRINT "1280 x 720"
30 END
```

<a id="screenh-function"></a>
## SCREENH Function

**Type:** Function  
**Status:** Supported (newbasic extension)  
**See also:** `SCREENW`, `SCREEN`

### Purpose

Returns the current graphics canvas height in pixels.

### Syntax

```basic
SCREENH
```

### Comments

In a text-only mode the canvas has no graphics pixels, so the result is `0`. Use `SCREEN` first to select a graphics mode.

### Examples

```basic
10 SCREEN 720
20 PRINT SCREENW; SCREENH
30 END
```

Output:

```text
 1280  720
```

<a id="screenw-function"></a>
## SCREENW Function

**Type:** Function  
**Status:** Supported (newbasic extension)  
**See also:** `SCREENH`, `SCREEN`

### Purpose

Returns the current graphics canvas width in pixels.

### Syntax

```basic
SCREENW
```

### Comments

In a text-only mode the canvas has no graphics pixels, so the result is `0`. Use `SCREEN` first to select a graphics mode.

<a id="select-case-statement"></a>
## SELECT CASE Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `IF`, `GOTO`

### Purpose

Selects one of several statement groups by comparing one selector expression.

### Syntax

```basic
SELECT CASE expression
CASE value [, value ...]
CASE IS comparison expression
CASE ELSE
END SELECT
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | Yes | Numeric or string selector, evaluated once. |
| `value` | Yes | A literal or expression compared for equality. |
| `comparison` | Yes | `=`, `<>`, `<`, `<=`, `>`, or `>=` after `CASE IS`. |

### Comments

Only the first matching `CASE` group runs. `CASE value, value` tests equality against each value. `CASE IS` performs one numeric comparison; use `CASE value` equality for strings. `CASE ELSE` runs when nothing matched and must be last. `END SELECT` closes the statement.

Ranges (`CASE 1 TO 5`) are not implemented. `SELECT CASE` is not allowed in direct mode.

A `GOTO` that leaves the active `SELECT CASE` block raises **Illegal function call** at run time with the message `GOTO would leave a SELECT CASE`. This is a runtime check because a computed `GOTO` target is not known when the program is compiled. The check is specifically about leaving the whole block; a `GOTO` to another line in the same active block is not an error, even though jumping between CASE arms is hard to follow. Jumping to the current block's `END SELECT` line is allowed and closes the block normally. Do not jump into a nested `SELECT CASE` that is not active; that raises `SELECT CASE stack mismatch` when the inner CASE or END is reached. `GOSUB` out of a CASE is allowed because `RETURN` comes back to the same block; however, a subroutine that opens a `SELECT CASE` and returns before its `END SELECT` raises `RETURN would leave a SELECT CASE`.

### Examples

#### Example 1 — Value list and ELSE

```basic
10 X = 2
20 SELECT CASE X
30 CASE 1
40   PRINT "ONE"
50 CASE 2, 3
60   PRINT "TWO"
70 CASE ELSE
80   PRINT "OTHER"
90 END SELECT
```

Output:

```text
TWO
```

#### Example 2 — Bad: `GOTO` out of the SELECT

```basic
10 X = 1
20 SELECT CASE X
30 CASE 1
40   GOTO 100
50 END SELECT
60 PRINT "AFTER"
70 END
100 PRINT "OUT"
```

This fails at line 40 with:

```text
Illegal function call: GOTO would leave a SELECT CASE
```

#### Example 3 — Good: jump to `END SELECT`

```basic
10 X = 1
20 SELECT CASE X
30 CASE 1
40   PRINT "ONE"
50   GOTO 70
60   PRINT "SKIPPED"
70 END SELECT
80 PRINT "AFTER"
90 END
```

Output:

```text
ONE
AFTER
```

GOTO to the `END SELECT` line is allowed because it executes the normal close of the block.

#### Example 4 — Good: call outside code with `GOSUB`

```basic
10 X = 1
20 SELECT CASE X
30 CASE 1
40   GOSUB 100
50   PRINT "IN CASE"
60 END SELECT
70 END
100 PRINT "SUBROUTINE"
110 RETURN
```

Output:

```text
SUBROUTINE
IN CASE
```

`GOSUB` does not leave the block permanently: `RETURN` comes back to line 50, inside the SELECT.

<a id="sgn-function"></a>
## SGN Function

**Type:** Function  
**Status:** Supported  
**See also:** `ABS`, `FIX`

### Purpose

Returns the sign of a number.

### Syntax

```basic
SGN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

Returns `-1` for negative, `0` for zero, and `1` for positive.

### Examples

#### Example 1 — Signs

```basic
10 PRINT SGN(-5); SGN(0); SGN(5)
20 END
```

Output:

```text
-1  0  1
```

<a id="shell-statement"></a>
## SHELL Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SYSTEM`

### Purpose

Runs a host command or starts a command interpreter.

### Syntax

```basic
SHELL [command$]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `command$` | No | Command string. Without it, newbasic starts an interactive shell. |

### Comments

`SHELL` hands control to the host OS. A bare `SHELL` requires an interactive terminal; with redirected input it reports a clear error rather than hanging. The command interpreter must be available on the host.

### Examples

#### Example 1 — Run a host command

```basic
10 SHELL "echo hello"
20 END
```

<a id="sin-function"></a>
## SIN Function

**Type:** Function  
**Status:** Supported  
**See also:** `COS`, `TAN`, `ATN`

### Purpose

Returns the sine of an angle in radians.

### Syntax

```basic
SIN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Angle in radians. |

### Comments

`SIN(0)` is `0`. Use `ATN(1)*4` for π if an approximation is not sufficient.

### Examples

#### Example 1 — Sine values

```basic
10 PRINT SIN(0)
20 PRINT SIN(3.14159265/2)
30 END
```

Output:

```text
 0
 1
```

<a id="sleep-statement"></a>
## SLEEP Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `TIMER`

### Purpose

Pauses execution for a number of milliseconds.

### Syntax

```basic
SLEEP milliseconds
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `milliseconds` | Yes | Non-negative duration. Fractions are rounded. |

### Comments

`SLEEP` blocks the interpreter through the host service. `SLEEP 0` is allowed and does not block. A negative value raises **Illegal function call**. Timer events are not serviced while a synchronous `SLEEP` is in progress; use short sleeps in an animation loop when responsiveness matters.

### Examples

```basic
10 PRINT "WAIT"
20 SLEEP 500
30 PRINT "DONE"
40 END
```

<a id="sound-statement"></a>
## SOUND Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `BEEP`, `PLAY`

### Purpose

Sounds a tone for a specified duration.

### Syntax

```basic
SOUND frequency, duration
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `frequency` | Yes | Tone frequency in Hz. |
| `duration` | Yes | Duration in clock ticks (about 18.2 per second). |

### Comments

`SOUND` blocks until the tone finishes. Frequencies are clamped to the supported range. The duration is capped for safety. Under a headless host the call is silent.

### Examples

#### Example 1 — Two tones

```basic
10 SOUND 440, 9
20 SOUND 880, 9
30 END
```

<a id="space-function"></a>
## SPACE$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `STRING$`, `SPC`

### Purpose

Returns a string of spaces.

### Syntax

```basic
SPACE$(count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of spaces. |

### Comments

`count` must be non-negative. `SPACE$(0)` is the empty string.

### Examples

#### Example 1 — Build padding

```basic
10 A$="["+SPACE$(5)+"]"
20 PRINT A$
30 END
```

<a id="spc-function"></a>
## SPC Function

**Type:** Function  
**Status:** Supported in PRINT only  
**See also:** `TAB`, `SPACE$`

### Purpose

Skips spaces within a `PRINT` or `LPRINT` item list.

### Syntax

```basic
SPC(count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of spaces to advance. |

### Comments

`SPC` is valid only inside `PRINT`, `LPRINT`, `PRINT#`, or `LPRINT USING` item lists. It moves the cursor without producing visible characters.

### Examples

#### Example 1 — Space output

```basic
10 PRINT "A"; SPC(5); "B"
20 END
```

Output:

```text
A     B
```

<a id="sqr-function"></a>
## SQR Function

**Type:** Function  
**Status:** Supported  
**See also:** `EXP`, `LOG`

### Purpose

Returns the square root of a non-negative number.

### Syntax

```basic
SQR(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Non-negative numeric expression. |

### Comments

A negative argument raises **Illegal function call**.

### Examples

#### Example 1 — Square roots

```basic
10 PRINT SQR(9)
20 PRINT SQR(2)
30 END
```

Output:

```text
 3
 1.414214
```

<a id="stack-statement"></a>
## STACK Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `CLEAR`, `SWAP`

### Purpose

Maintains a user-visible LIFO stack of BASIC values, including whole arrays. It is useful for manual parameter passing, recursive algorithms, and temporary value storage.

### Syntax

```basic
STACK PUSH source [, source]...
STACK POP target [, target]...
STACK TOP target [, target]...
STACK PEEK target [, target]...
STACK PEEK index, target
STACK DROP [count]
STACKAT(index)
STACKDEPTH
```

Each `source` is an expression or an array name followed by empty parentheses, such as `A()`. Each `target` is a scalar variable, an array element, or an array name followed by empty parentheses.

`STACK PEEK` with a target list is an alias for `STACK TOP`. `STACK PEEK index, target` is the indexed, non-destructive form.

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | Yes, for `PUSH` | Scalar value. Sources are evaluated left-to-right and pushed in that order. |
| `array()` | Yes, for whole arrays | Pushes a copy of the whole array. `A` without parentheses remains the scalar `A`; `A()` is the array. |
| `variable` | Yes, for `POP`/`TOP` | Assignable scalar variable or array element. Targets are assigned left-to-right. |
| `array()` target | Yes, for whole arrays | Creates or replaces the target array with a copy of the source array. |
| `index` | Yes, for indexed `PEEK`/`STACKAT` | Zero-based position from the top: `0` is the top entry, `1` the next entry down, and so on. |
| `count` | No, for `DROP` | Number of top entries to remove. Default is `1`. |

### Comments

`STACK` is a newbasic extension; GW-BASIC has no equivalent. There is one global stack per interpreter, and it is independent of the expression stack used internally by the VM.

`PUSH` evaluates its list left-to-right and pushes in that order, so the last source becomes the top of the stack. `POP` and `TOP` assign their target list left-to-right, with the first target receiving the top. The natural round trip is therefore:

```basic
STACK PUSH a, b, c
STACK POP  c, b, a
```

`POP` removes the values; `TOP`/`PEEK` leaves the stack unchanged.

<a id="stackat-function"></a>
`STACKAT(index)` reads a scalar from the stack without removing it. Index `0` is the top. It preserves the stored scalar type, so a string entry can be printed or assigned to a string variable. An out-of-range index raises **Subscript out of range**. A whole-array entry raises **Type mismatch** because a function cannot return a whole array; use indexed `PEEK` with an array target for that case.

<a id="stackdepth-function"></a>
`STACKDEPTH` returns the number of stack entries as a long integer. It takes no parentheses.

`STACK DROP` removes the top entry, or the top `count` entries when a count is given. `STACK DROP 0` is a no-op. A negative count raises **Illegal function call**. If `count` exceeds the number of entries, the statement raises **Stack empty** and leaves the entire stack intact. Fractional counts round to the nearest integer. Dropping whole-array entries also updates the internal element accounting.

The indexed form `STACK PEEK index, target` copies one entry without removing it. It supports scalar, array-element and whole-array targets, including `STACK PEEK 1, A()`. The index is zero-based from the top. Use parentheses for a computed index, for example `STACK PEEK (I), A`, because a bare variable as the first argument retains the old top-down target-list meaning.

Scalars keep their BASIC kind on the stack (integer, long, single, double, or string). Assigning to a scalar target uses the normal conversion rules: `A%` rounds and may raise **Overflow**, `A$` raises **Type mismatch** for a numeric value, and so on. Array-element targets work too, and an undeclared array element target creates the array at the default size with the current `OPTION BASE`.

A whole array pushed as `A()` is copied by value: its element type, lower bound, dimensions, and data are captured at the moment of `PUSH`. Changing `A()` afterwards does not change the stack. `PUSH A()` requires an existing array; an undimensioned array raises **Subscript out of range**. `POP B()` and `TOP B()` create or replace the whole array `B()`, preserving the source shape and lower bound. If `B` has a type suffix, each element is converted to that type; a failed element conversion leaves both the stack and `B()` unchanged.

Scalars and whole arrays can be mixed in one list:

```basic
STACK PUSH 5, A(), "x"
STACK POP  S$, B(), N%
```

List operations are **all-or-nothing**. If the stack has too few entries, a target conversion fails, an array subscript is out of range, or a whole-array type is incompatible, the whole statement fails and neither the stack nor any target is changed. All target subscripts are evaluated and checked before any assignment. Arrays that were auto-created during a failed list are removed again.

An underflowing operation raises **Stack empty** (newbasic error 77). The stack is capped at 1,000,000 entries and 1,000,000 total scalar/array elements; exceeding either limit raises **Out of memory**.

`CLEAR`, `RUN`, `LOAD`, `NEW`, and `CHAIN` clear the stack. Direct-mode statements preserve it between commands, so it can be used interactively. `GOSUB`/`RETURN` do not use or clear it.

### Examples

#### Example 1 — LIFO round trip

```basic
10 STACK PUSH 10, 20, 30
20 STACK POP C, B, A
30 PRINT A; B; C
40 END
```

Output:

```text
 10  20  30
```

#### Example 2 — Peek without removing

```basic
10 STACK PUSH "A", "B"
20 STACK TOP S2$, S1$
30 PRINT S1$; S2$
40 END
```

Output:

```text
AB
```

#### Example 3 — Array element targets

```basic
10 DIM A(2)
20 STACK PUSH 7, 8
30 STACK POP A(2), A(1)
40 PRINT A(1); A(2)
50 END
```

Output:

```text
 7  8
```

#### Example 4 — Copy a whole array

```basic
10 DIM A(10)
20 FOR I = 1 TO 10 : A(I) = I + 100 : NEXT I
30 STACK PUSH A()
40 STACK POP B()
50 FOR I = 1 TO 10 : PRINT B(I) : NEXT I
60 END
```

Output:

```text
 101
 102
 103
 104
 105
 106
 107
 108
 109
 110
```

#### Example 5 — Inspect a whole array without removing it

```basic
10 DIM A(2)
20 A(1) = 7 : A(2) = 8
30 STACK PUSH A()
40 STACK TOP B()
50 STACK POP C()
60 PRINT B(1); B(2); C(1); C(2)
70 END
```

Output:

```text
 7  8  7  8
```

#### Example 6 — Index, inspect and drop

```basic
10 STACK PUSH 10, "B", 30
20 IF STACKAT(0)=30 AND STACKAT(1)="B" AND STACKAT(2)=10 THEN PRINT "READ"
30 STACK DROP 2
40 IF STACKDEPTH=1 THEN PRINT "DROP"
50 END
```

Output:

```text
READ
DROP
```

#### Example 7 — Indexed PEEK of a whole array

```basic
10 DIM A(2)
20 A(1) = 7 : A(2) = 8
30 STACK PUSH 5, A()
40 STACK PEEK 0, B()
50 STACK PEEK 1, X
60 PRINT B(1); B(2); X; STACKDEPTH
70 END
```

Output:

```text
 7  8  5  2
```

<a id="stick-function"></a>
## STICK Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns a joystick axis position.

### Syntax

```basic
STICK(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick device.

<a id="stop-statement"></a>
## STOP Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `END`, `CONT`

### Purpose

Stops program execution and returns to direct mode.

### Syntax

```basic
STOP
```

### Comments

`STOP` saves a continue point; `CONT` can resume. `END` ends without a continue point. No error is reported.

### Examples

#### Example 1 — Stop and continue

```basic
10 PRINT "BEFORE"
20 STOP
30 PRINT "AFTER"
40 END
```

Run, then type `CONT` at the prompt to print `AFTER`.

<a id="str-function"></a>
## STR$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `VAL`, `PRINT`

### Purpose

Converts a number to its string representation.

### Syntax

```basic
STR$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

The result contains the same leading sign space that `PRINT` uses for positive numbers. If the leading space is not wanted, remove it with `MID$(s$, 2)`.

### Examples

#### Example 1 — Convert and concatenate

```basic
10 A=42
20 PRINT "VALUE=" + STR$(A)
30 END
```

<a id="strig-function"></a>
## STRIG Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the current or latched state of a joystick trigger.

### Syntax

```basic
STRIG(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick device.

<a id="strig-statement"></a>
## STRIG Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Enables, disables, or stops joystick-trigger trapping.

### Syntax

```basic
STRIG(n) ON
STRIG(n) OFF
STRIG(n) STOP
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick or trigger device.

<a id="string-function"></a>
## STRING$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `SPACE$`, `CHR$`

### Purpose

Returns a string consisting of repeated characters.

### Syntax

```basic
STRING$(count, character)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of characters. |
| `character` | Yes | Character code or one-character string. |

### Comments

`count` must be non-negative. A string argument uses its first character. A numeric argument is taken as a character code.

### Examples

#### Example 1 — Build a separator

```basic
10 PRINT STRING$(10, "-")
20 PRINT STRING$(5, 65)
30 END
```

Output:

```text
----------
AAAAA
```

<a id="swap-statement"></a>
## SWAP Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `LET`, `DIM`

### Purpose

Exchanges the values of two variables or array elements.

### Syntax

```basic
SWAP variable1, variable2
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable1`, `variable2` | Yes | Scalar variables or array elements of compatible type. |

### Comments

`SWAP` exchanges values without a temporary variable. Both targets must be assignable and of compatible type. It works with strings, numeric variables, and array elements.

### Examples

#### Example 1 — Swap two values

```basic
10 A=1: B=2
20 SWAP A, B
30 PRINT A; B
40 END
```

Output:

```text
 2  1
```

<a id="system-command"></a>
## SYSTEM Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `SHELL`

### Purpose

Exits newbasic and returns to the host operating system.

### Syntax

```basic
SYSTEM
```

### Comments

`SYSTEM` is a direct-mode command. It does not save the current program automatically. Use `SAVE` first.

### Examples

```text
SYSTEM
```
