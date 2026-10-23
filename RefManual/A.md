# A

<a id="abs-function"></a>
## ABS Function

**Type:** Function  
**Status:** Supported  
**See also:** `SGN`, `FIX`, `INT`

### Purpose

Returns the absolute value of a number.

### Syntax

```basic
ABS(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

`ABS(-3.5)` returns `3.5`; `ABS(3.5)` returns `3.5`. The result is numeric, with the same magnitude as the argument.

### Examples

#### Example 1 — Absolute values

```basic
10 PRINT ABS(-7)
20 PRINT ABS(7)
30 PRINT ABS(-2.5)
40 END
```

Output:

```text
 7
 7
 2.5
```

<a id="asc-function"></a>
## ASC Function

**Type:** Function  
**Status:** Supported  
**See also:** `CHR$`

### Purpose

Returns the numeric code of the first character in a string.

### Syntax

```basic
ASC(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | A non-empty string expression. |

### Comments

Only the first character is examined. An empty string raises **Illegal function call**. Newbasic stores strings as bytes; printable ASCII values are returned directly, and CP437 bytes are preserved.

### Examples

#### Example 1 — Character codes

```basic
10 PRINT ASC("A")
20 PRINT ASC("ABC")
30 END
```

Output:

```text
 65
 65
```

<a id="atn-function"></a>
## ATN Function

**Type:** Function  
**Status:** Supported  
**See also:** `SIN`, `COS`, `TAN`

### Purpose

Returns the arctangent of a number, in radians.

### Syntax

```basic
ATN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

The result is in the range `-π/2` to `π/2`. Use `ATN` for angle calculations and `TAN` for the inverse operation.

### Examples

#### Example 1 — Arctangent of one

```basic
10 PRINT ATN(1)
20 PRINT ATN(0)
30 END
```

Output:

```text
 .7853982
 0
```

<a id="auto-command"></a>
## AUTO Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `EDIT`, `RENUM`, `LIST`

### Purpose

Generates line numbers automatically while entering a program.

### Syntax

```basic
AUTO [start][, increment]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `start` | No | First line number. Default is `10`. |
| `increment` | No | Step between generated numbers. Default is `10`. |

### Comments

`AUTO` is a direct-mode command. Type a line after each generated number; the number and a trailing space are printed as a prompt, and the text appears on the same line. Press Enter on an empty line to finish. A line that already contains its own line number also ends the sequence. The lines are added to the current program; they are not executed until `RUN`.

### Examples

#### Example 1 — Enter two numbered lines

```text
AUTO 100,10
100 PRINT "AUTO"
110 END
120
```

Then run the program:

```text
RUN
```

Output:

```text
AUTO
```
