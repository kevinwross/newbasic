# D

<a id="data-statement"></a>
## DATA Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `READ`, `RESTORE`

### Purpose

Stores constant values for later `READ` statements.

### Syntax

```basic
DATA item [, item ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `item` | Yes | Numeric or string constant. Quote strings containing commas, colons, or leading/trailing spaces. |

### Comments

`DATA` lines are skipped during execution. Values are read in program order, regardless of where the `DATA` line sits. `RESTORE` rewinds the read pointer. An empty item is a zero-length string or zero, depending on the target.

### Examples

#### Example 1 — Read three records

```basic
10 DATA "ALICE", 30, "BOB", 42
20 READ N$, A
30 PRINT N$; A
40 READ N$, A
50 PRINT N$; A
60 END
```

Output:

```text
ALICE 30
BOB 42
```

<a id="date-function"></a>
## DATE$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `TIME$`, `TIMER`

### Purpose

Returns the current date as a string.

### Syntax

```basic
DATE$
```

### Arguments

None.

### Comments

The result is a ten-character date in `MM-DD-YYYY` form. `DATE$` is a function in newbasic; assigning to it is not supported. Use `TIME$` for the clock time.

### Examples

#### Example 1 — Print the date

```basic
10 PRINT DATE$
20 END
```

<a id="def-fn-statement"></a>
## DEF FN Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `FN` calls, `DEFINT`, `DEFSNG`, `DEFDBL`, `DEFSTR`

### Purpose

Defines a user-written function whose body is a single expression.

### Syntax

```basic
DEF FNname[(parameter [, parameter ...])] = expression
DEF FN name[(parameter [, parameter ...])] = expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | Function name beginning with `FN`; may carry a type suffix such as `$`. |
| `parameter` | No | Scalar parameter names. Parameters are local to the call. |
| `expression` | Yes | The function result. May use parameters, globals, built-ins, and other user functions. |

### Comments

The `DEF` line is skipped at run time. The definition must appear textually before the first call; an undefined function reports **Undefined user function** at compile time.

Parameters are passed by value and restored after the call. A zero-argument function may be called without parentheses. The body is one expression: there is no `END DEF`, statement block, or `RETURN`.

String functions use a `$` suffix. Redefining a function makes the latest definition win. `DEF FN` is illegal in direct mode.

### Examples

#### Example 1 — Numeric function and composition

```basic
10 DEF FNSQR(X)=X*X
20 DEF FNCUBE(X)=X*FNSQR(X)
30 PRINT FNSQR(7)
40 PRINT FNCUBE(3)
50 END
```

Output:

```text
 49
 27
```

#### Example 2 — String function

```basic
10 DEF FNTAG$(S$)="["+S$+"]"
20 PRINT FNTAG$("OK")
30 END
```

Output:

```text
[OK]
```

#### Example 3 — Spaced definition and call

```basic
10 DEF FN Smoo(X)=X+X*X
20 PRINT FN Smoo(13)
30 END
```

Output:

```text
 182
```

<a id="def-seg-statement"></a>
## DEF SEG Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Sets the current machine-memory segment for `PEEK`, `POKE`, `BLOAD`, `BSAVE`, `CALL`, and `USR`.

### Syntax

```basic
DEF SEG [= address]
```

### Newbasic status

Not implemented. Newbasic has no 8086 segment model or addressable machine memory.

<a id="def-usr-statement"></a>
## DEF USR Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Defines the entry address for a machine-language routine called by `USR`.

### Syntax

```basic
DEF USR[n] = address
```

### Newbasic status

Not implemented. There is no machine-code or native-call interface.

<a id="defdbl-statement"></a>
## DEFDBL Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DEFINT`, `DEFSNG`, `DEFSTR`

### Purpose

Declares that selected variable names default to double precision.

### Syntax

```basic
DEFDBL letter [- letter] [, letter [- letter] ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `letter` | Yes | A letter or a range. |

### Comments

Newbasic stores numeric values in host floating point, so this is primarily a declaration for compatibility and type checking.

### Examples

#### Example 1 — Double-precision default

```basic
10 DEFDBL D
20 D=1.25
30 PRINT D
40 END
```

<a id="defint-statement"></a>
## DEFINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DEFSNG`, `DEFDBL`, `DEFSTR`, `DEF FN`

