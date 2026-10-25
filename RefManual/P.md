# P

<a id="paint-statement"></a>
## PAINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `CIRCLE`, `LINE`, `PSET`

### Purpose

Flood-fills a graphics region up to a border colour.

### Syntax

```basic
PAINT [STEP] (x, y) [, [fill] [, [border]]]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Seed point. `STEP` makes it relative to the last graphics point. |
| `fill` | No | Fill attribute. Default is the current foreground. |
| `border` | No | Stop at this attribute. Default is the seed colour. |

### Comments

`PAINT` requires a graphics mode. A fill or border outside the current palette range raises **Illegal function call**. A border of `-1` is accepted as “stop at any colour different from the seed.”

### Examples

#### Example 1 — Fill a circle

```basic
10 SCREEN 1
20 CIRCLE (160,100),50,2
30 PAINT (160,100),3,2
40 END
```

<a id="palette-statement"></a>
## PALETTE Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `COLOR`, `SCREEN`

### Purpose

Changes the display colour associated with a palette attribute.

### Syntax

```basic
PALETTE [attribute, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `attribute` | No | Palette slot to change. |
| `colour` | No | Display colour for that slot. |

### Comments

With no arguments, `PALETTE` restores the current mode’s default palette. In `SCREEN 13` and `SCREEN 720`, attributes range over 256 entries; earlier modes use the mode’s smaller palette. `PALETTE` changes the mapping, not the pixel’s stored attribute.

### SCREEN 720 default palette

`SCREEN 720` exposes 256 palette attributes. The table below is the default mapping: attribute numbers `0`–`15` are the standard CGA colours, `16`–`95` are an 80-step grayscale ramp, and `96`–`255` are a 160-step hue sweep. Indices `0` and `16` are both black, and `15` and `95` are both white.

Selecting `SCREEN 720` resets this palette; `PALETTE` with no arguments also restores it. `PALETTE attribute, colour` remaps an attribute to another display colour without changing the stored pixel attributes. The names below are suggested labels; repeated hue names use numeric suffixes only to distinguish entries.

| Color | RGB | Suggested name |
|---:|---|---|
| 0 | 0, 0, 0 | Black |
| 1 | 0, 0, 170 | Blue |
| 2 | 0, 170, 0 | Green |
| 3 | 0, 170, 170 | Cyan |
| 4 | 170, 0, 0 | Red |
| 5 | 170, 0, 170 | Magenta |
| 6 | 170, 85, 0 | Brown |
| 7 | 170, 170, 170 | Light gray |
| 8 | 85, 85, 85 | Dark gray |
| 9 | 85, 85, 255 | Light blue |
| 10 | 85, 255, 85 | Light green |
| 11 | 85, 255, 255 | Light cyan |
| 12 | 255, 85, 85 | Light red |
| 13 | 255, 85, 255 | Light magenta |
| 14 | 255, 255, 85 | Yellow |
| 15 | 255, 255, 255 | White |
| 16 | 0, 0, 0 | Black |
| 17 | 3, 3, 3 | Gray 3 |
| 18 | 6, 6, 6 | Gray 6 |
| 19 | 9, 9, 9 | Gray 9 |
| 20 | 12, 12, 12 | Gray 12 |
| 21 | 16, 16, 16 | Gray 16 |
| 22 | 19, 19, 19 | Gray 19 |
| 23 | 22, 22, 22 | Gray 22 |
| 24 | 25, 25, 25 | Gray 25 |
| 25 | 29, 29, 29 | Gray 29 |
| 26 | 32, 32, 32 | Gray 32 |
| 27 | 35, 35, 35 | Gray 35 |
| 28 | 38, 38, 38 | Gray 38 |
| 29 | 41, 41, 41 | Gray 41 |
| 30 | 45, 45, 45 | Gray 45 |
| 31 | 48, 48, 48 | Gray 48 |
| 32 | 51, 51, 51 | Gray 51 |
| 33 | 54, 54, 54 | Gray 54 |
| 34 | 58, 58, 58 | Gray 58 |
| 35 | 61, 61, 61 | Gray 61 |
| 36 | 64, 64, 64 | Gray 64 |
| 37 | 67, 67, 67 | Gray 67 |
| 38 | 71, 71, 71 | Gray 71 |
| 39 | 74, 74, 74 | Gray 74 |
| 40 | 77, 77, 77 | Gray 77 |
| 41 | 80, 80, 80 | Gray 80 |
| 42 | 83, 83, 83 | Gray 83 |
| 43 | 87, 87, 87 | Gray 87 |
| 44 | 90, 90, 90 | Gray 90 |
| 45 | 93, 93, 93 | Gray 93 |
| 46 | 96, 96, 96 | Gray 96 |
| 47 | 100, 100, 100 | Gray 100 |
| 48 | 103, 103, 103 | Gray 103 |
| 49 | 106, 106, 106 | Gray 106 |
| 50 | 109, 109, 109 | Gray 109 |
| 51 | 112, 112, 112 | Gray 112 |
| 52 | 116, 116, 116 | Gray 116 |
| 53 | 119, 119, 119 | Gray 119 |
| 54 | 122, 122, 122 | Gray 122 |
| 55 | 125, 125, 125 | Gray 125 |
| 56 | 129, 129, 129 | Gray 129 |
| 57 | 132, 132, 132 | Gray 132 |
| 58 | 135, 135, 135 | Gray 135 |
| 59 | 138, 138, 138 | Gray 138 |
| 60 | 142, 142, 142 | Gray 142 |
| 61 | 145, 145, 145 | Gray 145 |
| 62 | 148, 148, 148 | Gray 148 |
| 63 | 151, 151, 151 | Gray 151 |
| 64 | 154, 154, 154 | Gray 154 |
| 65 | 158, 158, 158 | Gray 158 |
| 66 | 161, 161, 161 | Gray 161 |
| 67 | 164, 164, 164 | Gray 164 |
| 68 | 167, 167, 167 | Gray 167 |
| 69 | 171, 171, 171 | Gray 171 |
| 70 | 174, 174, 174 | Gray 174 |
| 71 | 177, 177, 177 | Gray 177 |
| 72 | 180, 180, 180 | Gray 180 |
| 73 | 183, 183, 183 | Gray 183 |
| 74 | 187, 187, 187 | Gray 187 |
| 75 | 190, 190, 190 | Gray 190 |
| 76 | 193, 193, 193 | Gray 193 |
| 77 | 196, 196, 196 | Gray 196 |
| 78 | 200, 200, 200 | Gray 200 |
| 79 | 203, 203, 203 | Gray 203 |
| 80 | 206, 206, 206 | Gray 206 |
| 81 | 209, 209, 209 | Gray 209 |
| 82 | 213, 213, 213 | Gray 213 |
| 83 | 216, 216, 216 | Gray 216 |
| 84 | 219, 219, 219 | Gray 219 |
| 85 | 222, 222, 222 | Gray 222 |
| 86 | 225, 225, 225 | Gray 225 |
| 87 | 229, 229, 229 | Gray 229 |
| 88 | 232, 232, 232 | Gray 232 |
| 89 | 235, 235, 235 | Gray 235 |
| 90 | 238, 238, 238 | Gray 238 |
| 91 | 242, 242, 242 | Gray 242 |
| 92 | 245, 245, 245 | Gray 245 |
| 93 | 248, 248, 248 | Gray 248 |
| 94 | 251, 251, 251 | Gray 251 |
| 95 | 255, 255, 255 | White |
| 96 | 255, 0, 0 | Red-orange |
| 97 | 246, 9, 0 | Red-orange 2 |
| 98 | 236, 19, 0 | Red-orange 3 |
| 99 | 226, 29, 0 | Red-orange 4 |
| 100 | 216, 39, 0 | Red-orange 5 |
| 101 | 206, 49, 0 | Red-orange 6 |
| 102 | 197, 58, 0 | Red-orange 7 |
| 103 | 187, 68, 0 | Orange |
| 104 | 177, 78, 0 | Orange 2 |
| 105 | 167, 88, 0 | Orange 3 |
| 106 | 157, 98, 0 | Orange 4 |
| 107 | 148, 107, 0 | Orange 5 |
| 108 | 138, 117, 0 | Orange 6 |
| 109 | 128, 127, 0 | Orange 7 |
| 110 | 118, 137, 0 | Amber |
| 111 | 108, 147, 0 | Amber 2 |
| 112 | 99, 156, 0 | Amber 3 |
| 113 | 89, 166, 0 | Amber 4 |
| 114 | 79, 176, 0 | Amber 5 |
| 115 | 69, 186, 0 | Amber 6 |
| 116 | 59, 196, 0 | Yellow |
| 117 | 50, 205, 0 | Yellow 2 |
| 118 | 40, 215, 0 | Yellow 3 |
| 119 | 30, 225, 0 | Yellow 4 |
| 120 | 20, 235, 0 | Yellow 5 |
| 121 | 10, 245, 0 | Yellow 6 |
| 122 | 255, 0, 0 | Red-orange 8 |
| 123 | 0, 246, 9 | Green |
| 124 | 0, 236, 19 | Green 2 |
| 125 | 0, 226, 29 | Green 3 |
| 126 | 0, 216, 39 | Green 4 |
| 127 | 0, 206, 49 | Green 5 |
| 128 | 0, 197, 58 | Green 6 |
| 129 | 0, 187, 68 | Emerald |
| 130 | 0, 177, 78 | Emerald 2 |
| 131 | 0, 167, 88 | Emerald 3 |
| 132 | 0, 157, 98 | Emerald 4 |
| 133 | 0, 148, 107 | Emerald 5 |
| 134 | 0, 138, 117 | Emerald 6 |
| 135 | 0, 128, 127 | Emerald 7 |
| 136 | 0, 118, 137 | Cyan |
| 137 | 0, 108, 147 | Cyan 2 |
| 138 | 0, 99, 156 | Cyan 3 |
| 139 | 0, 89, 166 | Cyan 4 |
| 140 | 0, 79, 176 | Cyan 5 |
| 141 | 0, 69, 186 | Cyan 6 |
| 142 | 0, 59, 196 | Blue |
| 143 | 0, 50, 205 | Blue 2 |
| 144 | 0, 40, 215 | Blue 3 |
| 145 | 0, 30, 225 | Blue 4 |
| 146 | 0, 20, 235 | Blue 5 |
| 147 | 0, 10, 245 | Blue 6 |
| 148 | 0, 255, 0 | Green 7 |
| 149 | 0, 246, 9 | Green 8 |
| 150 | 19, 0, 236 | Blue 7 |
| 151 | 29, 0, 226 | Blue 8 |
| 152 | 39, 0, 216 | Blue 9 |
| 153 | 49, 0, 206 | Blue 10 |
| 154 | 58, 0, 197 | Blue 11 |
| 155 | 68, 0, 187 | Violet |
| 156 | 78, 0, 177 | Violet 2 |
| 157 | 88, 0, 167 | Violet 3 |
| 158 | 98, 0, 157 | Violet 4 |
| 159 | 107, 0, 148 | Violet 5 |
| 160 | 117, 0, 138 | Violet 6 |
| 161 | 127, 0, 128 | Violet 7 |
| 162 | 137, 0, 118 | Magenta |
| 163 | 147, 0, 108 | Magenta 2 |
| 164 | 156, 0, 99 | Magenta 3 |
| 165 | 166, 0, 89 | Magenta 4 |
| 166 | 176, 0, 79 | Magenta 5 |
| 167 | 186, 0, 69 | Magenta 6 |
| 168 | 196, 0, 59 | Red |
| 169 | 205, 0, 50 | Red 2 |
| 170 | 215, 0, 40 | Red 3 |
| 171 | 225, 0, 30 | Red 4 |
| 172 | 235, 0, 20 | Red 5 |
| 173 | 245, 0, 10 | Red 6 |
| 174 | 0, 0, 255 | Blue 12 |
| 175 | 9, 0, 246 | Blue 13 |
| 176 | 236, 0, 19 | Red 7 |
| 177 | 226, 0, 29 | Red 8 |
| 178 | 216, 0, 39 | Red 9 |
| 179 | 206, 0, 49 | Red 10 |
| 180 | 197, 0, 58 | Red 11 |
| 181 | 187, 0, 68 | Rose |
| 182 | 177, 0, 78 | Rose 2 |
| 183 | 167, 0, 88 | Rose 3 |
| 184 | 157, 0, 98 | Rose 4 |
| 185 | 148, 0, 107 | Rose 5 |
| 186 | 138, 0, 117 | Rose 6 |
| 187 | 128, 0, 127 | Rose 7 |
| 188 | 118, 0, 137 | Pink |
| 189 | 108, 0, 147 | Pink 2 |
| 190 | 99, 0, 156 | Pink 3 |
| 191 | 89, 0, 166 | Pink 4 |
| 192 | 79, 0, 176 | Pink 5 |
| 193 | 69, 0, 186 | Pink 6 |
| 194 | 59, 0, 196 | Magenta 7 |
| 195 | 50, 0, 205 | Magenta 8 |
| 196 | 40, 0, 215 | Magenta 9 |
| 197 | 30, 0, 225 | Magenta 10 |
| 198 | 20, 0, 235 | Magenta 11 |
| 199 | 10, 0, 245 | Magenta 12 |
| 200 | 255, 0, 0 | Red 12 |
| 201 | 246, 0, 9 | Red 13 |
| 202 | 236, 0, 19 | Red 14 |
| 203 | 0, 29, 226 | Blue 14 |
| 204 | 0, 39, 216 | Blue 15 |
| 205 | 0, 49, 206 | Blue 16 |
| 206 | 0, 58, 197 | Blue 17 |
| 207 | 0, 68, 187 | Azure |
| 208 | 0, 78, 177 | Azure 2 |
| 209 | 0, 88, 167 | Azure 3 |
| 210 | 0, 98, 157 | Azure 4 |
| 211 | 0, 107, 148 | Azure 5 |
| 212 | 0, 117, 138 | Azure 6 |
| 213 | 0, 127, 128 | Azure 7 |
| 214 | 0, 137, 118 | Turquoise |
| 215 | 0, 147, 108 | Turquoise 2 |
| 216 | 0, 156, 99 | Turquoise 3 |
| 217 | 0, 166, 89 | Turquoise 4 |
| 218 | 0, 176, 79 | Turquoise 5 |
| 219 | 0, 186, 69 | Turquoise 6 |
| 220 | 0, 196, 59 | Green 9 |
| 221 | 0, 205, 50 | Green 10 |
| 222 | 0, 215, 40 | Green 11 |
| 223 | 0, 225, 30 | Green 12 |
| 224 | 0, 235, 20 | Green 13 |
| 225 | 0, 245, 10 | Green 14 |
| 226 | 0, 0, 255 | Blue 18 |
| 227 | 0, 9, 246 | Blue 19 |
| 228 | 0, 19, 236 | Blue 20 |
| 229 | 0, 29, 226 | Blue 21 |
| 230 | 39, 216, 0 | Green 15 |
| 231 | 49, 206, 0 | Green 16 |
| 232 | 58, 197, 0 | Green 17 |
| 233 | 68, 187, 0 | Lime |
| 234 | 78, 177, 0 | Lime 2 |
| 235 | 88, 167, 0 | Lime 3 |
| 236 | 98, 157, 0 | Lime 4 |
| 237 | 107, 148, 0 | Lime 5 |
| 238 | 117, 138, 0 | Lime 6 |
| 239 | 127, 128, 0 | Lime 7 |
| 240 | 137, 118, 0 | Yellow 7 |
| 241 | 147, 108, 0 | Yellow 8 |
| 242 | 156, 99, 0 | Yellow 9 |
| 243 | 166, 89, 0 | Yellow 10 |
| 244 | 176, 79, 0 | Yellow 11 |
| 245 | 186, 69, 0 | Yellow 12 |
| 246 | 196, 59, 0 | Orange-red |
| 247 | 205, 50, 0 | Orange-red 2 |
| 248 | 215, 40, 0 | Orange-red 3 |
| 249 | 225, 30, 0 | Orange-red 4 |
| 250 | 235, 20, 0 | Orange-red 5 |
| 251 | 245, 10, 0 | Orange-red 6 |
| 252 | 0, 255, 0 | Green 18 |
| 253 | 9, 246, 0 | Green 19 |
| 254 | 19, 236, 0 | Green 20 |
| 255 | 29, 226, 0 | Green 21 |

### Examples

#### Example 1 — Remap a colour

```basic
10 SCREEN 13
20 PALETTE 1, 63
30 PSET (10,10), 1
40 END
```

<a id="palette-using-statement"></a>
## PALETTE USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PALETTE`, `SCREEN`, `COLOR`

### Purpose

Sets many palette entries from an integer array in one operation.

### Syntax

```basic
PALETTE USING integer-array[(first index)]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `integer-array` | Yes | An integer array holding palette colour numbers. |
| `first index` | No | First array element to read. Defaults to the array’s base index. |

### Comments

Entries are copied starting at palette attribute `0`. The number of entries is the current mode’s palette size: 16 for the compatible graphics modes, or 256 for `SCREEN 13` and `SCREEN 720`.

An array value of `-1` leaves the corresponding palette entry unchanged. Other negative values are invalid. In the 16-colour modes, colours must be in the range `0`–`63`; in the 256-colour modes they must be in `0`–`0x3FFFFF`.

The array must be large enough to supply every palette entry from the chosen starting index. A non-integer array, a short array, or an out-of-range value raises **Illegal function call**.

`PALETTE USING` changes the same mapping as `PALETTE`; the existing screen pixels update immediately. `PALETTE` with no arguments still restores the mode’s default palette.

### Examples

#### Example 1 — Install a full 16-entry palette

```basic
10 SCREEN 1
20 DIM P%(15)
30 FOR I=0 TO 15: P%(I)=I: NEXT I
40 PALETTE USING P%(0)
50 PSET (10,10), 7
60 END
```

#### Example 2 — Leave some entries unchanged

```basic
10 SCREEN 1
20 DIM P%(15)
30 FOR I=0 TO 15: P%(I)=-1: NEXT I
40 P%(1)=5
50 P%(2)=9
60 PALETTE USING P%(0)
70 PSET (10,10), 1
80 END
```

#### Example 3 — Start at a later array element

```basic
10 SCREEN 1
20 DIM P%(20)
30 FOR I=0 TO 20: P%(I)=1: NEXT I
40 P%(5)=9
50 P%(6)=10
60 PALETTE USING P%(5)
70 END
```

<a id="pcopy-command"></a>
## PCOPY Command

**Type:** Command  
**Status:** Supported  
**See also:** `SCREEN`, `GET`, `PUT`

### Purpose

Copies one graphics page to another.

### Syntax

```basic
PCOPY source, destination
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `source` | Yes | Source page `0`–`7`. |
| `destination` | Yes | Destination page `0`–`7`. |

### Comments

Page `0` is the displayed canvas; pages `1`–`7` are snapshots. The source page must contain an image. Invalid pages raise **Illegal function call**.

### Examples

#### Example 1 — Save and restore a page

```basic
10 SCREEN 1
20 CIRCLE (160,100),40,2
30 PCOPY 0,1
40 CLS
50 PCOPY 1,0
60 END
```

<a id="peek-function"></a>
## PEEK Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Reads a byte from machine memory.

### Syntax

```basic
PEEK(address)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no emulated 64K address space.

<a id="pen-function"></a>
## PEN Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns light-pen coordinates and pen-down state.

### Syntax

```basic
PEN(n)
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no light-pen device.

<a id="pen-statement"></a>
## PEN Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Enables, disables, or stops light-pen trapping.

### Syntax

```basic
PEN ON
PEN OFF
PEN STOP
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no light-pen device.

<a id="play-statement"></a>
## PLAY Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `SOUND`, `BEEP`, `PLAY()`

### Purpose

Plays a music string.

### Syntax

```basic
PLAY command$
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `command$` | Yes | Music macro string: notes, octaves, tempo, length, rests, and articulation. |

### Comments

Notes play synchronously; `PLAY` blocks until the string finishes. `MB` is treated as `MF` (no background note queue). `Xstring;` is not supported. Tones are capped at five seconds. `PLAY` requires no graphics mode.

### Examples

#### Example 1 — Play a short scale

```basic
10 PLAY "T120 O4 L8 C D E F G A B"
20 END
```

<a id="play-function"></a>
## PLAY() Function

**Type:** Function  
**Status:** Not supported in newbasic

### Purpose

Returns the number of notes currently in the background music queue.

### Syntax

```basic
PLAY(n)
```

### Newbasic status

Not implemented because newbasic has no background note queue. `PLAY` is synchronous.

<a id="pmap-function"></a>
## PMAP Function

**Type:** Function  
**Status:** Supported  
**See also:** `WINDOW`, `POINT`

### Purpose

Maps a coordinate between world coordinates and screen pixels.

### Syntax

```basic
PMAP(coordinate, function)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `coordinate` | Yes | World or screen coordinate. |
| `function` | Yes | `0` maps x world→screen, `1` maps y world→screen, `2` maps x screen→world, `3` maps y screen→world. |

### Comments

`PMAP` is meaningful only while a `WINDOW` is active. Without one, coordinates are effectively identity mappings. The function number is required.

### Examples

#### Example 1 — Map a world coordinate

```basic
10 SCREEN 1
20 WINDOW (0,0)-(320,200)
30 PRINT PMAP(160,0)
40 PRINT PMAP(100,1)
50 END
```

<a id="point-function"></a>
## POINT Function

**Type:** Function  
**Status:** Supported  
**See also:** `PSET`, `PMAP`, `WINDOW`

### Purpose

Returns the colour attribute of a graphics pixel.

### Syntax

```basic
POINT(x, y)
POINT(function)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes (pixel form) | Pixel coordinates. |
| `function` | Yes (coordinate form) | `0` returns the last x, `1` the last y, `2` the last x under `WINDOW`, `3` the last y under `WINDOW`. |

### Comments

The pixel form returns `-1` for an out-of-range point. The coordinate form is useful after `DRAW` or `LINE` to query the current graphics point.

### Examples

#### Example 1 — Read a pixel

```basic
10 SCREEN 1
20 PSET (10,20),2
30 PRINT POINT(10,20)
40 PRINT POINT(0)
50 PRINT POINT(1)
60 END
```

<a id="poke-statement"></a>
## POKE Statement

**Type:** Statement  
**Status:** Not supported in newbasic

### Purpose

Writes a byte to machine memory.

### Syntax

```basic
POKE address, value
```

### Newbasic status

Recognised and refused with **Advanced Feature**. There is no addressable machine memory.

<a id="pop-view-statement"></a>
## POP VIEW Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `PUSH VIEW`, `VIEW`

### Purpose

Restores the graphics viewport saved by the matching `PUSH VIEW`.

### Syntax

```basic
POP VIEW
```

### Comments

`PUSH VIEW` and `POP VIEW` form a stack. `POP VIEW` restores the exact viewport rectangle, fill colour, and clipping state that was current at the matching push. The border is not redrawn; it is a one-shot `VIEW` effect. Popping an empty stack raises **Illegal function call**. The stack is cleared by `SCREEN` and `CLEAR`.

### Examples

#### Example 1 — Temporary viewport

```basic
10 SCREEN 1
20 VIEW (10,10)-(100,100)
30 PUSH VIEW
40 VIEW (20,20)-(80,80)
50 POP VIEW
60 END
```

<a id="pos-function"></a>
## POS Function

**Type:** Function  
**Status:** Supported  
**See also:** `CSRLIN`, `LOCATE`, `PRINT`

### Purpose

Returns the current text cursor column.

### Syntax

```basic
POS(dummy)
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `dummy` | Yes | Accepted for compatibility; the value is ignored. |

### Comments

The result is 1-based. `POS(0)` is conventional. `POS` reads the cursor; it does not move it.

### Examples

#### Example 1 — Column after printing

```basic
10 CLS
20 PRINT "ABC";
30 PRINT POS(0)
40 END
```

Output:

```text
 4
```

<a id="preset-statement"></a>
## PRESET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PSET`, `POINT`

### Purpose

Draws a point in the background colour.

### Syntax

```basic
PRESET [STEP] (x, y) [, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Point coordinates. `STEP` makes them relative. |
| `colour` | No | Attribute. Default is `0` (background). |

### Comments

`PRESET` is the inverse of `PSET`; it defaults to colour `0`. The point is clipped to the graphics canvas. `PRESET` updates the last graphics point.

### Examples

#### Example 1 — Erase a pixel

```basic
10 SCREEN 1
20 PSET (50,50),2
30 PRESET (50,50)
40 END
```

<a id="print-statement"></a>
## PRINT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT USING`, `PRINT#`, `LPRINT`

### Purpose

Writes values to the screen.

### Syntax

```basic
PRINT [expression] [;|, ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `expression` | No | Value to print. |
| `;` | No | Concatenate the next item directly. |
| `,` | No | Advance to the next print zone. |

### Comments

Numbers are printed with a leading space for a positive sign and a trailing space. A trailing semicolon suppresses the newline. `PRINT` with no items prints a blank line. `?` is shorthand for `PRINT`.

### Examples

#### Example 1 — Print values

```basic
10 A=5
20 PRINT "A="; A
30 PRINT 1, 2, 3
40 PRINT
50 END
```

Output:

```text
A= 5
 1          2          3
```

<a id="print-using-statement"></a>
## PRINT USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT`, `LPRINT USING`

### Purpose

Prints values under the control of a format string.

### Syntax

```basic
PRINT USING format$; expression [, expression ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `format$` | Yes | Field format string. |
| `expression` | Yes | Values to format. |

### Comments

Numeric fields use `#`, decimal point, comma, `+`, `-`, `$$`, `**`, and `^^^^`. String fields use `&` for the whole string or `!` for one character. Literal text in the format is printed as-is. A field with no matching expression stops formatting.

### Examples

#### Example 1 — Format a number

```basic
10 PRINT USING "###.##"; 12.345
20 PRINT USING "$$###.##"; 12.5
30 PRINT USING "! ! !"; "ABC"
40 END
```

<a id="print-statement-2"></a>
## PRINT# Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `OPEN`, `WRITE#`, `INPUT#`

### Purpose

Writes formatted output to a sequential file.

### Syntax

```basic
PRINT #file number, [expression] [;|, ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `expression` | No | Values to write. |

### Comments

The file must be open for output or append. `PRINT#` does not add quotes around strings; use `WRITE#` for comma-separated quoted records. A trailing semicolon suppresses the newline.

### Examples

#### Example 1 — Write two lines

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, "ONE"
30 PRINT #1, "TWO"
40 CLOSE #1
50 END
```

<a id="print-using-statement-2"></a>
## PRINT# USING Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRINT USING`, `PRINT#`

### Purpose

Writes formatted output to a sequential file.

### Syntax

```basic
PRINT #file number, USING format$; expression [, expression ...]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes | Open file number. |
| `format$` | Yes | Format string using `PRINT USING` rules. |
| `expression` | Yes | Values to format. |

### Comments

The file must be open for output. Formatting rules match `PRINT USING`; output goes to the file rather than the screen.

### Examples

#### Example 1 — Formatted file output

```basic
10 OPEN "TEST.DAT" FOR OUTPUT AS #1
20 PRINT #1, USING "###.##"; 12.345
30 CLOSE #1
40 END
```

<a id="pset-statement"></a>
## PSET Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `PRESET`, `POINT`, `LINE`

### Purpose

Draws a point in a graphics colour.

### Syntax

```basic
PSET [STEP] (x, y) [, colour]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `x`, `y` | Yes | Point coordinates. `STEP` makes them relative. |
| `colour` | No | Attribute. Default is the current foreground. |

### Comments

`PSET` requires a graphics mode; points are clipped to the canvas. The last graphics point becomes `(x,y)` for later `STEP` operations.

### Examples

#### Example 1 — Plot a point

```basic
10 SCREEN 1
20 PSET (160,100),2
30 PRINT POINT(160,100)
40 END
```

<a id="push-view-statement"></a>
## PUSH VIEW Statement

**Type:** Statement  
**Status:** Supported (newbasic extension)  
**See also:** `POP VIEW`, `VIEW`

### Purpose

Saves the current graphics viewport so it can be restored with `POP VIEW`.

### Syntax

```basic
PUSH VIEW
```

### Comments

The saved state includes the viewport rectangle, fill colour, and clipping state. The border is not saved. `PUSH VIEW` may be nested; each `POP VIEW` restores the most recent save. `SCREEN` and `CLEAR` clear the stack.

### Examples

#### Example 1 — Save and restore

```basic
10 SCREEN 1
20 VIEW (10,10)-(100,100)
30 PUSH VIEW
40 VIEW (20,20)-(80,80)
50 POP VIEW
60 END
```

<a id="put-statement"></a>
## PUT Statement

**Type:** Statement  
**Status:** Supported  
**See also:** `GET`, `FIELD`, `OPEN`, `PSET`

### Purpose

Writes a random-access file record, or places a graphics image on the screen.

### Syntax

```basic
PUT #file number [, record number]
PUT [STEP](x, y), array [, action]
```

### Arguments

| Argument | Required | Description |
|---|---|---|
| `file number` | Yes (file form) | Open random-access file. |
| `record number` | No | Record to write. Default is the next record. |
| `x`, `y` | Yes (graphics form) | Destination for the image. |
| `array` | Yes (graphics form) | Image array produced by `GET`. |
| `action` | No | `PSET`, `PRESET`, `AND`, `OR`, or `XOR`. Default `PSET`. |

### Comments

The file form writes the current `FIELD` buffer. The graphics form restores an image; the array layout is width, height, then row-major palette indices. `STEP` makes the destination relative.

### Examples

#### Example 1 — Write a record

```basic
10 OPEN "DATA.DAT" FOR RANDOM AS #1 LEN=20
20 FIELD #1, 20 AS A$
30 LSET A$="HELLO"
40 PUT #1, 1
50 CLOSE #1
60 END
```

#### Example 2 — Restore a graphics image

```basic
10 SCREEN 1
20 CIRCLE (160,100),30,2
30 DIM G(6000)
40 GET (120,70)-(200,130), G
50 CLS
60 PUT (20,20), G
70 END
```
