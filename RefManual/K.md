# K

<a id="key-statement"></a>
## KEY Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `KEY(n)`, `INKEY$`

### Purpose

Assigns a display string to a function-key slot and controls the function-key bar.

### Syntax

```basic
KEY n, string
KEY ON
KEY OFF
KEY LIST
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `n` | Yes (assignment form) | Key number `1`–`10` or `15`–`20`. |
| `string` | Yes (assignment form) | Label text; truncated to 15 characters. An empty string clears the label. |

### Comments

The assignment is stored and displayed by `KEY ON` / `KEY LIST`, but the current input backends do **not** turn F1–F10 into macro input. Pressing a function key does not type the assigned string, and `INKEY$` does not return it. `KEY ON` puts the first ten labels on the last screen line; `KEY OFF` restores that line to the text window.

### Examples

#### Example 1 — Label the function keys

```basic
10 KEY 1, "HELP"
20 KEY 2, "EXIT"
30 KEY ON
40 KEY LIST
50 END
```

<a id="keyn-statement"></a>
## KEY(n) Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `ON KEY(n)`, `KEY`, `INKEY$`

### Purpose

Enables, disables, or stops trapping for a key.

### Syntax

```basic
KEY(n) ON
KEY(n) OFF
KEY(n) STOP
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `n` | Yes | Key number `1`–`10`. |

### Comments

`KEY(n)` works with `ON KEY(n) GOSUB`. Newbasic’s input model delivers characters rather than function-key scan codes, so any pending key fires the lowest armed trap; the handler can read the character with `INKEY$`. It is not a true per-function-key trap.

### Examples

#### Example 1 — Trap the next key

```basic
10 ON KEY(1) GOSUB 100
20 KEY(1) ON
30 PRINT "WAITING"
40 FOR I=1 TO 100: NEXT I
50 END
100 K$=INKEY$: PRINT "KEY="; K$: RETURN
```

<a id="kill-command"></a>
## KILL Command

**Type:** Command  
**Status:** Direct mode only

### Purpose

Deletes a file.

### Syntax

```basic
KILL filename
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | File to delete. Wildcards are not supported. |

### Comments

`KILL` is a direct-mode command and is deliberately refused inside a program. It cannot delete a file that is currently open. Use `NAME` to rename files or `FILES` to inspect the directory.

### Examples

```text
KILL "OLD.DAT"
```
