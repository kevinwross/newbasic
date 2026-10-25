# bigpush — local commits waiting for GitHub

**Purpose.** Several sessions are working in this repo at once. One session owns `git push`;
everyone else commits locally and records the result here so the push session knows what to send.
**Rule: only the push session pushes. Never force-push.**

**Last updated:** 2026-10-25 10:43 America/Phoenix, by session **`general`**

---

## 1. Waiting to be pushed

Ordered oldest → newest. Count at last update: **4 commits ahead of `origin/main`**
(`git rev-list --count origin/main..main`).

| # | Commit | Session | Files | Subject |
|---|---|---|---|---|
| 1 | `cd35000` | (DRAW / RefManual session) | `RefManual/D.md` (+72) | Add DRAW examples for shapes, fills and patterns |
| 2 | `4d58222` | **`stng`** | `stng-handoff.md`, `bigpush.md` | Session handoff + push ledger |
| 3 | `48e1db9` | **`baslib`** | `baslib-handoff.md`, `bigpush.md` | Baslib session handoff + push ledger entry |
| 4 | this general commit, parent `48e1db9` | **`general`** | `general-handoff.md`, `bigpush.md` | General session handoff + push ledger entry |

`origin/main` was at `d66efed` when this file was written. Push the four local commits in order; no force-push.

## 2. Committed by session `stng`

Files in this session's single local commit:

| File | What it is |
|---|---|
| `stng-handoff.md` | Handoff for the Star Trek NG sample session: verified state, corrected build/run paths, condensed plan recap, open questions, next actions |
| `bigpush.md` | This ledger |

No changes were made to `samples/stng/startrekng.bas` or to the interpreter in this session.
Nothing here needs review before pushing — it is documentation only.

## 3. Uncommitted files deliberately left out

| File | State | Why it was not committed |
|---|---|---|
| `baslib/merged.bas` | untracked, 41,748 bytes, written 2026-10-25 10:03 | Generated artifact (baslib source merged with a host program), owned by the **baslib** session, which may still be editing it. It is not in `.gitignore`, but it is reproducible from `baslib/baslib.bas` + a host, so committing it here would add generated churn to the push. If it should travel, the baslib session should generate-and-commit it, or add it to `.gitignore`. |

Nothing else was uncommitted: `git status --porcelain` reported only that one line.

## 4. Action log

- **2026-10-25 10:35** — `stng`: checked `git status` → clean except untracked `baslib/merged.bas`;
  branch `main`, 1 commit ahead of `origin/main` (`cd35000`), HEAD `cd35000`.
- **2026-10-25 10:36** — `stng`: verified `samples/stng/startrekng.bas` (601 lines, 566 numbered,
  **no duplicate line numbers**). Found the plan's documented run path
  `out\src\newbasic-cli\Release\newbasic.exe` missing; located the live binary at
  `bin\Release\newbasic.exe` (`CMakeLists.txt:85` sets `RUNTIME_OUTPUT_DIRECTORY` to `bin/<Config>`).
