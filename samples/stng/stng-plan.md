# Star Trek NG — Working Plan and Handoff

**Status:** Phase 1 playable; ongoing UI and gameplay refinement  
**Program:** [`startrekng.bas`](startrekng.bas)  
**Dialect:** newbasic, `SCREEN 720`  
**Style:** line-numbered BASIC, mouse-first with keyboard fallback  
**Last known commit before this handoff:** `5c6b434`

This document is the working plan for another AI session taking over the Star Trek NG sample. Read it before editing. The interpreter and this sample are separate concerns: prefer changing only `samples/stng/startrekng.bas` unless a missing dialect feature is genuinely required.

---

## 1. What this program is

`startrekng.bas` is a modern, graphical reworking of the classic text-mode Star Trek game. It keeps the core turn-based mechanics:

- 8×8 galaxy of quadrants
- 8×8 sectors inside each quadrant
- Klingons, starbases, stars, and the Enterprise
- Energy, shields, hull, torpedoes, stardate/time
- Navigation, phasers, photon torpedoes, shields, damage, and computer status
- Win by destroying all Klingons; lose by hull loss or time expiry

The UI is a tactical console:

- Upper-left: short-range scanner
- Centre: galaxy map, local contacts, mission log
- Right: navigation/status page
- Right context panel: compass, damage gauges, phaser/shield/torpedo controls
- Bottom: clickable command palette

The faithful text-mode port remains at [`../startrek.bas`](../startrek.bas).

---

## 2. Read these first

- [RefManual index](../../RefManual/README.md) — the language reference for every BASIC statement used here
- [newbasic root plan](../../plan.md) — project-wide direction and milestones
- [newbasic specification plan](../../docs/newbasic-spec-plan.md) — dialect/spec planning
- [Windows build instructions](../../buildonwindows.md)
- [Faithful Star Trek text port](../startrek.bas)

Useful RefManual areas for this sample:

- `SCREEN`, `LINE`, `CIRCLE`, `PAINT`, `PSET` — graphics and scanner
- `MOUSEX`, `MOUSEY`, `MOUSEB` — pointer and click handling
- `INKEY$` — keyboard fallback
- `LOCATE`, `PRINT`, `CLS` — text overlay and panels
- `PALETTE` — UI and graphical colours
- `STR$`, `INT`, `ABS`, `SQR`, `RND`, `SIN`, `COS` — calculations and formatting
- string arrays, `DIM`, `FOR`, `GOSUB`/`RETURN`

---

## 3. Run and build

From the repository root:

```powershell
out\src\newbasic-cli\Release\newbasic.exe samples\stng\startrekng.bas
```

If the `out` build is stale:

```powershell
cmake --build out --config Release --parallel
```

The program is windowed by default. For a deterministic headless capture, use a key sequence that exits:

```powershell
$exe = "out\src\newbasic-cli\Release\newbasic.exe"
$keys = "N`rQ"   # N opens navigation, Enter engages, Q quits
& $exe --keys $keys --gpu-shot out\stng-check.ppm samples\stng\startrekng.bas
```

`--keys` feeds typed input. A CR in the PowerShell string is a real carriage return, not the literal characters `` `r ``.

Useful sequences:

| Purpose | Keys |
|---|---|
| Open/close NAV | `NQ` |
| Move down | `N7` then Enter, then Enter again to engage, then `Q` |
| Damage page | `DQ` |
| Phasers | `PQ` |
| Torpedo | `TQ` |
| Shields | `EQ` |
| Help | `?Q` |
| Combat smoke test | `N3++++++++` Enter three times, then `P` Enter, then `Q` |

---

## 4. Non-negotiable working style

1. **Line-numbered BASIC**
   - Every executable line has a unique numeric label.
   - BASIC sorts by line number, so source order is not execution order.
   - Before committing, run a duplicate-line-number check.
   - Keep related blocks numbered in ranges; the current map is in section 6.

2. **Descriptive names**
   - Use camelCase, roughly 6–12 characters where practical.
   - Examples already in use: `galaxyMap`, `klingonRows`, `sectorRow`, `shipEnergy`, `hullIntegrity`, `consoleMode$`, `statusMessage$`, `buttonWidth`, `compassCenterX`.
   - Keep short conventional loop indices (`I`, `J`) when they are clearly local.
   - If doing bulk renaming, use a quote-aware transform. Do not rename inside UI strings or `REM` comments.

3. **Mouse-first, keyboard fallback**
   - Every command should be reachable by mouse.
   - Keep keyboard shortcuts working for scripted testing.
   - Avoid blocking `INPUT` prompts in the graphical console; use the click/context state machine.

