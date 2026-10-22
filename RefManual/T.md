# T

<a id="tab-function"></a>
## TAB Function

**Type:** Function  
**Status:** Supported in PRINT only  
**See also:** `SPC`, `POS`

### Purpose

Moves the print cursor to a specified column.

### Syntax

```basic
TAB(column)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `column` | Yes | 1-based destination column. |

### Comments

`TAB` is valid only inside `PRINT`, `LPRINT`, `PRINT#`, or `LPRINT USING` item lists. If the target column is to the left of the current column, it moves to the next line.

### Examples

#### Example 1 — Align columns

```basic
10 PRINT "NAME"; TAB(15); "AGE"
20 PRINT "ALICE"; TAB(15); 30
30 END
```

<a id="tan-function"></a>
## TAN Function

**Type:** Function  
**Status:** Supported  
**See also:** `SIN`, `COS`, `ATN`

### Purpose

Returns the tangent of an angle in radians.

### Syntax

```basic
TAN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Angle in radians. |

### Comments

The result is unbounded near odd multiples of π/2. `ATN` is the inverse.

### Examples

#### Example 1 — Tangent of zero

```basic
10 PRINT TAN(0)
20 END
```

Output:

```text
 0
```

<a id="time-function"></a>
## TIME$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `DATE$`, `TIMER`

### Purpose

Returns the current time as a string.

### Syntax

```basic
TIME$
```

### Arguments

None.

### Comments

The result is an eight-character `HH:MM:SS` string in 24-hour form. `TIME$` is a function in newbasic; assigning to it is not supported.

### Examples

#### Example 1 — Print the time

```basic
10 PRINT TIME$
20 END
```

<a id="timer-function"></a>
## TIMER Function

**Type:** Function  
**Status:** Supported  
**See also:** `ON TIMER`, `TIMER ON/OFF/STOP`

### Purpose

Returns the number of seconds since midnight.

### Syntax

```basic
TIMER
```

### Arguments

None.

### Comments

The value is a single-precision number with fractions of a second. It is useful for elapsed-time measurements.

### Examples

#### Example 1 — Measure a loop

```basic
10 T=TIMER
20 FOR I=1 TO 1000: NEXT I
30 PRINT TIMER-T
40 END
```

<a id="timer-statement"></a>
## TIMER Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `ON TIMER`, `TIMER`

### Purpose

Enables, disables, or stops timer-event trapping.

### Syntax

```basic
TIMER ON
TIMER OFF
TIMER STOP
```

### Comments

`ON TIMER(interval) GOSUB line` must be executed first. `TIMER ON` begins servicing the handler; `TIMER OFF` disables it; `TIMER STOP` retains pending events. Timer events are not serviced while the debugger is stepping or stopped.

### Examples

#### Example 1 — Timer event

```basic
10 ON TIMER(1) GOSUB 100
20 TIMER ON
30 FOR I=1 TO 20000: NEXT I
40 TIMER OFF
50 END
100 PRINT "TICK": RETURN
```

<a id="troff-command"></a>
## TROFF Command

**Type:** Command  
**Status:** Supported  
**See also:** `TRON`

### Purpose

Turns off line-number tracing.

### Syntax

```basic
TROFF
```

### Comments

`TROFF` turns off the trace mode enabled by `TRON`. It can be used at the prompt or in a program.

### Examples

#### Example 1 — Trace a section

```basic
10 TRON
20 A=1
30 TROFF
40 PRINT A
50 END
```

<a id="tron-command"></a>
## TRON Command

**Type:** Command  
**Status:** Supported  
**See also:** `TROFF`

### Purpose

Turns on line-number tracing.

### Syntax

```basic
TRON
```

### Comments

In trace mode, newbasic prints `[line]` as execution enters a new line. The trace goes to the screen and can be turned off with `TROFF`.

### Examples

#### Example 1 — Trace two lines

```basic
10 TRON
20 A=1
30 TROFF
40 PRINT A
50 END
```