- **2026-10-25 10:36** — `stng`: smoke test passed — `bin\Release\newbasic.exe --keys "N`rQ"
  --gpu-shot out\stng-smoke.ppm samples\stng\startrekng.bas` exited 0 and drew the full console
  (energy 3000→2950, time 29, all eight command buttons present).
- **2026-10-25 10:40** — `stng`: wrote `stng-handoff.md` and this file.
- **2026-10-25 10:41** — `stng`: local commit of the two files (§2). Not pushed.
- `out/stng-smoke.ppm` / `out/stng-smoke.png` were written during verification; `out/` is
  gitignored and nothing from it was committed.

## 5. Checklist for the push session

1. `git status` — working tree should show only `?? baslib/merged.bas` (or be clean).
2. `git log --oneline origin/main..main` — expect the commits in §1 (plus anything appended below).
3. `git rev-list --count origin/main..main` — 15+ means push is due; also push on explicit request.
4. `git push origin main` (never `--force`).
5. Confirm `git rev-list --count origin/main..main` is `0`, then update §1 of this file to say
   "nothing pending" with the push date and new `origin/main` hash.
6. Do not stage `out/`, `*.ppm`, `*.png`, build trees, or `baslib/merged.bas`.

## 6. Append-only section for later sessions

<!-- Add your own entry below, newest last: session, timestamp, commit hash, files, subject. -->

- **2026-10-25 10:55** — `baslib`: checked `git status`; clean except untracked
  `baslib/merged.bas`. Local branch `main`, HEAD `4d58222`, two commits ahead of
  `origin/main` (`cd35000`, `4d58222`). Wrote `baslib-handoff.md`; updated this
  ledger; made one local commit containing those two files. Did **not** push.

---

## 7. Session `baslib` handoff record

**Handoff:** `baslib-handoff.md`  
**Commit:** this ledger commit, parent `4d58222`  
**Push:** handled by the push session; this session did not push.

### Files in this local commit

| File | What it is |
|---|---|
| `baslib-handoff.md` | Current baslib context: v0.3 API, dirty painting, tests, build/run paths, next actions |
| `bigpush.md` | This ledger update |

### Uncommitted files at handoff

| File | State | Handling |
|---|---|---|
| `baslib/merged.bas` | untracked generated merge artifact | Deliberately left out; reproducible from `baslib/baslib.bas` + a host |

### baslib state in one paragraph

baslib is v0.3 with a fully parameterized `SUB`/`FUNCTION` API, no
`blApi...`/`blReq...`/`blRet...` staging globals, descriptive 4–10 character
variable names, and dirty-subtree repainting via `blUpdateAll`. `blPaintAll`
remains the full repaint; `blUpdate(Handle%)` is an unconditional targeted
subtree paint and deliberately does not clear dirty state. Last verified state:
full suite **516 cases / 32,037 assertions**, CTest **7/7**, baslib suite
**21 cases / 2,806 assertions**, CLI fixtures **3/3**.

### Push-session note

The baslib code commits (`b36137f`, `514ce87`, `d66efed`) are already on
`origin/main`. The local commits still waiting are `cd35000`, `4d58222`, and
this handoff commit. Push in that order; do not force-push.

## 8. Session `general` handoff record

**Handoff:** `general-handoff.md`  
**Commit:** this ledger commit, parent `48e1db9`  
**Push:** this session will push after committing the handoff; no force-push.

### Files in this local commit

| File | What it is |
|---|---|
| `general-handoff.md` | Current general-session context: completed features, build layout, VS Code extension, tests, pending commits, resume steps |
| `bigpush.md` | This ledger update |

### Uncommitted files at handoff

| File | State | Handling |
|---|---|---|
| `baslib/merged.bas` | Untracked generated merge artifact | Deliberately left out; reproducible from `baslib/baslib.bas` plus a host. Do not stage unless the baslib session asks. |

Nothing else was uncommitted. `git status --porcelain` reported only
`?? baslib/merged.bas`.

### Action log

- **2026-10-25 10:43** — `general`: checked `git status`; only
  `?? baslib/merged.bas` is uncommitted. Branch `main`, HEAD `48e1db9`,
  three commits ahead of `origin/main` (`d66efed`).
- **2026-10-25 10:43** — `general`: recorded the uncommitted artifact here,
  wrote `general-handoff.md`, and updated this ledger.
- **2026-10-25 10:43** — `general`: made one local commit containing
  `general-handoff.md` and `bigpush.md`. Push is the next step.
- **Last observed tests:** `32,032 assertions in 515 test cases passed` from
  `build-clean-20261025`; re-run CTest on the next machine.

### Push-session note

Push the four commits in §1 in order. After a successful push, update §1 to
“nothing pending” with the new `origin/main` hash, then commit and push that
ledger update as well.

