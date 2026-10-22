# N

<a id="name-command"></a>
## NAME Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Renames a file.

### Syntax

```basic
NAME oldfile AS newfile
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `oldfile` | Yes | Existing file name. |
| `newfile` | Yes | New file name. Must not already exist. |

### Comments

Implemented as a direct-mode command, not a program statement. It does not overwrite an existing destination; that raises **Path/File access error**. It can move a file to another directory on the same volume.

### Examples

```text
NAME "DATA.DAT" AS "BACKUP.DAT"
```

<a id="new-command"></a>
## NEW Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `CLEAR`, `LOAD`

### Purpose

Clears the current program and its variables.

### Syntax

```basic
NEW
```

### Comments

`NEW` is a direct-mode command. It removes the stored program and closes the current program context. Saved files are unaffected. Use `CLEAR` to keep the program and clear runtime state.

### Examples

```text
NEW
```

<a id="next-statement"></a>
## NEXT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `FOR`

### Purpose

Closes a `FOR` loop and advances the loop variable.

### Syntax

```basic
NEXT [variable [, variable ...]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable` | No | Loop variable to close. With no name, closes the innermost loop. |

### Comments

The loop limit is checked before the first iteration. A negative `STEP` counts downward. Nested loops should use matching variable names for clarity: `NEXT I` rather than a bare `NEXT`.

### Examples

#### Example 1 — Nested loops

```basic
10 FOR I=1 TO 2
20 FOR J=1 TO 3
30 PRINT I; J
40 NEXT J
50 NEXT I
60 END
```

Output:

```text
 1  1
 1  2
 1  3
 2  1
 2  2
 2  3
```
