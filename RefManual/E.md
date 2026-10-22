# E

<a id="edit-command"></a>
## EDIT Command

**Type:** Command  
**Status:** Not supported in newbasic

### Purpose

Opens a stored program line in GW-BASIC’s full-screen editor.

### Syntax

```basic
EDIT line
EDIT .
```

### Newbasic status

Not implemented. `EDIT` inside a program reports **Illegal direct**, and the direct-mode screen editor has not been written. Newbasic edits a line by retyping its number; use `LIST`, `DELETE`, `RENUM`, and `MERGE` for other editing operations.

<a id="end-statement"></a>
## END Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `STOP`, `CONT`

### Purpose

Ends program execution without an error.

### Syntax

```basic
END
```

### Arguments

None.

### Comments

`END` returns to direct mode. Variables and arrays remain available for inspection. Unlike `STOP`, it does not save a continue point for `CONT`.

### Examples

#### Example 1 — Stop after output

```basic
10 PRINT "DONE"
20 END
30 PRINT "NEVER"
```

Output:

```text
DONE
```

<a id="environ-statement"></a>
## ENVIRON Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Changes an environment variable.

### Syntax

```basic
ENVIRON string$
```

### Newbasic status

Not implemented. Newbasic does not expose a process-environment API.

<a id="environ-function"></a>
## ENVIRON$ Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Reads an environment variable.

### Syntax

```basic
ENVIRON$(name$)
ENVIRON$(number)
```

### Newbasic status

Not implemented; there is no environment-variable model.

<a id="eof-function"></a>
## EOF Function

**Type:** Function  
**Status:** Supported  
**See also:** `LOC`, `LOF`, `INPUT#`

### Purpose

Tests whether the end of a sequential file has been reached.

### Syntax

```basic
EOF(file number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Number of an open file. |

### Comments

Returns `-1` (true) when the file pointer is at end of file and `0` otherwise. The file must be open; an unopened number raises **Bad file number**. Reading past the end of a sequential file raises **Input past end**.

### Examples

#### Example 1 — Read until EOF

```basic
10 OPEN "TEST.DAT" FOR INPUT AS #1
20 WHILE NOT EOF(1)
30 LINE INPUT #1, A$
40 PRINT A$
50 WEND
60 CLOSE #1
70 END
```

<a id="erase-statement"></a>
## ERASE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `DIM`, `CLEAR`

### Purpose

Deletes one or more arrays.

### Syntax

```basic
ERASE array [, array ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `array` | Yes | Name of a previously dimensioned array. |

### Comments

`ERASE` releases the array; the array must be `DIM`ed again before use. Erasing a non-existent array is harmless.

### Examples

#### Example 1 — Erase and redeclare

```basic
10 DIM A(3)
20 A(1)=7
30 ERASE A
40 DIM A(5)
50 PRINT A(1)
60 END
```

Output:

```text
 0
```

<a id="erdev-variable"></a>
## ERDEV Variable

**Type:** Variable  
**Status:** Not supported in newbasic

### Purpose

Holds the device-driver error code from the last device error.

### Syntax

```basic
ERDEV
```

### Newbasic status

Not implemented; newbasic has no DOS device-driver layer.

<a id="erdev-variable-2"></a>
## ERDEV$ Variable

**Type:** Variable  
**Status:** Not supported in newbasic

### Purpose

Holds the name of the device that produced the last device error.

### Syntax

```basic
ERDEV$
```

### Newbasic status

Not implemented.

<a id="erl-variable"></a>
## ERL Variable

**Type:** Variable  
**Status:** Supported  
**See also:** `ERR`, `ON ERROR`, `RESUME`

### Purpose

Returns the line number in which the last trapped error occurred.

### Syntax

```basic
ERL
```

### Comments

`ERL` is normally read inside an error handler. It is updated when an error is trapped and remains available until the next error. A direct-mode error has line `0`.

### Examples

#### Example 1 — Report the error line

```basic
10 ON ERROR GOTO 100
20 A=1/0
30 END
100 PRINT "ERROR AT"; ERL
110 RESUME NEXT
```

<a id="err-variable"></a>
## ERR Variable

**Type:** Variable  
**Status:** Supported  
**See also:** `ERL`, `ON ERROR`, `ERROR`

### Purpose

Returns the numeric code of the last trapped error.

### Syntax

```basic
ERR
```

### Comments

Use `ERR` in an error handler to distinguish failures. `ERROR n` raises a specific code deliberately, which is useful for testing handlers.

### Examples

#### Example 1 — Catch and identify an error

```basic
10 ON ERROR GOTO 100
20 ERROR 7
30 END
100 PRINT ERR
110 RESUME NEXT
```

Output:

```text
 7
```

<a id="error-statement"></a>
## ERROR Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `ERR`, `ERL`, `ON ERROR`

### Purpose

Raises a specified BASIC error code.

### Syntax

```basic
ERROR code
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `code` | Yes | Error number, from the GW-BASIC error table. |

### Comments

If error trapping is enabled, the handler runs with `ERR` set to the code. Otherwise the program stops with that error. `ERROR 0` is accepted as a no-op by some dialects; avoid it.

### Examples

#### Example 1 — Raise and trap a custom error

```basic
10 ON ERROR GOTO 100
20 ERROR 5
30 END
100 PRINT "CAUGHT"; ERR
110 RESUME NEXT
```

Output:

```text
CAUGHT 5
```

<a id="exp-function"></a>
## EXP Function

**Type:** Function  
**Status:** Supported  
**See also:** `LOG`, `SQR`

### Purpose

Returns e raised to a power.

### Syntax

```basic
EXP(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric exponent. |

### Comments

`EXP(0)` is `1`. Very large results may raise **Overflow**.

### Examples

#### Example 1 — Exponential growth

```basic
10 PRINT EXP(0)
20 PRINT EXP(1)
30 END
```

Output:

```text
 1
 2.718282
```

<a id="exterr-function"></a>
## EXTERR Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns extended error information from DOS.

### Syntax

```basic
EXTERR(code)
```

### Newbasic status

Not implemented. Newbasic reports GW-BASIC-style error codes and messages; it has no DOS extended-error table.
