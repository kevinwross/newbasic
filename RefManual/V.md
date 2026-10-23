# V

<a id="val-function"></a>
## VAL Function

**Type:** Function  
**Status:** Supported  
**See also:** `STR$`, `ASC`

### Purpose

Converts a numeric string to a number.

### Syntax

```basic
VAL(string$)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `string$` | Yes | String containing a number. |

### Comments

Leading spaces are ignored. Parsing stops at the first character that cannot be part of a number. If no number can be read, the result is `0`. `VAL` understands decimal and `&H`/`&O` forms.

### Examples

#### Example 1 — Parse numeric text

```basic
10 PRINT VAL(" 42")
20 PRINT VAL("3.5XYZ")
30 PRINT VAL("&H10")
40 END
```

Output:

```text
 42
 3.5
 16
```

<a id="varptr-function"></a>
## VARPTR Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the machine-memory address of a variable or file control block.

### Syntax

```basic
VARPTR(variable)
VARPTR(#file number)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. Variables are not exposed as fixed machine-memory addresses.

<a id="varptr-function-2"></a>
## VARPTR$ Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns a string containing a variable’s address in a form usable by `DRAW`.

### Syntax

```basic
VARPTR$(variable)
```

### Newbasic status

Not implemented; there is no machine-address representation.

<a id="view-print-statement"></a>
## VIEW PRINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `VIEW`, `LOCATE`, `CLS`

### Purpose

Restricts text output and scrolling to a text window.

### Syntax

```basic
VIEW PRINT [top TO bottom]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `top` | No | First row of the text window. |
| `bottom` | No | Last row of the text window. |

### Comments

With no arguments, the whole grid is the text window. Printing wraps within the window, and scrolling affects only that window. The default is the whole grid rather than lines 1–24 because newbasic has no function-key bar unless `KEY ON` is used.

### Examples

#### Example 1 — Confine output

```basic
10 CLS
20 VIEW PRINT 5 TO 10
30 FOR I=1 TO 10: PRINT I: NEXT I
40 END
```
<a id="view-statement"></a>
## VIEW Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `WINDOW`, `CLS`, `SCREEN`

### Purpose

Restricts graphics drawing to a viewport.

### Syntax

```basic
VIEW [(x1, y1)-(x2, y2) [, [fill] [, [border]]]]
VIEW
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x1`, `y1` | No | Upper-left corner of the viewport. |
| `x2`, `y2` | No | Lower-right corner. |
| `fill` | No | Fill attribute for the viewport. |
| `border` | No | Border attribute. |

### Comments

With no arguments, `VIEW` restores the full screen. Drawing is clipped to the viewport and coordinates are relative to its upper-left corner. A `VIEW` does not clear existing pixels. While a viewport is active, `CLS 1` clears only its interior, keeps the viewport and coordinate mapping in place, and uses the fill colour when one was supplied; `CLS` and `CLS 0` clear the whole graphics canvas.

### Examples

#### Example 1 — Draw inside a viewport

```basic
10 SCREEN 1
20 VIEW (50,40)-(270,160)
30 LINE (0,0)-(100,100),2
40 END
```

