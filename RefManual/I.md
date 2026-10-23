# I

<a id="if-statement"></a>
## IF Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOTO`, `GOSUB`, `ELSE`

### Purpose

Executes one set of statements when a condition is true and an optional alternative when it is false.

### Syntax

```basic
IF expression THEN statement[: statement ...] [ELSE statement[: statement ...]]
IF expression THEN line
IF expression GOTO line
IF expression GOSUB line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | Yes | Numeric condition. Zero is false; any nonzero value is true. |
| `THEN` clause | Yes | A line number or one or more statements separated by colons. |
| `ELSE` clause | No | Executed when the condition is false. |
| `line` | Depends | Branch target for `THEN line`, `GOTO`, or `GOSUB`. |

### Comments

The entire `IF ... THEN ... ELSE` statement must be on one line; `ELSE` cannot be placed on a separate line.

`THEN` followed by a line number is an implicit `GOTO`. `IF expression GOSUB line` is a newbasic extension; the subroutine returns with `RETURN`.

Nested `IF` statements are supported; an `ELSE` belongs to the nearest unmatched `IF`.

Newbasic does not accept a comma between the condition and `THEN`: write `IF A>5 THEN ...`, not `IF A>5, THEN ...`.

Line-number branches are illegal in direct mode.

### Examples

#### Example 1 — THEN and ELSE

```basic
10 A=7
20 IF A>5 THEN PRINT "BIG" ELSE PRINT "SMALL"
30 IF A=7 THEN 100
40 END
100 PRINT "SEVEN"
110 END
```

Output:

```text
BIG
SEVEN
```

#### Example 2 — Nested ELSE IF

```basic
10 A=7
20 IF A>10 THEN PRINT "BIG" ELSE IF A>5 THEN PRINT "MEDIUM" ELSE PRINT "SMALL"
30 END
```

Output:

```text
MEDIUM
```

<a id="inkey-function"></a>
## INKEY$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `INPUT$`, `KEY`, `KEY(n)`

### Purpose

Returns a character if one is pending, or an empty string if not.

### Syntax

```basic
INKEY$
```

### Arguments

None.

### Comments

`INKEY$` does not wait. It returns at most one character per call. Newbasic maps character keys and the arrow-key extended codes; function-key macros are not delivered by the current input backends. A null result means no key is waiting.

### Examples

#### Example 1 — Poll for a key

```basic
10 PRINT "Press a key"
20 K$=INKEY$
30 IF K$="" THEN 20
40 PRINT "YOU PRESSED "; K$
50 END
```

<a id="inp-function"></a>
## INP Function

**Type:** Function  
**Status:** Not supported in newbasic  
**See also:** `INKEY$`, `KEY`

### Purpose

Reads a byte from a hardware I/O port.

### Syntax

```basic
INP(port)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. Newbasic has no x86 I/O-port model.

### Portability note

GW-BASIC programs commonly used `INP(1)` or `INP(71)` to test whether a key was waiting. Those port addresses do not exist in newbasic, so the technique cannot be ported.

Use `INKEY$` instead. It returns one pending character, or an empty string when no key is waiting, without blocking:

```basic
10 K$=INKEY$
20 IF K$="" THEN 10
30 PRINT "KEY: "; K$
40 END
```

`INKEY$` is the portable replacement for keyboard-status polling. It does not provide the old function-key macros; see the `KEY` entry.

<a id="input-statement"></a>
## INPUT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `LINE INPUT`, `INPUT#`, `INPUT$`

### Purpose

Reads values from the keyboard into variables.

### Syntax

