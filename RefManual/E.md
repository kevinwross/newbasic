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

Newbasic deliberately refuses this statement so a BASIC program cannot modify the host’s system environment variables. Use the read-only `ENVIRON$` function instead.

<a id="environ-function"></a>
## ENVIRON$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `ENVIRON`, `SHELL`

### Purpose

Reads a host environment variable without modifying the environment.

### Syntax

```basic
ENVIRON$(name$)
ENVIRON$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name$` | Yes (name form) | Environment variable name. Matching is case-insensitive and names are normalized to uppercase. |
| `number` | Yes (number form) | 1-based position in the sorted environment snapshot, from 1 to 255. |

### Comments

The name form returns the variable’s value, or an empty string when it is not defined. Names are compared case-insensitively: `PATH`, `Path`, and `path` all find the same entry. The returned value is the text after the first `=`; an empty value is returned as an empty string.

The number form returns the selected entry as `NAME=value`, using the uppercase name. Entries are sorted by uppercase name, which makes the numbered form deterministic across hosts. A number outside `1`–`255` raises **Illegal function call**; `0` is not a valid index. A number beyond the available entries returns an empty string.

Because every numbered entry is returned as `NAME=value`, an empty result means there is no entry at that position, not merely that the value is empty. This makes the number form safe for walking the environment.

`ENVIRON$` is read-only. Newbasic deliberately refuses the `ENVIRON` statement so a program cannot change the environment inherited by `SHELL` or other child processes. Values may contain host-specific paths and may reveal sensitive configuration; print them only when intended.

### Examples

#### Example 1 — Read a variable case-insensitively

```basic
10 PRINT ENVIRON$("PATH")
20 PRINT ENVIRON$("Path")
30 PRINT ENVIRON$("NO_SUCH_VARIABLE")
40 END
```

#### Example 2 — Walk the environment until an empty entry

```basic
10 FOR I=1 TO 255
20 E$=ENVIRON$(I)
30 IF E$="" THEN 100
40 PRINT E$
50 NEXT I
60 PRINT "INDEX 255 REACHED"
70 END
100 PRINT "END OF ENVIRONMENT"
110 END
```

The loop stops at the first missing entry. If all 255 positions contain entries, it exits normally at line 60. `ENVIRON$(0)` is invalid and raises **Illegal function call**.

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

`ERL` is normally read inside an error handler. It is updated when an error is trapped and remains available until the next error. The value is a signed 32-bit line number; a direct-mode error has line `0`.

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
