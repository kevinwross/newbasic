# R

<a id="randomize-statement"></a>
## RANDOMIZE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `RND`

### Purpose

Seeds the random-number generator.

### Syntax

```basic
RANDOMIZE [seed]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `seed` | No | Numeric seed. Without it, newbasic uses a fixed/default seed path. |

### Comments

Newbasic uses `std::mt19937`, so sequences differ from GW-BASIC’s generator but are reproducible for a given seed. `RANDOMIZE` without a seed does not prompt; it uses the implementation’s default seeding path.

### Examples

#### Example 1 — Reproducible random values

```basic
10 RANDOMIZE 1234
20 FOR I=1 TO 3
30 PRINT RND
40 NEXT I
50 END
```

<a id="read-statement"></a>
## READ Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DATA`, `RESTORE`

### Purpose

Reads values from `DATA` statements into variables.

### Syntax

```basic
READ variable [, variable ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable` | Yes | Scalar variable or array element to receive the next value. |

### Comments

Values are read in the order the `DATA` items appear in the program, not necessarily in line-number order. Running out of data raises **Out of DATA**. Type mismatches raise **Type mismatch**.

### Examples

#### Example 1 — Read names and ages

```basic
10 DATA "ALICE", 30, "BOB", 42
20 FOR I=1 TO 2
30 READ N$, A
40 PRINT N$; A
50 NEXT I
60 END
```

<a id="rem-statement"></a>
## REM Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `'` comment

### Purpose

Marks a comment.

### Syntax

```basic
REM comment text
```

### Comments

Everything after `REM` to the end of the line is ignored. A single quote `'` is the shorter comment form. `REM` can be used after a colon to comment out the rest of a line.

### Examples

#### Example 1 — Comment a line

```basic
10 REM This line does nothing
20 A=7: REM set A
30 PRINT A
40 END
```

<a id="renum-command"></a>
## RENUM Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `LIST`, `DELETE`, `MERGE`

### Purpose

Renumbers stored program lines and updates line references.

### Syntax

```basic
RENUM [new][, [old][, increment]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `new` | No | New first line number. Default is `10`. |
| `old` | No | Old first line to renumber. Default is the first line. |
| `increment` | No | Step between new numbers. Default is `10`. |

### Comments

`RENUM` rewrites references after `ELSE`, `GOTO`, `GOSUB`, `THEN`, `ON ... GOTO`, `ON ... GOSUB`, `RESTORE`, `RESUME`, and `RUN`, while skipping strings and comments. It refuses to reorder lines and reports undefined references. It is a direct-mode command.

### Examples

```text
RENUM
RENUM 100,20,10
```

<a id="reset-command"></a>
## RESET Command

**Type:** Command  
**Status:** Not supported in newbasic

### Purpose

Closes all disk files and writes directory information before removing a diskette.

### Syntax

```basic
RESET
```

### Newbasic status

Not implemented. Use `CLOSE` to close files. Newbasic has no removable-disk directory buffer to flush.

<a id="restore-statement"></a>
## RESTORE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DATA`, `READ`

### Purpose

Rewinds or positions the `DATA` read pointer.

### Syntax

```basic
RESTORE [line]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | No | Line containing the `DATA` item to read next. Without it, the pointer returns to the first `DATA` item. |

### Comments

`RESTORE` affects only subsequent `READ` statements. If the line has no `DATA`, the next `DATA` after that line is used.

### Examples

#### Example 1 — Read the same data twice

```basic
10 DATA 10,20
20 READ A,B
30 RESTORE
40 READ C,D
50 PRINT A;B;C;D
60 END
```

Output:

```text
 10  20  10  20
```

<a id="resume-statement"></a>
## RESUME Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `ON ERROR`, `ERR`, `ERL`

### Purpose

Returns control from an error handler.

### Syntax

```basic
RESUME
RESUME NEXT
RESUME line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | No | Line at which to resume. |

### Comments

