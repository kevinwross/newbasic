# B

<a id="beep-statement"></a>
## BEEP Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SOUND`, `PLAY`

### Purpose

Sounds the system beep.

### Syntax

```basic
BEEP
```

### Arguments

None.

### Comments

`BEEP` is a short, fixed tone. On Windows it uses the same tone synthesiser as `SOUND`; under a headless host it is silent. It does not change the text cursor or screen contents.

### Examples

#### Example 1 — Signal completion

```basic
10 PRINT "Press a key"
20 BEEP
30 END
```

<a id="bload-command"></a>
## BLOAD Command

**Type:** Command  
**Status:** Not supported in newbasic

### Purpose

Loads a binary memory image from a file.

### Syntax

```basic
BLOAD filename [, offset]
```

### Newbasic status

Not implemented. Newbasic deliberately has no machine-memory model. Programs that need data files should use `OPEN`, `INPUT#`, `GET`, or `LOAD` for BASIC programs.

<a id="bsave-command"></a>
## BSAVE Command

**Type:** Command  
**Status:** Not supported in newbasic

### Purpose

Saves a region of machine memory to a binary file.

### Syntax

```basic
BSAVE filename, offset, length
```

### Newbasic status

Not implemented for the same reason as `BLOAD`: there is no emulated 64K memory space or addressable binary image.
