# Entry template

Copy this block into the appropriate letter file, replace the placeholders, and keep entries alphabetical within the file.

~~~~markdown
## NAME Type

**Type:** Statement | Function | Command | Variable  
**Status:** Supported | Partial | Not supported | Direct mode only  
**See also:** `RELATED`, `RELATED`

### Purpose

One or two sentences describing what the entry does.

### Syntax

```basic
SYNTAX [optional] required
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `name` | Yes | What it is and what values it accepts. |
| `optional` | No | What happens when it is omitted. |

### Comments

Explain newbasic-specific behaviour, limits, error conditions, and differences from GW-BASIC.

### Examples

#### Example 1 — Short description

```basic
10 REM Up to 30 lines, runnable.
20 PRINT "example"
30 END
```

Output:

```text
example
```
~~~~

For an unsupported GW-BASIC keyword, use:

~~~~markdown
## NAME Type

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

The GW-BASIC purpose.

### Syntax

```basic
GW-BASIC syntax
```

### Newbasic status

State the exact behaviour: syntax error, Advanced Feature, or direct-mode-only. Include the reason.
~~~~
