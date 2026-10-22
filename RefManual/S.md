# S

<a id="save-command"></a>
## SAVE Command

**Type:** Command  
**Status:** Supported  
**See also:** `LOAD`, `MERGE`

### Purpose

Saves the stored program to disk.

### Syntax

```basic
SAVE filename [, A|B|P]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `filename` | Yes | Destination file. |
| `A` | No | ASCII/plain-text form. This is the default for `SAVE`. |
| `B` | No | Tokenized GW-BASIC binary form. |
| `P` | No | Protected tokenized form. |

### Comments

`SAVE` with no modifier writes plain text, a deliberate inversion of GW-BASIC’s default. `SAVE ,B` and `SAVE ,P` write binary forms. Protected-file byte fidelity with Microsoft files is not proven.

### Examples

```text
SAVE "HELLO.BAS"
SAVE "HELLO.BAS", B
```

<a id="screen-function"></a>
## SCREEN Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the character or colour attribute at a text-screen position.

### Syntax

```basic
SCREEN(row, column [, colour flag])
```

### Newbasic status

Not implemented as a function. Newbasic exposes cursor position through `CSRLIN` and `POS`; text-grid access is not part of the dialect.

<a id="screen-statement"></a>
## SCREEN Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `COLOR`, `PALETTE`, `CLS`, `WIDTH`

### Purpose

Selects a text or graphics screen mode.

### Syntax

```basic
SCREEN [mode] [, [colour switch] [, [active page] [, [visible page]]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `mode` | No | `0`–`13`, or the newbasic extension `720`. Default is text mode. |
| `colour switch` | No | Accepted for compatibility; newbasic uses the mode’s palette. |
| `active page` | No | Accepted for compatibility; page flipping is not implemented. |
| `visible page` | No | Accepted for compatibility; page flipping is not implemented. |

### Comments

Modes `0`–`13` keep an 80×25 text grid. Mode `720` is newbasic’s extension: 1280×720, 160×45 text, and a 256-entry palette. A mode change resets the palette and viewport. Invalid modes raise **Illegal function call**.

### Examples

#### Example 1 — Graphics circle

```basic
10 SCREEN 1
20 CIRCLE (160,100),50
30 END
```

#### Example 2 — Wide extension mode

```basic
10 SCREEN 720
20 PRINT "1280 x 720"
30 END
```

<a id="sgn-function"></a>
## SGN Function

**Type:** Function  
**Status:** Supported  
**See also:** `ABS`, `FIX`

### Purpose

Returns the sign of a number.

### Syntax

```basic
SGN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

Returns `-1` for negative, `0` for zero, and `1` for positive.

### Examples

#### Example 1 — Signs

```basic
10 PRINT SGN(-5); SGN(0); SGN(5)
20 END
```

Output:

```text
-1  0  1
```

<a id="shell-statement"></a>
## SHELL Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SYSTEM`

### Purpose

Runs a host command or starts a command interpreter.

### Syntax

```basic
SHELL [command$]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `command$` | No | Command string. Without it, newbasic starts an interactive shell. |

### Comments

`SHELL` hands control to the host OS. A bare `SHELL` requires an interactive terminal; with redirected input it reports a clear error rather than hanging. The command interpreter must be available on the host.

### Examples

#### Example 1 — Run a host command

```basic
10 SHELL "echo hello"
20 END
```

<a id="sin-function"></a>
## SIN Function

**Type:** Function  
**Status:** Supported  
**See also:** `COS`, `TAN`, `ATN`

### Purpose

Returns the sine of an angle in radians.

### Syntax

```basic
SIN(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Angle in radians. |

### Comments

`SIN(0)` is `0`. Use `ATN(1)*4` for π if an approximation is not sufficient.

### Examples

#### Example 1 — Sine values

```basic
10 PRINT SIN(0)
20 PRINT SIN(3.14159265/2)
30 END
```

Output:

```text
 0
 1
```

<a id="sound-statement"></a>
## SOUND Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `BEEP`, `PLAY`

### Purpose

Sounds a tone for a specified duration.

### Syntax

```basic
SOUND frequency, duration
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `frequency` | Yes | Tone frequency in Hz. |
| `duration` | Yes | Duration in clock ticks (about 18.2 per second). |

### Comments

`SOUND` blocks until the tone finishes. Frequencies are clamped to the supported range. The duration is capped for safety. Under a headless host the call is silent.

### Examples

#### Example 1 — Two tones

```basic
10 SOUND 440, 9
20 SOUND 880, 9
30 END
```

<a id="space-function"></a>
## SPACE$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `STRING$`, `SPC`

### Purpose

Returns a string of spaces.

### Syntax

```basic
SPACE$(count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of spaces. |

### Comments

`count` must be non-negative. `SPACE$(0)` is the empty string.

### Examples

#### Example 1 — Build padding

```basic
10 A$="["+SPACE$(5)+"]"
20 PRINT A$
30 END
```

<a id="spc-function"></a>
## SPC Function

**Type:** Function  
**Status:** Supported in PRINT only  
**See also:** `TAB`, `SPACE$`

### Purpose

Skips spaces within a `PRINT` or `LPRINT` item list.

### Syntax

```basic
SPC(count)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of spaces to advance. |

### Comments

`SPC` is valid only inside `PRINT`, `LPRINT`, `PRINT#`, or `LPRINT USING` item lists. It moves the cursor without producing visible characters.

### Examples

#### Example 1 — Space output

```basic
10 PRINT "A"; SPC(5); "B"
20 END
```

Output:

```text
A     B
```

<a id="sqr-function"></a>
## SQR Function

**Type:** Function  
**Status:** Supported  
**See also:** `EXP`, `LOG`

### Purpose

Returns the square root of a non-negative number.

### Syntax

```basic
SQR(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Non-negative numeric expression. |

### Comments

A negative argument raises **Illegal function call**.

### Examples

#### Example 1 — Square roots

```basic
10 PRINT SQR(9)
20 PRINT SQR(2)
30 END
```

Output:

```text
 3
 1.414214
```

<a id="stick-function"></a>
## STICK Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns a joystick axis position.

### Syntax

```basic
STICK(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick device.

<a id="stop-statement"></a>
## STOP Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `END`, `CONT`

### Purpose

Stops program execution and returns to direct mode.

### Syntax

```basic
STOP
```

### Comments

`STOP` saves a continue point; `CONT` can resume. `END` ends without a continue point. No error is reported.

### Examples

#### Example 1 — Stop and continue

```basic
10 PRINT "BEFORE"
20 STOP
30 PRINT "AFTER"
40 END
```

Run, then type `CONT` at the prompt to print `AFTER`.

<a id="str-function"></a>
## STR$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `VAL`, `PRINT`

### Purpose

Converts a number to its string representation.

### Syntax

```basic
STR$(number)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `number` | Yes | Numeric expression. |

### Comments

The result contains the same leading sign space that `PRINT` uses for positive numbers. If the leading space is not wanted, remove it with `MID$(s$, 2)`.

### Examples

#### Example 1 — Convert and concatenate

```basic
10 A=42
20 PRINT "VALUE=" + STR$(A)
30 END
```

<a id="strig-function"></a>
## STRIG Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the current or latched state of a joystick trigger.

### Syntax

```basic
STRIG(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick device.

<a id="strig-statement"></a>
## STRIG Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Enables, disables, or stops joystick-trigger trapping.

### Syntax

```basic
STRIG(n) ON
STRIG(n) OFF
STRIG(n) STOP
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no joystick or trigger device.

<a id="string-function"></a>
## STRING$ Function

**Type:** Function  
**Status:** Supported  
**See also:** `SPACE$`, `CHR$`

### Purpose

Returns a string consisting of repeated characters.

### Syntax

```basic
STRING$(count, character)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `count` | Yes | Number of characters. |
| `character` | Yes | Character code or one-character string. |

### Comments

`count` must be non-negative. A string argument uses its first character. A numeric argument is taken as a character code.

### Examples

#### Example 1 — Build a separator

```basic
10 PRINT STRING$(10, "-")
20 PRINT STRING$(5, 65)
30 END
```

Output:

```text
----------
AAAAA
```

<a id="swap-statement"></a>
## SWAP Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `LET`, `DIM`

### Purpose

Exchanges the values of two variables or array elements.

### Syntax

```basic
SWAP variable1, variable2
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `variable1`, `variable2` | Yes | Scalar variables or array elements of compatible type. |

### Comments

`SWAP` exchanges values without a temporary variable. Both targets must be assignable and of compatible type. It works with strings, numeric variables, and array elements.

### Examples

#### Example 1 — Swap two values

```basic
10 A=1: B=2
20 SWAP A, B
30 PRINT A; B
40 END
```

Output:

```text
 2  1
```

<a id="system-command"></a>
## SYSTEM Command

**Type:** Command  
**Status:** Direct mode only  
**See also:** `SHELL`

### Purpose

Exits newbasic and returns to the host operating system.

### Syntax

```basic
SYSTEM
```

### Comments

`SYSTEM` is a direct-mode command. It does not save the current program automatically. Use `SAVE` first.

### Examples

```text
SYSTEM
```
