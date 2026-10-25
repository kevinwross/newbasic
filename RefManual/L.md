# L

<a id="label-statement"></a>
## LABEL Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `GOTO`, `GOSUB`, `RENUM`

### Purpose

Gives a line a symbolic name that `GOTO`, `GOSUB`, `IF ... THEN`, and
`ON ... GOTO`/`ON ... GOSUB` can use instead of a number.

### Syntax

```basic
LABEL name
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | A label name. Case-insensitive; no type suffix. |

### Comments

Put `LABEL` as the first statement on its line. The label marks that line; the
statement itself does nothing. Code may follow on the same line after a colon:

```basic
100 LABEL PaintButton: PRINT "paint": RETURN
```

A bare identifier in a line-target position is a label. A type-suffixed
identifier remains a computed line number (`GOSUB T%`), and an unsuffixed
computed variable must be parenthesized (`GOTO (T)`).

Labels are checked when the program is compiled. Duplicate labels and references
to undefined labels are errors. `RENUM` moves the `LABEL` line and leaves the
symbolic references alone, so the next compile resolves them to the new line
numbers.

### Examples

#### Example 1 — Forward GOSUB

```basic
10 GOSUB Work
20 PRINT "BACK"
30 END
100 LABEL Work
110 PRINT "SUB"
120 RETURN
```

Output:

```text
SUB
BACK
```

#### Example 2 — RENUM keeps the name

```basic
10 GOSUB Work
20 END
100 LABEL Work
110 PRINT "STILL WORKS"
120 RETURN
```

After `RENUM 1000,,100`, the same program still runs: the call remains
`GOSUB Work` and the definition moves to line 1000.

<a id="left-function"></a>
## LEFT$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `RIGHT$`, `MID$`, `LEN`

### Purpose

Returns the leftmost characters of a string.

### Syntax

```basic
LEFT$(string$, count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Source string. |
| `count` | Yes | Number of characters. `0` returns the empty string. |

### Comments

If `count` exceeds the string length, the whole string is returned. A negative count raises **Illegal function call**.

### Examples

#### Example 1 — Take a prefix

```basic
10 A$="NEWBASIC"
20 PRINT LEFT$(A$, 3)
30 PRINT LEFT$(A$, 99)
40 END
```

Output:

```text
NEW
NEWBASIC
```

<a id="len-function"></a>
## LEN Function

**Type:** Function  
**Status:** Supported  
**See also:** `LEFT$`, `MID$`, `INSTR`

### Purpose

Returns the length of a string in bytes.

### Syntax

```basic
LEN(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | String expression. |

### Comments

The result is the number of stored bytes, not a count of display columns. CP437 bytes are counted one each.

### Examples

#### Example 1 — Measure strings

```basic
10 PRINT LEN("")
20 PRINT LEN("HELLO")
30 END
```

Output:

```text
 0
 5
```

<a id="let-statement"></a>
## LET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DIM`, `SWAP`

### Purpose

Assigns a value to a variable or array element.

### Syntax

```basic
[LET] variable = expression
[LET] array(subscripts) = expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable` | Yes | Scalar or array element target. |
| `expression` | Yes | Value to assign. |

### Comments

`LET` is optional; `A=1` is the usual form. The target type is determined by its suffix or `DEF` declaration. Assigning a string to a numeric variable raises **Type mismatch**.

### Examples

#### Example 1 — Scalar and array assignment

```basic
10 DIM A(2)
20 LET X=5
30 A(1)=X*2
40 PRINT X; A(1)
50 END
```

Output:

```text
 5  10
```

<a id="line-input-statement"></a>
## LINE INPUT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `INPUT`, `LINE INPUT#`, `INPUT$`

### Purpose

Reads an entire line from the keyboard into one string variable.

### Syntax

```basic
LINE INPUT [;] ["prompt" ;] string variable
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `prompt` | No | Text displayed before reading. |
| `string variable` | Yes | Single string variable to receive the line. |

### Comments

Commas and quotes are ordinary characters; the whole line is stored. `LINE INPUT` does not print a question mark. Only one string variable is allowed.

### Examples

#### Example 1 — Read a sentence

```basic
10 LINE INPUT "TEXT: "; A$
20 PRINT "["; A$; "]"
30 END
```

<a id="line-input-statement-2"></a>
## LINE INPUT# Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `INPUT#`, `LINE INPUT`, `EOF`

### Purpose

Reads an entire line from a sequential file into a string variable.

### Syntax

```basic
LINE INPUT #file number, string variable
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `string variable` | Yes | String variable to receive the line. |

### Comments

The line is read without comma parsing. It is useful for text files and for fields containing commas. End of file raises **Input past end** unless checked with `EOF`.

### Examples

#### Example 1 — Read a text file line by line

```basic
10 OPEN "TEST.TXT" FOR OUTPUT AS #1
20 PRINT #1, "ONE,TWO"
30 CLOSE #1
40 OPEN "TEST.TXT" FOR INPUT AS #1
50 LINE INPUT #1, A$
60 CLOSE #1
70 PRINT A$
80 END
```

Output:

```text
ONE,TWO
```

<a id="line-statement"></a>
## LINE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PSET`, `CIRCLE`, `PAINT`, `SCREEN`

### Purpose

Draws a line or a rectangular box between two graphics points.

### Syntax

```basic
LINE [[STEP](x1,y1)]-[STEP](x2,y2) [, [color] [, B[F]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x1`, `y1` | No | Start point. If omitted, drawing starts at the last graphics point. |
| `x2`, `y2` | Yes | End point. |
| `color` | No | Drawing attribute. Default is the current foreground. |
| `B` | No | Draw a box instead of a line. |
| `BF` | No | Draw a filled box. |

### Comments

`STEP` makes a coordinate relative to the current graphics point. Omitting the first point uses the last point. A box uses the two points as opposite corners. `LINE` requires a graphics mode.

### Examples

#### Example 1 — Line and box

```basic
10 SCREEN 1
20 LINE (20,20)-(200,100), 2
30 LINE (20,120)-(200,180), 3, BF
40 END
```

<a id="list-command"></a>
## LIST Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `LLIST`, `DELETE`, `RENUM`

### Purpose

Lists stored program lines on the screen.

### Syntax

```basic
LIST [first] [- [last]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `first` | No | First line to list. |
| `last` | No | Last line to list. |

### Comments

`LIST` is a direct-mode command. Bare `LIST` lists the whole program. A range can be `LIST 20-40`, and a single number lists that line. It does not print to the line printer; use `LLIST` for that.

### Examples

```text
LIST
LIST 20-40
```

<a id="llist-command"></a>
## LLIST Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `LIST`, `LPRINT`

### Purpose

Sends stored program lines to the line printer.

### Syntax

```basic
LLIST [first] [- [last]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `first` | No | First line to print. |
| `last` | No | Last line to print. |

### Comments

`LLIST` is a direct-mode command. The current implementation ignores the range and prints the whole program; this is a known limitation. Output goes through `Host::WriteToPrinter`.

### Examples

```text
LLIST
```

<a id="load-command"></a>
## LOAD Command

**Type:** Command  
**Status:** Supported (text only)  
**See also:** `SAVE`, `RUN`, `MERGE`

### Purpose

Loads a plain-text BASIC program from disk.

### Syntax

```basic
LOAD filename [, R]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | Plain-text `.BAS` program file. |
| `R` | No | Run immediately after loading. |

### Comments

`LOAD` replaces the stored program. Only plain-text `.BAS` files are accepted.
Tokenized and protected files are rejected with **Bad file mode**. Use `MERGE`
to add lines without clearing the current program.

### Examples

```text
LOAD "HELLO.BAS"
LOAD "HELLO.BAS", R
```

<a id="loc-function"></a>
## LOC Function

**Type:** Function  
**Status:** Supported  
**See also:** `LOF`, `EOF`, `GET`

### Purpose

Returns the current position within an open file.

### Syntax

```basic
LOC(file number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |

### Comments

For sequential files the value is the current byte position; for random files it is the current record position. The file must be open.

### Examples

#### Example 1 — Report position

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "ABC"
30 PRINT LOC(1)
40 CLOSE #1
50 END
```

<a id="local-statement"></a>
## LOCAL Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `SUB`, `FUNCTION`, `CALL`, `EXIT`

### Purpose

Declares variables that are private to one `SUB` or `FUNCTION` activation.

### Syntax

```basic
LOCAL name [, name ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | Local variable name, with an optional type suffix. |

### Comments

`LOCAL` must appear inside a procedure and before the first executable
statement. Multiple `LOCAL` lines are allowed.

Each name is initialized on every call: numeric locals to `0` of their declared
kind, string locals to `""`. Parameters are also private to the call and share
the same name space as `LOCAL` names; duplicates are errors.

A local shadows a global of the same name for the duration of the call. A
callee does not see its caller's locals; undeclared names inside a procedure
are globals.

Local arrays are not implemented. `DIM` inside a procedure still creates a
global array. `LOCAL` is illegal in direct mode.

### Examples

#### Example 1 — Local shadows a global

```basic
10 X% = 99
20 CALL Show
30 PRINT X%
40 END
100 SUB Show
110 LOCAL X%
120 X% = 42
130 PRINT X%
140 END SUB
```

Output:

```text
 42
 99
```

#### Example 2 — Locals are reinitialized per call

```basic
10 CALL Count
20 CALL Count
30 END
100 SUB Count
110 LOCAL N%
120 N% = N% + 1
130 PRINT N%
140 END SUB
```

Output:

```text
 1
 1
```

<a id="locate-statement"></a>
## LOCATE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `CSRLIN`, `POS`, `CLS`

### Purpose

Moves the text cursor and optionally sets its visibility.

### Syntax

```basic
LOCATE [row] [, [column] [, [cursor] [, [start] [, stop]]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `row` | No | 1-based row. |
| `column` | No | 1-based column. |
| `cursor` | No | `1` shows the cursor; `0` hides it. |
| `start`, `stop` | No | Cursor scan lines, accepted for compatibility. |

### Comments

Coordinates are clamped to the text grid. `LOCATE` does not print anything. Use `CSRLIN` and `POS` to read the resulting position.

### Examples

#### Example 1 — Place text

```basic
10 CLS
20 LOCATE 5, 10
30 PRINT "CENTER"
40 END
```

<a id="lock-statement"></a>
## LOCK Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Locks a range of records in an open file against access by other processes.

### Syntax

```basic
LOCK [#file number] [, [record] [TO record]]
```

### Newbasic status

Not implemented. Newbasic does not provide record locking or multi-user file arbitration.

<a id="lof-function"></a>
## LOF Function

**Type:** Function  
**Status:** Supported  
**See also:** `LOC`, `EOF`

### Purpose

Returns the length of an open file in bytes.

### Syntax

```basic
LOF(file number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |

### Comments

The value is the total file length, not the remaining length. The file must be open.

### Examples

#### Example 1 — File length

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "ABCDE"
30 PRINT LOF(1)
40 CLOSE #1
50 END
```

<a id="log-function"></a>
## LOG Function

**Type:** Function  
**Status:** Supported  
**See also:** `EXP`, `SQR`

### Purpose

Returns the natural logarithm of a positive number.

### Syntax

```basic
LOG(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Positive numeric expression. |

### Comments

`LOG(0)` or a negative argument raises **Illegal function call**. For base-10 logarithms, divide by `LOG(10)`.

### Examples

#### Example 1 — Natural and base-10 logarithms

```basic
10 PRINT LOG(1)
20 PRINT LOG(100) / LOG(10)
30 END
```

Output:

```text
 0
 2
```

<a id="lpos-function"></a>
## LPOS Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the current position of the line printer print head.

### Syntax

```basic
LPOS(printer number)
```

### Newbasic status

Not implemented. Newbasic has a printer sink but no print-head position model.

<a id="lprint-statement"></a>
## LPRINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT`, `LLIST`, `LPRINT USING`

### Purpose

Sends formatted output to the line printer.

### Syntax

```basic
LPRINT [expression] [;|, ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | No | Values to print. Semicolons concatenate; commas advance to print zones. |

### Comments

`LPRINT` uses the same formatting as `PRINT` but writes through the printer sink. In tests this is a recorder; on Windows it writes to the configured printer/file sink. `LPRINT` does not affect the screen cursor.

### Examples

#### Example 1 — Print two lines

```basic
10 LPRINT "REPORT"
20 LPRINT "VALUE"; 42
30 END
```

<a id="lprint-using-statement"></a>
## LPRINT USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT USING`, `LPRINT`

### Purpose

Prints formatted output to the line printer.

### Syntax

```basic
LPRINT USING format$; expression [, expression ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `format$` | Yes | Format string using the same rules as `PRINT USING`. |
| `expression` | Yes | Values to format and print. |

### Comments

The format fields follow `PRINT USING`: `#`, `+`, `-`, `.`, `,`, `$$`, `**`, `^^^^`, and string fields. `LPRINT USING` writes to the printer sink, not the screen.

### Examples

#### Example 1 — Formatted printer output

```basic
10 LPRINT USING "###.##"; 12.345
20 END
```

<a id="lset-statement"></a>
## LSET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `RSET`, `FIELD`

### Purpose

Left-justifies a string within a fixed-width field buffer.

### Syntax

```basic
LSET field variable = string expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `field variable` | Yes | String variable bound by `FIELD`, or a fixed-length string. |
| `string expression` | Yes | Value to place in the field. |

### Comments

If the value is shorter than the field, spaces pad the right. If it is longer, it is truncated on the right. `RSET` pads and truncates on the left.

### Examples

#### Example 1 — Left-justify a field

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 LSET A$="LEFT"
40 PRINT "["; A$; "]"
50 CLOSE #1
60 END
```