4. **Turn-based gameplay**
   - A successful NAV, phaser, torpedo, or shield transfer consumes one turn.
   - Non-action pages (help, damage view, computer status) do not consume time.
   - Keep the original command semantics where possible.

5. **Graphics limitations**
   - In graphics modes, `COLOR` controls graphics background/palette selection, not text foreground. Do not use `COLOR` expecting white text.
   - Text is rendered in the current text attribute; currently that means white HUD text.
   - Use `PALETTE` for UI colours and draw coloured bars/icons with `LINE`, `CIRCLE`, and `PAINT`.

---

## 5. Screen and panel geometry

The screen is **1280×720** logical pixels with a **160×45** text grid. All mouse hitboxes use these logical coordinates.

| Region | Bounds |
|---|---|
| Header | `x=16..1264`, `y=16..56` |
| Short-range scanner panel | `x=16..496`, `y=64..544` |
| Scanner grid | `x=56..456`, `y=104..504`, 8×8 cells at 50 px |
| Main viewer | `x=512..912`, `y=64..632` |
| Right status panel | `x=928..1264`, `y=64..632` |
| Context panel | `x=930..1262`, `y=300..630` |
| Command bar | `x=16..1264`, `y=648..712` |
| Command buttons | `x=24+I*152`, `y=656`, `144×40` |
| Compass centre | `(1096,440)` |
| Compass outer ring | radius 78 |
| Compass hub | radius 16 |
| Cardinal arrow tip | radius 72 |
| Intercardinal arrow tip | radius 54 |
| Target rings | outer radius 10, inner radius 9 |
| Warp track | `x=944..1224`, `y=550` |
| Engage button | `x=944..1224`, `y=590..624` |

### Compass rendering rules

- Draw the hub circle first.
- Draw the outer ring and tick marks.
- Build each arrow as a triangle whose base is on the hub edge and whose tip is the compass point.
- Arrow triangles must not overlap in the centre.
- `PAINT` each triangle with its arm colour.
- Repaint the hub after the arms to clean the centre.
- `CIRCLE` uses an explicit aspect of `1` so the ring is round on 1280×720. The default mode aspect makes it oval.
- Major arrows use lighter blue (attribute 4); intercardinal arrows use darker blue (attribute 5).
- The selected arrow is green (attribute 1).
- There are no direction letters.
- Each tip has two pure-white circles (radii 10 and 9) plus a small crosshair.
- Click radius at a tip is 18 px.

If you change the compass, keep the click targets and rendered tips in sync. Shared variables: `compassCenterX`, `compassCenterY`, `compassRadius`, `centerRadius`, `compassX()`, `compassY()`.

---

## 6. Current source map

Line numbers are approximate anchors, not strict end boundaries.

| Lines | Purpose |
|---|---|
| `10–270` | Header, dimensions, course/compass tables, palette, startup |
| `300–510` | Main input/draw loop |
| `1000–1210` | New game: galaxy generation, start quadrant, mission state |
| `2000–2196` | Short-range scanner and object icons |
| `2200–2554` | Main galaxy map, quadrant info, local contacts, mission log |
| `2560–2598` | Damage-control gauge page |
| `2600–2880` | Context pages: command, NAV compass, phasers, torpedo, shields, help |
| `2890–2960` | Bottom command buttons |
| `5000–5170` | Full-screen redraw orchestration |
| `3000–3930` | Mouse click dispatch and hitboxes |
| `4000–4305` | Keyboard dispatch |
| `6000` | NAV action |
| `6900` | Phasers |
| `6200` | Torpedo |
| `6400` | Shield transfer |
| `6500` | Klingon attack |
| `6700` | Starbase resupply |
| `6800` | Mission/win/loss check |
| `7000` | Horizontal value-bar helper |
| `7200` | Button drawing helper |
| `8000` | Help overlay |
| `8500` | Game-over overlay |

Note: section 5000 appears before 3000 in the source file. That is harmless because BASIC sorts by line number, but do not duplicate labels.

---

## 7. Game state and data model

### Galaxy

`galaxyMap(row,col)` is an encoded count:

```text
Klingons × 100 + starbases × 10 + stars
```

- Quadrants are 1–8 in both axes.
- `baseRows(row,col)` / `baseCols(row,col)` store starbase sector positions.
- `klingonRows/Cols/Hull` are flattened arrays with three slots per quadrant:
  - `quadrantIndex = (quadRow-1)*8 + quadCol`
  - `klingonIndex = (quadrantIndex-1)*3 + I`
