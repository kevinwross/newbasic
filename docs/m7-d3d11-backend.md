# M7 design note: Win32 + D3D11 screen backend

Scope: a third `Screen` backend that renders the existing **80×25 text grid** in
an owned window through D3D11. This is a *display* backend, not GW-BASIC
graphics statements — `PSET`/`CIRCLE`/etc. remain "parse but not implemented"
per the v1 coverage decision.

## Why a note first

The plan's risk list already names the danger: *"D3D initialization failures
(M7) — headless/CI machines may lack GPU drivers; the D3D11 backend must
degrade to a software rasterizer."* A backend that can fail to initialise must
have its fallback contract written down before the code, or the failure path
becomes the bug.

## Shape

```
Screen (existing interface: 80×25 cells, attributes, cursor, keys)
  ├── ConsoleScreen   (Win32 console API)      — current default
  ├── HeadlessScreen  (tests)                  — no output
  └── D3D11Screen     (new)
        ├── Window          Win32 window + message pump
        ├── GlyphAtlas      CP437 → 8×16 bitmap font → texture
        └── Renderer        D3D11 device/swap chain, one quad per cell
```

`Screen` already abstracts everything the VM needs, so the VM, REPL and DAP are
untouched by this work. Selection happens in the CLI only.

## Degradation contract (three tiers)

