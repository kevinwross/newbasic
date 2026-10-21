# M12 design note: graphics

**Status: proposal, for review.** No code is written. The decision needed is at the
end.

## Why a note first

Graphics changes the one abstraction everything else sits on. `Screen` today owns an
80×25 character grid with per-cell attributes; every backend (console, D3D11,
headless) implements that. GW-BASIC's graphics modes *replace* the display with a
pixel buffer and overlay a text grid on top, which is a different shape of model. The
D3D11 backend can render either, but the console backend can only render text — so the
degradation story needs deciding before the code, exactly as it did for M7.

## Two possible scopes

| | **A. Refuse graphics** | **B. Implement graphics** |
|---|---|---|
| Work | A clear refusal for the graphics statements (already the current behaviour: they report Advanced Feature) | A framebuffer, drawing primitives, `DRAW`, viewports, and D3D11 presentation |
| Est. | hours | ~4–6 rounds, phased below |
| Cost of being wrong | Programs using graphics stay unusable | `Screen` becomes mode-aware; every backend gains a branch |

Recommendation: **B, phased**, because graphics is the last large GW-BASIC area and
the headless-testable part is larger than it looks (see testing).

## Model

```
Canvas            indexed-colour framebuffer: width, height, 4-bit palette indices
  SCREEN 1        320×200, 4 colours (palette 0), 16:10 pixels
  SCREEN 2        640×200, 2 colours
  SCREEN 9        640×350, 16 colours
Screen
  text grid        always present (SCREEN 0 is text-only; 1/2/9 overlay text)
  Canvas*          null in text mode
  mode             Text, Cga1, Cga2, Ega9
```

`Screen::Mode()` and `Screen::CanvasImage()` are the two new seams. The console
backend renders text only; in a graphics mode it prints a one-line notice and falls
back to a cleared text screen (documented degradation, consistent with M7's tiers).
The headless backend keeps the canvas so tests can assert pixels. The D3D11 backend
uploads the canvas as an `R8` texture and applies the 16-colour palette in the shader
— the same pipeline the glyph atlas already uses.

## Statements and what each needs

| Statement | Notes |
|---|---|
| `SCREEN mode[,[colorswitch][,[apage][,[vpage]]]]` | Set the mode; page flipping in `apage`/`vpage` is likely out of scope (see risks) |
| `PSET (x,y)[,c]`, `PRESET (x,y)[,c]` | Plot; predicates are trivial and fully testable |
| `LINE (x1,y1)-(x2,y2)[,c[,B\|BF]]` | Bresenham; `B`/`BF` box outlines |
| `CIRCLE (x,y),r[,c[,start,end[,aspect]]]` | Midpoint circle with arcs and aspect; the fiddliest geometry |
| `PAINT (x,y)[,fill[,border]]` | Flood fill, optionally bounded by a colour |
| `DRAW "macro"` | A macro language (`U/D/L/R/E/F/G/H`, `M`, `B`, `N`, colour/scale prefixes); a parser plus the same line routine |
| `POINT (x,y)`, `PMAP`, `VIEW`, `WINDOW`, `PALETTE`, `PCOPY` | Coordinates, clipping and palette |
| `GET`/`PUT` (graphics) | Bit-block transfer, which collides with the *record* `GET`/`PUT` from M9 — the parser already distinguishes them by the `#n` form, so this is the trickiest grammar change |
| `CLS n` | `n=1` clears the graphics viewport, which is why `CLS 1` is a no-op today |

## Testing strategy (what is verifiable here)

| Part | How |
|---|---|
| Canvas, palette, plotting, clipping | Headless unit tests asserting pixels — the bulk of the work |
| `LINE`/`CIRCLE`/`PAINT`/`DRAW` algorithms | Pure functions over the canvas; property-style checks (symmetry, endpoints, fill containment) |
| Text overlay in graphics modes | Row/pixel mapping tests |
| D3D11 presentation | **Not automatable here** — the same by-eye limit as M7's window |

That split is why this is worth doing even though the display cannot be verified here:
most of it *can* be.

## Risks

- **`CIRCLE` and `DRAW` are the fiddly ones.** GW-BASIC's circle aspect ratio
  (`5/6` in `SCREEN 1`, `5/12` in `SCREEN 2`) and `DRAW`'s macro grammar are easy to
  get subtly wrong. The reference documents them, but the examples are OCR-damaged in
  places — same trap as `PRINT USING`.
- **`GET`/`PUT` grammar overlap** with the record statements. Needs care in the
  parser and tests for both forms.
- **Page flipping** (`apage`/`vpage`) probably deserves to be out of scope; it exists
  for animation and adds double-buffer semantics to every backend.
- **Palette fidelity**: `PALETTE` can change colours at run time, which the D3D11
  shader must read rather than bake in.

## Phasing, if approved

1. **Canvas + palette + `PSET`/`PRESET`/`POINT`/`LINE` + D3D11 presentation.** Visible
   progress, fully testable, and it settles the mode plumbing.
2. **`CIRCLE`, `PAINT`, `DRAW`.** The geometry and the macro language.
3. **`VIEW`/`WINDOW`/`PMAP`/`PALETTE`/`PCOPY`, then graphics `GET`/`PUT`.**
4. **Text overlay in graphics modes** and a `docs/m12-*` note recording each
   interpretation, as M7 and M8 did.

## The decision

**Is graphics in scope?**

- **In** → I start with phase 1 (canvas, palette, plotting, `LINE`, D3D11 presentation)
  and report back with tests and a screenshot request.
- **Out** → the graphics statements keep reporting Advanced Feature, and I record that
  as the M12 decision in `plan.md` rather than leaving it open indefinitely.
