# O

<a id="oct-function"></a>
## OCT$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `HEX$`, `STR$`

### Purpose

Returns the octal representation of a number.

### Syntax

```basic
OCT$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression; converted to an integer. |

### Comments

The result is uppercase octal with no leading `&O`. Negative values use two’s-complement form.

### Examples

#### Example 1 — Octal values

```basic
10 PRINT OCT$(8)
20 PRINT OCT$(64)
30 END
```

Output:

```text
10
100
```

<a id="on-comn-statement"></a>
## ON COM(n) Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Arms a communications-port event trap.

### Syntax

```basic
ON COM(n) GOSUB line
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no COM device or serial event source.

<a id="on-error-goto-statement"></a>
## ON ERROR GOTO Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `ERR`, `ERL`, `RESUME`, `ERROR`

### Purpose

Enables error trapping and names the handler line.

### Syntax

```basic
ON ERROR GOTO line
ON ERROR GOTO 0
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | Yes | First line of the error handler. `0` disables trapping. |

### Comments

Once enabled, a runtime error branches to the handler. `ERR` and `ERL` identify the error and line. A handler should finish with `RESUME`, `RESUME NEXT`, or `RESUME line`. `ON ERROR GOTO 0` disables trapping; errors then stop the program.

Newbasic does not trap errors inside an active handler; an error there terminates execution.

### Examples

#### Example 1 — Trap division by zero

```basic
10 ON ERROR GOTO 100
20 PRINT 1/0
30 PRINT "AFTER"
40 END
100 PRINT "CAUGHT"; ERR
110 RESUME NEXT
```

Output:

```text
CAUGHT 11
AFTER
```

<a id="on-expression-gosub-statement"></a>
## ON expression GOSUB Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOSUB`, `RETURN`, `ON ... GOTO`

### Purpose

Calls one of several subroutines, selected by the rounded value of an expression.

### Syntax

```basic
ON expression GOSUB line [, line ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | Yes | Selects a subroutine. Rounded to the nearest integer. |
| `line` | Yes | Existing line numbers, separated by commas. Each begins a subroutine. |

### Comments

The selection is 1-based. `ON 0` or an index beyond the list (up to 255) falls through to the next statement. A negative index or one greater than 255 raises **Illegal function call**. Each subroutine should end with `RETURN`. Direct-mode use is illegal.

### Examples

#### Example 1 — Dispatch to three subroutines

```basic
10 FOR I=1 TO 3
20 ON I GOSUB 100,200,300
30 NEXT I
40 END
100 PRINT "ONE": RETURN
200 PRINT "TWO": RETURN
300 PRINT "THREE": RETURN
```

Output:

```text
ONE
TWO
THREE
```

#### Example 2 — Zero falls through

```basic
10 ON 0 GOSUB 100
20 PRINT "NEXT"
30 END
100 PRINT "NOT CALLED": RETURN
```

Output:

```text
NEXT
```

<a id="on-expression-goto-statement"></a>
## ON expression GOTO Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOTO`, `ON ... GOSUB`

### Purpose

Branches to one of several line numbers, selected by the rounded value of an expression.

### Syntax

```basic
ON expression GOTO line [, line ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | Yes | Selects a target. Rounded to the nearest integer. |
| `line` | Yes | Existing line numbers, separated by commas. |

### Comments

The selection is 1-based. Zero or an index past the end falls through. Negative values and values above 255 raise **Illegal function call**. Unlike `ON ... GOSUB`, no return address is saved.

### Examples

#### Example 1 — Menu branch

```basic
10 A=2
20 ON A GOTO 100,200,300
30 END
100 PRINT "ONE": END
200 PRINT "TWO": END
300 PRINT "THREE": END
```

Output:

```text
TWO
```

<a id="on-keyn-statement"></a>
## ON KEY(n) Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `KEY(n)`, `INKEY$`

### Purpose

Names a subroutine to run when a key event is trapped.

### Syntax

```basic
ON KEY(n) GOSUB line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `n` | Yes | Key number `1`–`10`. |
| `line` | Yes | Handler line, ending in `RETURN`. |

### Comments