Bare `RESUME` retries the statement that failed. `RESUME NEXT` continues with the statement after the one that failed. `RESUME line` branches to a specific line. `RESUME` outside an active handler raises **RESUME without error**.

### Examples

#### Example 1 — Skip the failing statement

```basic
10 ON ERROR GOTO 100
20 ERROR 7
30 PRINT "AFTER"
40 END
100 PRINT "HANDLED"
110 RESUME NEXT
```

Output:

```text
HANDLED
AFTER
```

<a id="return-statement"></a>
## RETURN Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOSUB`, `ON ... GOSUB`

### Purpose

Returns from a subroutine called by `GOSUB`.

### Syntax

```basic
RETURN [line]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | No | Optional line to continue at instead of the saved return address. |

### Comments

`RETURN` without a saved `GOSUB` raises **RETURN without GOSUB**. A line argument is a non-local return; use it sparingly.

### Examples

#### Example 1 — Return to the caller

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

<a id="right-function"></a>
## RIGHT$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `LEFT$`, `MID$`, `LEN`

### Purpose

Returns the rightmost characters of a string.

### Syntax

```basic
RIGHT$(string$, count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | Source string. |
| `count` | Yes | Number of characters. |

### Comments

If `count` exceeds the string length, the whole string is returned. `RIGHT$(s,0)` returns the empty string.

### Examples

#### Example 1 — Take a suffix

```basic
10 A$="NEWBASIC"
20 PRINT RIGHT$(A$, 5)
30 END
```

Output:

```text
BASIC
```

<a id="rmdir-command"></a>
## RMDIR Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Removes an empty directory.

### Syntax

```basic
RMDIR path
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `path` | Yes | Directory to remove. It must be empty. |

### Comments

Implemented as a direct-mode command, not a program statement. Removing a non-empty directory raises **Path/File access error**.

### Examples

```text
CHDIR ".."
RMDIR "DATA"
```

<a id="rnd-function"></a>
## RND Function

**Type:** Function  
**Status:** Supported  
**See also:** `RANDOMIZE`

### Purpose

Returns a pseudo-random single-precision number.

### Syntax

```basic
RND[(number)]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | No | Controls the sequence. Negative reseeds; zero repeats the last value; positive or omitted advances the sequence. |

### Comments

The result is in the range `0` to just below `1`. Newbasic uses `std::mt19937`; sequences differ from GW-BASIC but are reproducible after `RANDOMIZE seed`.

### Examples

#### Example 1 — Three random values

```basic
10 RANDOMIZE 42
20 FOR I=1 TO 3
30 PRINT RND
40 NEXT I
50 END
```

<a id="rset-statement"></a>
## RSET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `LSET`, `FIELD`

### Purpose

Right-justifies a string within a fixed-width field buffer.

### Syntax

```basic
RSET field variable = string expression
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `field variable` | Yes | String variable bound by `FIELD`, or a fixed-length string. |
| `string expression` | Yes | Value to place in the field. |

### Comments

Short values are padded on the left; long values are truncated on the left. `LSET` pads and truncates on the right.

### Examples

#### Example 1 — Right-justify a field

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 RSET A$="RIGHT"
40 PRINT "["; A$; "]"
50 CLOSE #1
60 END
```

<a id="run-command"></a>
## RUN Command

**Type:** Command  
**Status:** Supported  
**See also:** `LOAD`, `CHAIN`, `CONT`

### Purpose

Runs the current program or loads and runs a file.

### Syntax

```basic
RUN [line]
RUN filename [, R]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | No | Start line in the current program. |
| `filename` | No | Program file to load before running. |
| `R` | No | Retained for the `RUN filename,R` form; run after loading. |

### Comments

`RUN` clears variables and arrays before starting. `RUN line` starts at that line. `RUN filename` loads the file first. In a program, bare `RUN` restarts the program; use it carefully.

### Examples

```text
RUN
RUN 100
RUN "HELLO.BAS"
```
