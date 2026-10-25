# stng — session handoff

**Session:** `stng` (Star Trek NG sample)
**Written:** 2026-10-25, ~10:40 America/Phoenix
**Machine:** Windows · repo `C:\Users\kevin\repos\newbasic` · branch `main`
**HEAD at handoff:** see §7 of this file
**Read next:** this file, then [`samples/stng/stng-plan.md`](samples/stng/stng-plan.md) (the authoritative working plan, 470 lines).

> **TL;DR** — Nothing under `samples/stng/` changed this session; the sample was verified, not modified.
> The sample is confirmed playable with the current build (clean launch, NAV engage works, all eight
> command buttons draw). One real finding: the run command in `stng-plan.md` §3 points at an executable
> path that no longer exists — the live binary is `bin\Release\newbasic.exe`.
> Next session: pick a P0/P1 item from plan §11, or get answers to the design questions in plan §14.

---

## 1. What this session did

- Read `samples/stng/stng-plan.md` in full (all 16 sections).
- Inspected the repo state, the sample source, and every build tree to establish where the
  executable actually lives.
- Ran the sample headless and captured a frame; confirmed it starts, draws the full console,
  and exits cleanly (details in §3).
- Wrote this handoff and [`bigpush.md`](bigpush.md), then made one local commit of both.
- **Did not** edit `startrekng.bas`, the plan, the interpreter, or the RefManual.
- **Did not** push (per plan §13, another session owns the push).

## 2. Sample state as verified this session

| Fact | Value |
|---|---|
| Source | [`samples/stng/startrekng.bas`](samples/stng/startrekng.bas) |
| Size | 601 lines; 566 line-numbered statements |
| Duplicate line numbers | **none** (checked with a `^\s*(\d+)\s` scan) |
| Last commit touching the sample | `ea86a1e` (2026-10-23) "Reorganize Star Trek NG and add a handoff plan" |
| Working-tree changes in the sample | none |
| Faithful text port (untouched) | [`samples/startrek.bas`](samples/startrek.bas) |

## 3. Build and run — corrected

**The plan's command is stale.** `stng-plan.md` §3 says to run
`out\src\newbasic-cli\Release\newbasic.exe`; that file **does not exist** (the `out` tree's cache
predates the output-layout change). `CMakeLists.txt:85` sets

```cmake
RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release>"
```

so the canonical binary is **`bin\Release\newbasic.exe`**.

Verified working this session (exit code 0, frame written):