1. **D3D11 hardware** (`D3D_DRIVER_TYPE_HARDWARE`) — the normal path.
2. **D3D11 WARP** (`D3D_DRIVER_TYPE_WARP`, Microsoft's software rasterizer) —
   still D3D11, no GPU required. This satisfies the plan's "degrade to a
   software rasterizer" requirement without a second renderer.
3. **Win32 console** — if both D3D attempts fail (no DXGI, no device, headless
   session), construction returns null and the CLI silently uses
   `ConsoleScreen` and prints one line saying why.

Construction never throws and never aborts: `D3D11Screen::Create()` returns a
`std::unique_ptr<Screen>` (null on failure). CI must never construct it at all.

## Glyph atlas

CP437 glyphs are rasterised with **GDI** (`CreateFont`/`TextOutW` into a DIB
section) at 8×16, then uploaded as a single-channel `R8` texture. Rationale: GDI
needs no embedded font-ROM data (no licensing question, and no 4 KB blob to
maintain), and 8×16 is exactly the console cell the text mode already assumes.
`MultiByteToWideChar(437, …)` maps each byte so GDI draws the dialect's glyph
rather than Latin-1.

**Finding (already implemented):** system fonts routinely lack the CP437
*block and shade* glyphs — the reference font returned a 15-pixel stub for the
solid block (0xDB), while `A` drew 45 pixels. GW-BASIC programs use those
characters for crude bar graphics via `CHR$()`, so they are **generated
procedurally** after rasterisation: 0xB0/0xB1/0xB2 as 25/50/75% dither patterns,
0xDB solid, and 0xDC-0xDF as half blocks. Tests pin that the solid block is the
densest glyph and that the shades are lighter than it.

## Rendering

- One dynamic vertex buffer of 80×25 quads (4 vertices each), rebuilt per frame
  (6400 vertices; trivial for any GPU).
- One shader pair: textured quad × per-vertex colour (the cell attribute →
  16-colour CGA palette, already modelled by `Screen`).
- Cursor: drawn as an inverted quad on the cursor cell.
- `Present(1)` vsync; the existing `Screen::PresentIfDirty()` contract drives
  the frame.

## Keyboard

`WM_KEYDOWN`/`WM_CHAR` feed the same key queue the console backend fills, so
`INKEY$` and `INPUT` behave identically. Arrow keys map to the dialect's
extended codes (already modelled in the console backend).

## Testing strategy (what is and is not verifiable here)

| Part | How it is tested |
|---|---|
| Glyph atlas generation | **Headless unit tests**: atlas dimensions, every glyph rasterised, distinct glyphs differ (`A` vs `B`), blank glyphs blank, CP437 high range covered |
| Quad/NDC geometry | **Headless unit tests**: cell rect → normalised device coordinates, row 0 at the top, 80×25 coverage without gaps |
| Attribute → colour | **Headless unit tests**: the existing CGA palette mapping |
| Window + device + Present | **Not automatable here** (no GPU, and the CLI requires a console). A hidden `[.gpu]` manual test constructs the backend and draws one frame; it is skipped in CI exactly like the `[.audio]` sound checks |

This is stated plainly so nobody later assumes the visual path was verified: the
GPU path will be confirmed by eye on a machine with a display adapter, and the
code must fail softly when that is not available.

## Risks to handle in the code

- **Device loss** (`DXGI_ERROR_DEVICE_REMOVED`): recreate the device; if that
  fails, fall back to the console backend mid-run.
- **DPI scaling**: query the monitor DPI and scale the quad size, or accept a
  slightly blurry nearest-neighbour scale. Prefer integer scaling when possible.
- **Resize**: `WM_SIZE` → swap chain resize + render target recreation.
- **No message pump starvation**: the window pump runs inside `PresentIfDirty`
  (non-blocking `PeekMessage`), so `INKEY$` stays responsive without threads.

## CLI surface

`lwbasic --gpu prog.bas` (and the same flag for the REPL) requests the windowed
backend; without the flag the console backend is used, so existing behaviour is
unchanged. The flag is best-effort: failure to initialise falls back with a
one-line notice rather than an error.

## Implementation status (round 3)

**Implemented:** window class + `CreateWindowExW`; D3D11 device via
`D3D11CreateDeviceAndSwapChain` trying **hardware then WARP**; render target;
inline HLSL (vertex pass-through, pixel = atlas ink × vertex colour); 128×256
`R8` atlas upload with a point sampler; per cell **two quads** — a background
sampled from the solid-block glyph (fully inked, so the same shader paints the
attribute's background colour) and the glyph itself; alpha blending; `Present(1)`
with vsync; keyboard through `WM_CHAR`/`WM_KEYDOWN` (arrows to the dialect's
extended codes); `--gpu` with the three-tier fallback.

**Verified here:** it compiles clean under `/W4 /WX`; `lwbasic --gpu prog.bas` ran
a program to completion (exit 0, no fallback message); and the hidden
`[.gpu]` test constructs the backend, writes text and presents one frame with
its state checks passing. So construction, the device, shaders, atlas upload,
quad submission and `Present` all work in this session.

**Not verified — needs your eyes:** whether the picture is *correct* (glyph
shapes, colours, sizes, cursor, no tearing). Nothing here can check pixels.

**Implemented since the first pass:** `WM_SIZE` → deferred `ResizeBuffers` with
render-target rebuild and a viewport that follows the client area; DPI-scaled
window sizing (`GetDeviceCaps(LOGPIXELSX)`); the atlas rasterised **at the scaled
cell size** so glyphs are crisp rather than magnified from 8×16; a block cursor
drawn as inverted video in the cursor cell; one-shot device recreation on
`DXGI_ERROR_DEVICE_REMOVED` (releasing the whole chain except the window and
rebuilding it, ending the run cleanly if that fails); `--gpu` documented in
`--help`; and the hidden `[.gpu]` manual test.

**Still open:** only human verification of the picture. The known cosmetic
limits are that the glyphs come from a substituted system font (so shapes are
not the original GW-BASIC ROM font) and that CP437 block/shade glyphs are
generated procedurally rather than fonted.

## Checking the window rather than the dump

`--gpu-shot` writes the screen from the grid model, so it is possible for it to agree with a
correct model while the window shows something else. `--gpu-pixel X,Y` reads one pixel back from
the window's own back buffer after the run, at canvas coordinates, and prints it:

    lwbasic --gpu-pixel 204,120 prog.bas
    lwbasic: window pixel 204,120 = 170,170,170

That is how the invisible cursor was found: the block read 170,170,170 in the window and black
everywhere else, and the dump had shown a cursor all along. Use it when a claim is about what the
display does rather than what the model holds.
