# K

<a id="key-statement"></a>
## KEY Statement

**Type:** Statement  
**Status:** Not supported in newbasic  
**See also:** `INKEY$`

### Purpose

Assigns a display string to a function-key slot and controls the function-key bar.

### Syntax

```basic
KEY n, string
KEY ON
KEY OFF
KEY LIST
```

### Newbasic status

The entire `KEY` family is deliberately not supported. Any form reports **Advanced Feature**:

- `KEY n, string`
- `KEY ON`
- `KEY OFF`
- `KEY LIST`

Newbasic does not display a function-key bar, does not install default soft-key labels, and does not turn a function key into text input. Keyboard polling is available through `INKEY$`; see that entry for the supported alternative.

<a id="keyn-statement"></a>
## KEY(n) Statement

**Type:** Statement  
**Status:** Not supported in newbasic  
**See also:** `INKEY$`

### Purpose

Enables, disables, or stops trapping for a function key.

### Syntax

```basic
KEY(n) ON
KEY(n) OFF
KEY(n) STOP
```

### Newbasic status

Not supported. The function-key trap machinery has been removed from the VM. Any `KEY(n)` form reports **Advanced Feature**.

Use `INKEY$` for non-blocking keyboard polling. Newbasic does not provide per-function-key event traps.