```powershell
$exe = "bin\Release\newbasic.exe"          # built 2026-10-25 05:55
& $exe --keys "N`rQ" --gpu-shot out\stng-smoke.ppm samples\stng\startrekng.bas
```

To rebuild, use the newest configured tree (`build-clean-20261025`, cache 05:54 today, is the one
that produced the current binary) — or any tree, they all write to `bin\Release` now:

```powershell
cmake --build build-clean-20261025 --config Release --parallel
```

Build trees present: `build-clean-20261025` (newest), `build-bin-output`, `build-clean`,
`build-vs2026`, `out` (stale for the CLI), `build-wsl*` (WSL). Only `build-clean` and
`build-bin-output` also keep per-tree `src/newbasic-cli/Release/newbasic.exe` copies from before
the layout change; treat those as historical.

### What the capture showed

With `N`, Enter, `Q`: NAV engaged, ship moved from sector `4,5` back through the engage path,
energy `3000 → 2950` (warp 1 costs 50), `TIME 29`, `ENGINES ENGAGED` recorded in the mission log.
Header, condition line, short-range scanner, galaxy map, local contacts, mission log, NAV status
block and all eight palette buttons (`NAV SCAN PHASERS TORPEDO SHIELDS DAMAGE COMPUTER HELP`)
render correctly at 1280×720 and are legible.

### Handy: PPM → PNG without image libraries

`--gpu-shot` writes binary P6 PPM, which `read_image` cannot open. `System.Drawing` converts it in
PowerShell (works in this sandbox in FullLanguage mode):

```powershell
Add-Type -AssemblyName System.Drawing
$b = [IO.File]::ReadAllBytes((Resolve-Path 'out\x.ppm')); $p = 0; $t = @()
while ($t.Count -lt 4) {
  while ($b[$p] -in 32,10,13,9) { $p++ }
  if ($b[$p] -eq 35) { while ($b[$p] -ne 10) { $p++ }; continue }
  $s = $p; while ($b[$p] -notin 32,10,13) { $p++ }; $t += [Text.Encoding]::ASCII.GetString($b[$s..($p-1)])
}
$p++; $w=[int]$t[1]; $h=[int]$t[2]; $bmp = New-Object Drawing.Bitmap($w,$h)
for ($y=0;$y -lt $h;$y++){for($x=0;$x -lt $w;$x++){$i=$p+($y*$w+$x)*3;$bmp.SetPixel($x,$y,[Drawing.Color]::FromArgb(255,$b[$i],$b[$i+1],$b[$i+2]))}}
$bmp.Save("$PWD\out\x.png", [Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
```

Ignore the header-parse loop if you only need the PNG — that loop is what makes P6 usable.

## 4. Where the context lives

| Doc | Why |
|---|---|
| [`samples/stng/stng-plan.md`](samples/stng/stng-plan.md) | **Primary handoff.** Geometry table, source map, data model, semantics, work queue, testing checklist, git policy, open questions |
| [`samples/stng/startrekng.bas`](samples/stng/startrekng.bas) | The program |
| [`samples/startrek.bas`](samples/startrek.bas) | Faithful text-mode original — source of command semantics |
| [`RefManual/README.md`](RefManual/README.md) | Language reference for every statement used (`SCREEN`, `LINE`, `CIRCLE`, `PAINT`, `MOUSEX/Y/B`, `INKEY$`, `PALETTE`, …) |
| [`plan.md`](plan.md) | Project-wide direction and milestones |
| [`docs/newbasic-spec-plan.md`](docs/newbasic-spec-plan.md) | Dialect/spec planning |
| [`buildonwindows.md`](buildonwindows.md) | Build + Catch2/CTest instructions |
| [`bigpush.md`](bigpush.md) | Local commits waiting for the push session |

## 5. Condensed technical recap

Only the load-bearing facts; `stng-plan.md` is authoritative and has the full detail.

**Constraints (§4, §15).** Line-numbered BASIC — unique numbers, source order is not execution
order, run a duplicate check before every commit. camelCase names 6–12 chars; never rename inside
UI strings or `REM`s. Mouse-first with keyboard fallback. One turn per successful NAV / phaser /
torpedo / shield action; help, damage and computer pages are free. In graphics modes `COLOR` sets
the graphics background, not text colour — use `PALETTE`. Preserve: the no-text compass, the green
selected direction, aspect-1 circles, and the shared compass variables.

**Geometry (§5).** 1280×720 logical, 160×45 text grid. Scanner `x=16..496, y=64..544` (grid
`56..456 / 104..504`, 50 px cells). Viewer `x=512..912`. Right panel `x=928..1264`.
Context panel `930..1262, 300..630`. Command bar `16..1264 / 648..712`, buttons at
`x=24+I*152, y=656`, size `144×40`. Compass centre `(1096,440)`, ring radius 78, hub 16,
cardinal tip 72, intercardinal tip 54, click radius 18. Warp track `944..1224` at `y=550`
(click band 542..558); ENGAGE `944..1224, 590..624`.

**Source map (§6).** `10–270` startup/tables/palette · `300–510` main loop · `1000–1210` new game ·
`2000–2196` scanner · `2200–2554` galaxy map + status · `2560–2598` damage gauges · `2600–2880`
context pages · `2890–2960` command buttons · `5000–5170` redraw orchestration · `3000–3930` mouse
dispatch · `4000–4305` keyboard dispatch · `6000` NAV · `6200` torpedo · `6400` shields · `6500`
Klingon attack · `6700` starbase · `6800` mission check · `6900` phasers · `7000` bar helper ·
`7200` button helper · `8000` help · `8500` game over. Note block `5000` sits physically before
`3000` — harmless, but do not duplicate numbers.

**Model (§7).** `galaxyMap = K×100 + bases×10 + stars`; quadrants 1–8; three slots per quadrant at
`klingonIndex = (quadrantIndex-1)*3 + I`. Course 1 = east, clockwise. Start `(4,4)/(4,4)`, energy
3000, hull 100, 10 torpedoes, 30 time, stardate 3400, phaser power 500, transfer 100, warp 1.

**NAV (§9).** Continuous 0–10 warp sampled at 0.1 (`INT(courseX/280*10*10+.5)/10`).
`moveSteps = INT(warp*.8+.5)` (warp 10 → 8 sectors), `energyCost = INT(warp*50+.5)`.
Below ~0.7 gives no movement and `WARP TOO LOW TO MOVE`; `moveSteps < 1` returns before spending
energy. A successful move decrements time, updates heading, clears target lock, tries starbase
resupply, resolves Klingon fire, then checks the mission end.

**Combat (§10).** Phasers scale damage by range and decrement `klingonsLeft` / `galaxyMap` on a
kill. Torpedoes fly up to 8 sectors along `currentHeading` and stop at the first hit; the count
decrements even on a miss. Shields transfer 100–1000 in 100-unit steps. Klingon fire hits shields
first, then hull, and dents a random subsystem for the gauges — **currently cosmetic**.

## 6. Repo and git state carried forward

- Working tree was clean except **one untracked file**: `baslib/merged.bas` (41,748 bytes, written
  today 10:03 by the **baslib** session). It is a generated merge artifact (baslib + host), not
  gitignored, and **not mine**. It was deliberately left uncommitted — see [`bigpush.md`](bigpush.md) §3.
  Do not sweep it into an unrelated commit.
- One commit was already ahead of `origin/main` before this session (`cd35000`, RefManual DRAW
  examples). This session added a second. Push policy unchanged: **another session pushes**; push
  only at 15 accumulated commits or on explicit request; never force-push.
- `out/` is full of another session's capture artifacts (`ng-*.ppm`, `*.png`, …). `out/` is
  gitignored — leave it alone and never commit captures.

## 7. HEAD at handoff

The commit that added this file, titled *Add stng session handoff and bigpush push ledger*, with
parent `cd35000`. Run `git log -1 --oneline` for its exact hash — it is referenced by title rather
than by hash so that amending the commit does not invalidate the text.

## 8. Open decisions blocking design work (from plan §14)

None of these were answered this session; they gate the P1–P3 work:

1. Should subsystem damage have real gameplay effects next phase?
2. Should warp map directly to sector steps, or keep 0–10 → 0–8?
3. Should the computer become a full-screen pager or stay a right-panel page?
4. Sound, scan animation, or both?
5. Hover states / tooltips on the command palette?
6. Fixed 1280×720 layout, or scale with the window?
7. Mouse or keyboard as the primary control path?

## 9. Suggested next actions

1. **Fix the stale run command** in `stng-plan.md` §3 (one-line edit) — it will otherwise send the
   next session chasing a missing binary.
2. Work the P0 list from plan §11: verify real mouse clicks for all eight buttons, the compass tips,
   the warp scale and the game-over buttons; check text fit; keep the duplicate-number check.
3. P1 wins that need no user decision: hover highlighting, a context hint line, mission-log
   scrolling.
4. Anything under P2 needs answers to §14 questions 1, 2 and 3 first.
