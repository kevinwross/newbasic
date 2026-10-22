# W

<a id="wait-statement"></a>
## WAIT Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Waits for a hardware port to have a specified bit pattern.

### Syntax

```basic
WAIT port, and mask [, xor mask]
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no I/O-port model.

<a id="wend-statement"></a>
## WEND Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `WHILE`

### Purpose

Closes a `WHILE` loop.

### Syntax

```basic
WEND
```

### Comments

`WEND` returns execution to the matching `WHILE` condition. It is not a general loop terminator; use `NEXT` for `FOR` loops.

### Examples

#### Example 1 — Pair with WHILE

```basic
10 I=0
20 WHILE I<2
30 PRINT "X"
40 I=I+1
50 WEND
60 END
```

<a id="while-statement"></a>
## WHILE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `WEND`, `FOR`

### Purpose

Begins a condition-controlled loop.

### Syntax

```basic
WHILE condition
...
WEND
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `condition` | Yes | Numeric expression; nonzero continues the loop. |

### Comments

The condition is tested before each iteration. `WEND` jumps back to its matching `WHILE`. A missing `WEND` raises **WHILE without WEND**; a stray `WEND` raises **WEND without WHILE**. Loops may be nested.

### Examples

#### Example 1 — Count with WHILE

```basic
10 I=1
20 WHILE I<=3
30 PRINT I
40 I=I+1
50 WEND
60 END
```

Output:

```text
 1
 2
 3
```

<a id="width-statement"></a>
## WIDTH Statement

**Type:** Statement  
**Status:** Partial  
**See also:** `SCREEN`, `VIEW PRINT`

### Purpose

Changes the number of text columns used for printing and wrapping.

### Syntax

```basic
WIDTH columns
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `columns` | Yes | New text width. Newbasic accepts up to `80`. |

### Comments

`WIDTH` changes the text grid and wrapping width; it does not resize the physical window. The standard maximum is 80 columns. `WIDTH 40` is useful for narrow output. Device-name forms such as `WIDTH "LPT1:", 80` are refused.

### Examples

#### Example 1 — Narrow printing

```basic
10 WIDTH 40
20 PRINT STRING$(60, "A")
30 END
```

<a id="window-statement"></a>
## WINDOW Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `VIEW`, `PMAP`, `POINT`

### Purpose

Maps a world-coordinate rectangle onto the graphics screen.

### Syntax

```basic
WINDOW [[SCREEN](x1, y1)-(x2, y2)]
WINDOW
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x1`, `y1` | No | World coordinate mapped to the upper-left corner. |
| `x2`, `y2` | No | World coordinate mapped to the lower-right corner. |
| `SCREEN` | No | Makes the y axis increase downward. |

### Comments

With no arguments, `WINDOW` restores screen coordinates. A window changes the interpretation of graphics coordinates, not the canvas size. `PMAP` converts between world and screen coordinates.

### Examples

#### Example 1 — Map a world rectangle

```basic
10 SCREEN 1
20 WINDOW (0,0)-(100,100)
30 PSET (50,50),2
40 PRINT POINT(50,50)
50 END
```

<a id="write-statement"></a>
## WRITE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT`, `WRITE#`

### Purpose

Writes comma-separated, quoted values to the screen.

### Syntax

```basic
WRITE [expression [, expression ...]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | No | Values to write. |

### Comments

Strings are enclosed in double quotes; values are separated by commas. `WRITE` with no items prints a blank line. It is useful for producing data that `INPUT` can read back.

### Examples

#### Example 1 — Quoted output

```basic
10 WRITE "ALICE", 30
20 WRITE 1, 2, 3
30 END
```

Output:

```text
"ALICE",30
1,2,3
```

<a id="write-statement-2"></a>
## WRITE# Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `WRITE`, `INPUT#`, `OPEN`

### Purpose

Writes comma-separated, quoted values to a sequential file.

### Syntax

```basic
WRITE #file number, [expression [, expression ...]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `expression` | No | Values to write. |

### Comments

Strings are quoted and values separated by commas, matching `INPUT#` parsing. The file must be open for output or append. Each `WRITE#` writes one record line.

### Examples

#### Example 1 — Write a record

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 WRITE #1, "ALICE", 30
30 CLOSE #1
40 END
```