- `starRows/Cols` use the same three-slot indexing.
- `systemHealth(1..8)` drives the damage gauges.
- `systemName$(1..8)` labels warp, impulse, phasers, torpedoes, shields, sensors, computer, life support.

### Mission state

- Start quadrant/sector: `4,4` / `4,4`
- `shipEnergy = 3000`
- `shieldPower = 0`
- `hullIntegrity = 100`
- `torpedoCount = 10`
- `timeLeft = 30`
- `stardate = 3400`
- `phaserPower = 500`
- `transferAmount = 100`
- `warpFactor = 1`
- Start quadrant is cleared of Klingons; a fallback Klingon is placed if the galaxy otherwise has none.

### Course convention

Course 1 is east, then clockwise:

```text
1 E, 2 NE, 3 N, 4 NW, 5 W, 6 SW, 7 S, 8 SE
```

`courseRows()` and `courseCols()` hold the row/column deltas. Increasing sector row moves downward on the scanner.

---

## 8. Interaction model

### Mouse

- Clicking the bottom command bar selects a command mode.
- `NAV`: click a compass tip to set direction, click the linear warp scale to set warp, click ENGAGE to move.
- `PHASERS`: click MINUS/PLUS to set power, click FIRE PHASERS.
- `TORPEDO`: click a Klingon on the scanner to lock, click FIRE TORPEDO.
- `SHIELDS`: set transfer amount, choose TO SHIELDS/TO ENERGY, click APPLY.
- `DAMAGE`: shows eight signed gauges.
- `COMPUTER`: currently reports the galactic record in the status area.
- `HELP`: modal help overlay; click or key closes.
- Game over: RESTART and QUIT buttons.

`previousMouse` is used to detect the button-down edge. Do not trigger an action continuously while the button is held.

### Keyboard shortcuts

| Key | Meaning |
|---|---|
| `N` | NAV |
| `S` | scan |
| `P` | phasers |
| `T` | torpedo |
| `E` | shields |
| `D` | damage |
| `C` | computer |
| `?` / `H` | help |
| `Q` | quit |
| `1–8` | course in NAV |
| `+` / `-` | coarse warp ±0.5, phaser power, or shield transfer |
| `Enter` | engage / fire / apply |
| `R` | restart on game-over screen |

---

## 9. NAV and warp semantics

The warp UI is a continuous 0–10 scale, sampled in 0.1 increments:

```basic
warpFactor = (pointerX-944)/280*10
warpFactor = INT(warpFactor*10+0.5)/10
```

The visible track is `x=944..1224`, `y=550`; the click band is `y=542..558`.

On ENGAGE:

```basic
moveSteps = INT(warpFactor*.8+.5)
energyCost = INT(warpFactor*50+.5)
```

- Warp 10 maps to eight sector moves.
- Values below roughly 0.7 produce no movement and message `WARP TOO LOW TO MOVE`.
- `moveSteps < 1` returns before spending energy.
- Movement crosses quadrant boundaries; the galactic perimeter is blocked.
- A successful move decrements `timeLeft`, updates heading, clears target lock, checks starbase resupply, resolves Klingon fire, then checks mission end.

The fractional warp scale and integer sector grid are intentionally decoupled. If this feels wrong, document the change and adjust both movement and tests.

---

## 10. Combat and mission rules

### Phasers

- Power is allocated from ship energy.
- Each Klingon in the current quadrant takes proportional damage based on range.
- Killing a Klingon clears its hull, decrements `klingonsLeft`, removes `100` from `galaxyMap`, and clears target lock if needed.
- Firing consumes a turn and triggers Klingon counterfire.

### Torpedoes

- Torpedo travels up to eight sectors along `currentHeading`.
- It stops at the first Klingon hit.
- Torpedo count decrements even on a miss.
- Firing consumes a turn and triggers counterfire.

### Shields

- Transfer from energy to shields or shields to energy.
- Transfer amount is 100–1000, in 100-unit steps.
- Applying consumes a turn and triggers counterfire.

### Klingon attack

- Each living Klingon in the current quadrant adds to the attack strength.
- Shields absorb first; remaining damage reduces hull.
- A random subsystem loses health for the damage gauges.
- Currently system damage is **cosmetic**; it does not yet disable or degrade commands.

### Starbase resupply

Docking in the same sector as a starbase restores energy, torpedoes, shields, and hull.

### End conditions

- All Klingons destroyed: victory.
- `hullIntegrity <= 0`: defeat.
- `timeLeft < 1`: defeat.

---

## 11. Known limitations and likely next work

### P0 — handoff stability

