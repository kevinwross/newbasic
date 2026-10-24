# G

<a id="get-statement"></a>
## GET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PUT`, `FIELD`, `OPEN`, `SCREEN`

### Purpose

Reads a random-access file record, or captures a graphics rectangle into an array.

### Syntax

```basic
GET #file number [, record number]
GET (x1, y1)-(x2, y2), array
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes (file form) | An open random-access file. |
| `record number` | No | Record to read. Default is the next record. |
| `x1`, `y1`, `x2`, `y2` | Yes (graphics form) | Rectangle to capture, inclusive of both corners. |
| `array` | Yes (graphics form) | Integer array receiving the image. |

### Comments

The file form fills the `FIELD` buffers; use `CVI`, `CVS`, or `CVD` to decode numeric fields. A record that has never been written reads as zero-filled data.

The graphics form stores width, height, then one palette index per pixel in row-major order. This is a documented divergence from GW-BASIC’s packed image format.

### Examples

#### Example 1 — Read a random record

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 GET #1, 1
40 PRINT A$
50 CLOSE #1
60 END
```

#### Example 2 — Capture and restore a graphics rectangle

```basic
10 SCREEN 1
20 CIRCLE (160,100),30,2
30 DIM G(6000)
40 GET (120,70)-(200,130), G
50 CLS
60 PUT (20,20), G
70 END
```

<a id="gosub-statement"></a>
## GOSUB Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `RETURN`, `ON ... GOSUB`

### Purpose

Calls a subroutine at a specified line.

### Syntax

```basic
GOSUB line
GOSUB label
GOSUB expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | Yes | Existing line number where the subroutine begins. |
| `label` | Yes | Name declared by a `LABEL` statement. |
| `expression` | Yes | newbasic extension: a numeric expression that evaluates to a line number at run time. |

### Comments

`GOSUB` pushes a return address; the subroutine must finish with `RETURN`. A `GOSUB` without a matching `RETURN` eventually raises **Return without GOSUB** when the stack is misused. Direct-mode `GOSUB` is not allowed.

A literal target is resolved when the program is compiled. A bare identifier is a label; a type-suffixed identifier is a computed line-number expression (`GOSUB T%`), and an unsuffixed computed variable must be parenthesized (`GOSUB (T)`). A computed expression is evaluated when the statement runs, rounded to an integer, and looked up in the program's line table; a missing line raises **Undefined line number** and a non-numeric target raises **Type mismatch**. This makes computed dispatch possible without `ON ... GOSUB`:

```basic
100 T% = 200
110 GOSUB T%
```

A literal target followed by an operator is an expression (`GOSUB 100 + 100`); a bare literal followed by `:`, `ELSE`, a comment, or the end of the line keeps the old compile-time resolution.

### Examples

#### Example 1 — Call and return

```basic
10 GOSUB 100
20 PRINT "BACK"
30 END
100 PRINT "SUB"
110 RETURN
```

Output:

```text
SUB
BACK
```

#### Example 2 — Computed target

```basic
10 T% = 200
20 GOSUB T%
30 END
200 PRINT "DISPATCHED"
210 RETURN
```

Output:

```text
DISPATCHED
```

<a id="goto-statement"></a>
## GOTO Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOSUB`, `IF`, `ON ... GOTO`

### Purpose

Branches unconditionally to a line number.

### Syntax

```basic
GOTO line
GOTO label
GOTO expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | Yes | Existing line number. |
| `label` | Yes | Name declared by a `LABEL` statement. |
| `expression` | Yes | newbasic extension: a numeric expression that evaluates to a line number at run time. |

### Comments

`GOTO` is not allowed in direct mode. A literal target that does not exist raises **Undefined line number** at compile time or load time. A bare identifier is a label; a type-suffixed identifier is a computed line-number expression (`GOTO T%`), and an unsuffixed computed variable must be parenthesized (`GOTO (T)`). A computed target is evaluated when the statement runs, rounded to an integer, and looked up in the program's line table; a missing line raises **Undefined line number** and a non-numeric target raises **Type mismatch**. Prefer structured loops where possible, but `GOTO` remains useful for menus and error recovery.

A literal target followed by an operator is an expression (`GOTO 100 + 100`); a bare literal followed by `:`, `ELSE`, a comment, or the end of the line keeps the ordinary compile-time resolution.

`GOTO` out of an active `SELECT CASE` raises **Illegal function call** at run time with the message `GOTO would leave a SELECT CASE`. Jumping to a line inside the same `SELECT CASE` is allowed; jumping to its `END SELECT` line is also allowed and closes it normally. Use structured control flow or `GOSUB` when a CASE body needs to call code outside the block.

### Examples

#### Example 1 — Skip a line

```basic
10 GOTO 30
20 PRINT "SKIPPED"
30 PRINT "HERE"
40 END
```

Output:

```text
HERE
```

#### Example 2 — Computed target

```basic
10 T% = 200
20 GOTO T%
30 PRINT "SKIPPED"
40 END
200 PRINT "TARGET"
210 END
```

Output:

```text
TARGET
```
