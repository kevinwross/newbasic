# F

<a id="field-statement"></a>
## FIELD Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `OPEN`, `GET`, `PUT`, `LSET`, `RSET`

### Purpose

Binds string variables to fields inside a random-access file buffer.

### Syntax

```basic
FIELD #file number, width AS variable [, width AS variable ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | An open random-access file. |
| `width` | Yes | Field width in bytes. Must be a constant. |
| `variable` | Yes | String variable that views that field. |

### Comments

Field widths must be constants; a computed width raises **Advanced Feature**. The total field widths cannot exceed the record length. `LSET` and `RSET` write into the buffer; `GET` and `PUT` transfer it to or from disk.

### Examples

#### Example 1 — Define two fields

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=30
20 FIELD #1, 10 AS N$, 20 AS A$
30 LSET N$="ALICE"
40 LSET A$="123 MAIN ST"
50 PUT #1, 1
60 CLOSE #1
70 END
```

<a id="files-command"></a>
## FILES Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Lists files in the current directory.

### Syntax

```basic
FILES [pattern]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `pattern` | No | Accepted syntactically, but newbasic lists the directory and does not filter by pattern. |

### Comments

`FILES` is a direct-mode command and is deliberately not accepted inside a program. It prints names in sorted order.

### Examples

```text
FILES
FILES "*.BAS"
```

<a id="fix-function"></a>
## FIX Function

**Type:** Function  
**Status:** Supported  
**See also:** `INT`, `CINT`

### Purpose

Truncates a number toward zero.

### Syntax

```basic
FIX(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

`FIX` removes the fractional part without rounding. For negative values it truncates toward zero, unlike `INT`, which rounds down.

### Examples

#### Example 1 — Compare FIX and INT

```basic
10 PRINT FIX(2.9); INT(2.9)
20 PRINT FIX(-2.9); INT(-2.9)
30 END
```

Output:

```text
 2  2
-2 -3
```

<a id="for-statement"></a>
## FOR Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `NEXT`, `WHILE`, `WEND`

### Purpose

Repeats a block of statements a fixed number of times.

### Syntax

```basic
FOR variable = start TO end [STEP step]
...
NEXT [variable]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable` | Yes | Numeric loop variable. |
| `start` | Yes | Initial value. |
| `end` | Yes | Limit value. |
| `step` | No | Increment. Default is `1`. |

### Comments

The limit is checked before the first iteration, so a loop whose start is already past the limit does not execute. `NEXT` without a variable matches the innermost `FOR`. A negative step counts downward. `NEXT` after the loop variable is optional, but matching names make nested loops clearer.

### Examples

#### Example 1 — Count upward

```basic
10 FOR I=1 TO 3
20 PRINT I
30 NEXT I
40 END
```

Output:

```text
 1
 2
 3
```

#### Example 2 — Count downward

```basic
10 FOR I=3 TO 1 STEP -1
20 PRINT I
30 NEXT I
40 END
```

Output:

```text
 3
 2
 1
```

<a id="fre-function"></a>
## FRE Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the number of free bytes in BASIC’s string/numeric memory.

### Syntax

```basic
FRE(number)
FRE(string$)
```

### Newbasic status

Recognised but refused with **Advanced Feature**. Newbasic uses host memory rather than a fixed 64K BASIC data space, so a free-byte count would be meaningless.

<a id="function-statement"></a>
## FUNCTION Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `SUB`, `CALL`, `LOCAL`, `EXIT`, `DEF FN`

### Purpose

Defines a statement-bodied function with parameters, local variables, and a
typed return value.

### Syntax

```basic
FUNCTION name%(parameter [, parameter ...])
    [LOCAL name [, name ...]]...
    statements
    name% = expression
END FUNCTION
```

`name%` may use `%`, `&`, `!`, `#`, or `$`; the suffix is required and selects
the result type.

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name%` | Yes | Function name, including its type suffix. |
| `parameter` | No | Parameter name with an optional type suffix. Parameters are passed by value. |

### Comments

A function is called in an expression:

```basic
PRINT Twice%(4)
```

The result is the function-named local variable. Assign it before `END
FUNCTION`, or use `EXIT FUNCTION`. Falling into `END FUNCTION` with no
assignment returns the type's zero value (`0` or `""`).

Parameters and `LOCAL` names are private to each call, so recursion works.
Undeclared names are globals; a callee cannot see its caller's locals.

A function may call itself and may call `SUB`s. `CALL` cannot be used on a
function. `FUNCTION` definitions may not be nested, may not appear in direct
mode, and cannot be used inside another procedure.

A function name may not also be used as a scalar or array. Array parameters,
`BYREF`, optional parameters, and local arrays are not implemented in this
release.

See `SUB`, `LOCAL`, and `EXIT` for the shared procedure rules.

### Examples

#### Example 1 — A typed function

```basic
10 PRINT Clamp%(150, 0, 100)
20 END
100 FUNCTION Clamp%(V%, LO%, HI%)
110 LOCAL R%
120 R% = V%
130 IF R% < LO% THEN R% = LO%
140 IF R% > HI% THEN R% = HI%
150 Clamp% = R%
160 END FUNCTION
```

Output:

```text
 100
```

#### Example 2 — Recursion

```basic
10 PRINT Fact%(5)
20 END
100 FUNCTION Fact%(N%)
110 IF N% <= 1 THEN Fact% = 1 : EXIT FUNCTION
120 Fact% = N% * Fact%(N% - 1)
130 END FUNCTION
```

Output:

```text
 120
```