- [ ] Manually verify real mouse clicks for every command button, compass target, warp scale, and game-over button.
- [ ] Add an automated or scripted way to test mouse hitboxes, if practical.
- [ ] Check all UI text fits at 1280×720 and remains readable.
- [ ] Confirm no duplicated line numbers after every edit.
- [ ] Keep the plan and file path references current after moves.

### P1 — UI polish

- [ ] Hover highlighting for command buttons and compass targets.
- [ ] Tooltips or a context hint line.
- [ ] Scanner sweep/ping animation when scanning.
- [ ] Red-alert visual/sound feedback.
- [ ] Optional `BEEP`/`SOUND` effects for hits, launches, and alerts.
- [ ] Better mission-log formatting and scrolling.
- [ ] Responsive or scaled layout instead of hardcoded coordinates.

### P2 — gameplay depth

- [ ] Make subsystem damage affect performance:
  - warp damage limits speed
  - phaser damage reduces output
  - torpedo damage can disable tubes
  - sensor damage hides contacts
  - computer damage disables computer pages
- [ ] Damage-control repair allocation.
- [ ] Full computer pager: galaxy record, status, torpedo data, distance calculator, region map.
- [ ] Better target-lock UI and torpedo path preview.
- [ ] Starbase repair vs resupply distinction.
- [ ] Klingon movement and counterfire variety.
- [ ] Difficulty levels / initial Klingon count.
- [ ] Win/loss rankings and efficiency score.

### P3 — presentation

- [ ] Animated starfield or scanner sweep.
- [ ] Transition/alert animations.
- [ ] Additional palettes/theme support.
- [ ] Dedicated title/start screen.

---

## 12. Testing checklist

Minimum before committing:

1. Program launches and draws the dashboard without runtime error.
2. `N` opens NAV; compass selection changes course.
3. Clicking/targeting the warp scale updates the marker and `WARP` readout.
4. `ENGAGE` moves exactly once and consumes energy/time.
5. Warp 0 produces no movement.
6. Phasers fire, reduce Klingon hull, and trigger counterfire.
7. Torpedo count decrements and misses/hits are reported.
8. Shield transfer changes energy and shields.
9. Damage gauges display and change after a hit.
10. Help opens and closes.
11. Time expiry and hull loss produce the game-over overlay.
12. `R` restarts; `Q` quits.
13. No duplicate line numbers.

If the interpreter itself changes, run the relevant Catch2 suite and CTest as described in [`buildonwindows.md`](../../buildonwindows.md).

---

## 13. Git and commit policy

- Commit locally in coherent chunks.
- **Do not push automatically.** Push only when:
  - 15 local commits have accumulated since the last push, or
  - the user explicitly asks.
- Check `git rev-list --count origin/main..main` to see how many commits are ahead.
- Never force-push.
- Do not commit `out/`, screenshots, generated PPM/PNG files, printer output, or unrelated RefManual work.
- If another agent has committed unrelated files, leave them alone; stage only this sample’s files.

Current known last push before this handoff: `5c6b434`.

---

## 14. Open design questions for the user

1. Should subsystem damage have real gameplay effects in the next phase?
2. Should the warp scale map directly to sector steps, or is the current 0–10 → 0–8 mapping correct?
3. Should the computer become a full-screen pager or stay a right-panel status page?
4. Do we want sound, scan animation, or both?
5. Should the command palette gain hover states/tooltips?
6. Should the scanner, galaxy, and compass remain fixed at 1280×720, or scale with the window?
7. Should mouse clicks or keyboard remain the primary control path?

---

## 15. Handoff rules for the next AI

- Read this plan and [`startrekng.bas`](startrekng.bas) before changing anything.
- Preserve the no-text compass and tip-target interaction.
- Preserve the green selected direction.
- Keep the aspect-1 circles; do not reintroduce the oval default.
- Keep UI strings intact when renaming variables.
- Do not add external dependencies; this is a pure BASIC sample.
- Prefer small local commits with clear messages.
- If you need a new interpreter feature, update the RefManual and tests in the same change, and say so explicitly.
- When in doubt, ask the user before large architectural changes.

---

## 16. Current acceptance target

The current phase is complete when:

- The console launches cleanly.
- All eight command buttons work.
- NAV supports direction → warp → engage.
- Phasers, torpedoes, shields, damage, computer, and help are reachable.
- The short-range scanner reflects current quadrant contents.
- The right status page reflects energy, shields, hull, torpedoes, Klingons, starbases, time, and position.
- A mission can be won or lost and restarted.
- Mouse and keyboard both operate the console.