### Purpose

Declares that variables beginning with selected letters default to integer type.

### Syntax

```basic
DEFINT letter [- letter] [, letter [- letter] ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `letter` | Yes | A letter or a range such as `A-C`. |

### Comments

The declaration applies to variable names that do not already have an explicit type suffix. It is compiled into the program; restate it in a chained program.

### Examples

#### Example 1 — Integer by default

```basic
10 DEFINT A-C
20 A=12
30 PRINT A
40 END
```

Output:

```text
 12
```

<a id="defsng-statement"></a>
## DEFSNG Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DEFINT`, `DEFDBL`, `DEFSTR`

### Purpose

Declares that selected variable names default to single precision.

### Syntax

```basic
DEFSNG letter [- letter] [, letter [- letter] ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `letter` | Yes | A letter or a range. |

### Comments

As with the other `DEF` type declarations, explicit suffixes on a variable name take precedence.

### Examples

#### Example 1 — Single-precision default

```basic
10 DEFSNG S
20 S=1.5
30 PRINT S
40 END
```

<a id="defstr-statement"></a>
## DEFSTR Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DEFINT`, `DEFSNG`, `DEFDBL`

### Purpose

Declares that selected variable names default to string type.

### Syntax

```basic
DEFSTR letter [- letter] [, letter [- letter] ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `letter` | Yes | A letter or a range. |

### Comments

A variable with an explicit `%`, `&`, `!`, `#`, or `$` suffix keeps that type.

### Examples

#### Example 1 — String by default

```basic
10 DEFSTR A
20 A="TEXT"
30 PRINT A
40 END
```

<a id="delete-command"></a>
## DELETE Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `LIST`, `RENUM`

### Purpose

Deletes a range of stored program lines.

### Syntax

```basic
DELETE [first] [- [last]]
DELETE first
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `first` | No | First line to delete. Default is the first stored line. |
| `last` | No | Last line to delete. Default is the last stored line. |

### Comments

`DELETE` edits the stored program; it is not available inside a running program. Use `DELETE 20` to remove one line, `DELETE 20-40` for a range, or bare `DELETE` to remove the whole program.

### Examples

```text
LIST
DELETE 20-30
LIST
```

<a id="dim-statement"></a>
## DIM Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `ERASE`, `OPTION BASE`, `DIM` arrays

### Purpose

Declares one or more arrays and reserves their elements.

### Syntax

```basic
DIM array(subscripts) [, array(subscripts) ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `array` | Yes | Array name, optionally with a type suffix. |
| `subscripts` | Yes | One or more upper bounds; lower bound is `0` unless `OPTION BASE 1` was used. |

### Comments

Arrays are row-major. A dimension is an upper bound, not a count: `DIM A(3)` creates A(0) through A(3). Re-declaring an array resets it. `REDIM` is not a GW-BASIC keyword; use `ERASE` followed by `DIM`.

### Examples

#### Example 1 — One- and two-dimensional arrays

```basic
10 DIM A(3), B(2,2)
20 A(1)=7
30 B(2,1)=9
40 PRINT A(1); B(2,1)
50 END
```

Output:

```text
 7  9
```

<a id="draw-statement"></a>
## DRAW Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PSET`, `LINE`, `CIRCLE`, `SCREEN`

### Purpose

Draws lines using GW-BASIC’s DRAW macro language.

### Syntax

```basic
DRAW command$
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `command$` | Yes | String expression containing DRAW commands. |

### Comments

Commands include `U`, `D`, `L`, `R`, `E`, `F`, `G`, `H`, `M`, `B`, and `N`, with optional distances. The drawing starts at the current graphics point. `DRAW` requires a graphics mode. Unsupported macro letters raise **Illegal function call**.

### Examples

#### Example 1 — Draw a square

```basic
10 SCREEN 1
20 PSET (100,100), 1
30 DRAW "R40 D40 L40 U40"
40 END
```