The trap fires only after `KEY(n) ON`. Newbasic’s input model delivers characters rather than function-key scan codes, so any pending character key can fire the lowest armed trap. The handler can read the character with `INKEY$`. This is not a true per-function-key trap.

### Examples

#### Example 1 — Trap a character

```basic
10 ON KEY(1) GOSUB 100
20 KEY(1) ON
30 PRINT "WAITING"
40 FOR I=1 TO 100: NEXT I
50 END
100 K$=INKEY$: PRINT "KEY="; K$: RETURN
```

<a id="on-pen-statement"></a>
## ON PEN Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Arms a light-pen event trap.

### Syntax

```basic
ON PEN GOSUB line
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no light-pen device.

<a id="on-playn-statement"></a>
## ON PLAY(n) Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Arms a background note-queue event trap.

### Syntax

```basic
ON PLAY(n) GOSUB line
```

### Newbasic status

Recognised and refused with **Advanced Feature**. Newbasic plays notes synchronously and has no background note queue.

<a id="on-strign-statement"></a>
## ON STRIG(n) Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Arms a joystick-trigger event trap.

### Syntax

```basic
ON STRIG(n) GOSUB line
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick or trigger device.

<a id="on-timern-statement"></a>
## ON TIMER(n) Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `TIMER`, `TIMER ON/OFF/STOP`

### Purpose

Arms a timer event trap.

### Syntax

```basic
ON TIMER(seconds) GOSUB line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `seconds` | Yes | Interval in seconds. |
| `line` | Yes | Handler line, ending in `RETURN`. |

### Comments

The timer is armed by `ON TIMER` and enabled with `TIMER ON`. The handler runs between statements when the interval has elapsed. Events are not serviced while the debugger is stepping or stopped. Use `TIMER OFF` or `TIMER STOP` to control it.

### Examples

#### Example 1 — Tick once

```basic
10 ON TIMER(1) GOSUB 100
20 TIMER ON
30 FOR I=1 TO 20000: NEXT I
40 TIMER OFF
50 END
100 PRINT "TICK": RETURN
```

<a id="open-com-statement"></a>
## OPEN "COM" Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Opens a serial communications port.

### Syntax

```basic
OPEN "COM1:9600,N,8,1" AS #1
```

### Newbasic status

Not implemented. Newbasic has no serial-port or modem interface.

<a id="open-statement"></a>
## OPEN Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `CLOSE`, `INPUT#`, `PRINT#`, `FIELD`

### Purpose

Opens a file for sequential or random access.

### Syntax

```basic
OPEN filename FOR mode AS #file number [LEN=record length]
OPEN mode, #file number, filename [, record length]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | File name or path. |
| `mode` | Yes | `INPUT`, `OUTPUT`, `APPEND`, or `RANDOM`. |
| `file number` | Yes | Integer handle used by `INPUT#`, `PRINT#`, `GET`, `PUT`, and `CLOSE`. |
| `record length` | No | Random-file record size. Default is `128`. |

### Comments

`OUTPUT` creates or truncates a file; `APPEND` adds to the end; `INPUT` requires an existing file; `RANDOM` creates a record file if needed. Opening an already open file in an incompatible mode raises an error. Long and short forms are accepted.

### Examples

#### Example 1 — Write and read a sequential file

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "HELLO"
30 CLOSE #1
40 OPEN "TEST.DAT" FOR INPUT AS #1
50 LINE INPUT #1, A$
60 CLOSE #1
70 PRINT A$
80 END
```

Output:

```text
HELLO
```

<a id="option-base-statement"></a>
## OPTION BASE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DIM`

### Purpose

Sets the default lower bound for arrays.

### Syntax

```basic
OPTION BASE 0
OPTION BASE 1
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `0` or `1` | Yes | Lower bound for arrays declared after this statement. |

### Comments

The default is `0`. `OPTION BASE` must appear before the arrays it affects. Newbasic allows only one base value per program; changing it after arrays are declared raises an error.

### Examples

#### Example 1 — One-based array

```basic
10 OPTION BASE 1
20 DIM A(3)
30 A(1)=7
40 PRINT A(1)
50 END
```

<a id="out-statement"></a>
## OUT Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Writes a byte to a hardware I/O port.

### Syntax

```basic
OUT port, value
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no x86 I/O-port model.
