# G

<a id="get-statement"></a>
## GET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PUT`, `FIELD`, `OPEN`, `SCREEN`

### Purpose

Reads a random-access file record, or captures a graphics rectangle into an array.

### Syntax

```basic
GET #file number [, record number]
GET (x1, y1)-(x2, y2), array
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes (file form) | An open random-access file. |
| `record number` | No | Record to read. Default is the next record. |
| `x1`, `y1`, `x2`, `y2` | Yes (graphics form) | Rectangle to capture, inclusive of both corners. |
| `array` | Yes (graphics form) | Integer array receiving the image. |

### Comments

The file form fills the `FIELD` buffers; use `CVI`, `CVS`, or `CVD` to decode numeric fields. A record that has never been written reads as zero-filled data.

The graphics form stores width, height, then one palette index per pixel in row-major order. This is a documented divergence from GW-BASIC’s packed image format.

### Examples

#### Example 1 — Read a random record

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 GET #1, 1
40 PRINT A$
50 CLOSE #1
60 END
```

#### Example 2 — Capture and restore a graphics rectangle

```basic
10 SCREEN 1
20 CIRCLE (160,100),30,2
30 DIM G(6000)
40 GET (120,70)-(200,130), G
50 CLS
60 PUT (20,20), G
70 END
```

<a id="gosub-statement"></a>
## GOSUB Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `RETURN`, `ON ... GOSUB`

### Purpose

Calls a subroutine at a specified line.

### Syntax

```basic
GOSUB line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | Yes | Existing line number where the subroutine begins. |

### Comments

`GOSUB` pushes a return address; the subroutine must finish with `RETURN`. A `GOSUB` without a matching `RETURN` eventually raises **Return without GOSUB** when the stack is misused. Direct-mode `GOSUB` is not allowed.

### Examples

#### Example 1 — Call and return

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

<a id="goto-statement"></a>
## GOTO Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GOSUB`, `IF`, `ON ... GOTO`

### Purpose

Branches unconditionally to a line number.

### Syntax

```basic
GOTO line
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `line` | Yes | Existing line number. |

### Comments

`GOTO` is not allowed in direct mode. A target that does not exist raises **Undefined line number** at compile time or load time. Prefer structured loops where possible, but `GOTO` remains useful for menus and error recovery.

### Examples

#### Example 1 — Skip a line

```basic
10 GOTO 30
20 PRINT "SKIPPED"
30 PRINT "HERE"
40 END
```

Output:

```text
HERE
```