```basic
INPUT [;] ["prompt" ;|,] variable [, variable ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `prompt` | No | Text displayed before input. A semicolon keeps the cursor on the same line; a comma adds a question mark. |
| `variable` | Yes | One or more scalar variables. Arrays are not accepted. |

### Comments

`INPUT` waits for a complete line and parses comma-separated values. String input may be quoted or unquoted. A leading semicolon suppresses the automatic question mark. Invalid input raises **Redo from start** and re-prompts. `LINE INPUT` reads a whole line without comma parsing.

### Examples

#### Example 1 — Prompt for a name and age

```basic
10 INPUT "NAME, AGE"; N$, A
20 PRINT N$; " IS"; A
30 END
```

<a id="input-statement-2"></a>
## INPUT# Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `INPUT`, `LINE INPUT#`, `OPEN`

### Purpose

Reads values from a sequential file.

### Syntax

```basic
INPUT #file number, variable [, variable ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Number of a file opened for input. |
| `variable` | Yes | Scalar variables to receive the values. |

### Comments

The file must be open for input. Values are comma-separated as written by `WRITE#`; strings may be quoted. Array elements are not accepted as `INPUT#` targets. End of file raises **Input past end** unless checked with `EOF`.

### Examples

#### Example 1 — Read a record

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 WRITE #1, "ALICE", 30
30 CLOSE #1
40 OPEN "TEST.DAT" FOR INPUT AS #1
50 INPUT #1, N$, A
60 CLOSE #1
70 PRINT N$; A
80 END
```

Output:

```text
ALICE 30
```

<a id="input-function"></a>
## INPUT$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `INKEY$`, `LINE INPUT#`

### Purpose

Returns a specified number of characters from the keyboard or a file.

### Syntax

```basic
INPUT$(count [, #file number])
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of characters to read. |
| `file number` | No | Open file to read from. Without it, input comes from the keyboard. |

### Comments

Unlike `INPUT`, `INPUT$` does not wait for Enter and does not echo keyboard characters. It reads exactly `count` characters unless end of file is reached. The file form requires an open input file.

### Examples

#### Example 1 — Read four characters from a file

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "ABCDE"
30 CLOSE #1
40 OPEN "TEST.DAT" FOR INPUT AS #1
50 PRINT INPUT$(4, #1)
60 CLOSE #1
70 END
```

Output:

```text
ABCD
```

<a id="instr-function"></a>
## INSTR Function

**Type:** Function  
**Status:** Supported  
**See also:** `MID$`, `LEFT$`, `RIGHT$`

### Purpose

Finds the position of one string inside another.

### Syntax

```basic
INSTR([start,] haystack$, needle$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `start` | No | 1-based position at which to begin searching. Default is `1`. |
| `haystack$` | Yes | String to search. |
| `needle$` | Yes | String to find. |

### Comments

Returns the 1-based position of the first match, or `0` if not found. If `needle$` is empty, the result is `start`. A `start` outside the haystack returns `0`.

### Examples

#### Example 1 — Find a substring

```basic
10 A$="NEWBASIC REFERENCE"
20 PRINT INSTR(A$, "BASIC")
30 PRINT INSTR(4, A$, "BASIC")
40 PRINT INSTR(A$, "ZZZ")
50 END
```

Output:

```text
 4
 4
 0
```

<a id="int-function"></a>
## INT Function

**Type:** Function  
**Status:** Supported  
**See also:** `FIX`, `CINT`

### Purpose

Returns the largest integer less than or equal to a number.

### Syntax

```basic
INT(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

`INT` rounds toward negative infinity. For positive values it behaves like truncation; for negative values it goes to the next lower integer. Use `FIX` to truncate toward zero.

### Examples

#### Example 1 — Floor values

```basic
10 PRINT INT(2.9)
20 PRINT INT(-2.1)
30 END
```

Output:

```text
 2
-3
```

<a id="ioctl-statement"></a>
## IOCTL Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Sends a control string to a device driver.

### Syntax

```basic
IOCTL [#file number], string$
```

### Newbasic status

Not implemented. Newbasic has no DOS device-driver interface.

<a id="ioctl-function"></a>
## IOCTL$ Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Reads a control response from a device driver.

### Syntax

```basic
IOCTL$([#file number])
```

### Newbasic status

Not implemented.
