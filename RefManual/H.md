# H

<a id="hex-function"></a>
## HEX$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `OCT$`, `STR$`, `CINT`

### Purpose

Returns the hexadecimal representation of a number.

### Syntax

```basic
HEX$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression; converted to an integer. |

### Comments

The result is uppercase hexadecimal with no leading `&H`. Negative values use two’s-complement form. Use `VAL("&H" + HEX$(n))` if a numeric value is needed again.

### Examples

#### Example 1 — Convert decimal values

```basic
10 PRINT HEX$(255)
20 PRINT HEX$(16)
30 PRINT HEX$(0)
40 END
```

Output:

```text
FF
10
0
```
