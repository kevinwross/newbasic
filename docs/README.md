# GW-BASIC documentation (chunked reference)

Chunked, verbatim copies of `GWBASIC.txt` — a UTF-8 text dump of the
**Microsoft GW-BASIC User's Guide + User's Reference** (1986/1987,
document 410130013-330-R02-0787; 18,729 lines, ~382 KB total). The full
original file is kept at the repo root; the chunks under `gw-basic/` are
split on section boundaries so individual topics fit in a single read.

Each chunk carries a header comment with its source line range. Text is
**verbatim, including OCR artifacts** (e.g. "PDF nage", "retum", "whieh",
curly quotes); read around them.

> **Copyright:** this is Microsoft copyrighted material kept as a *local*
> reference. GW-Basic was released on GitHub under an MIT license. `

## Chunk index

| File | `GWBASIC.txt` lines | Contents |
|---|---|---|
| `gw-basic/00-front-matter-and-tocs.md` | 1–1077 |  "Introduction", User's Guide TOC, User's Reference TOC, preface material |
| `gw-basic/01-ch1-welcome.md` | 1078–1207 | User's Guide — Ch. 1: Welcome to GW-BASIC (manual overview) |
| `gw-basic/02-ch2-getting-started.md` | 1208–1559 | Ch. 2: Getting Started (first programs, direct mode, editor intro) |
| `gw-basic/03-ch3-reviewing-practicing.md` | 1560–1789 | Ch. 3: Reviewing and Practicing (exercises) |
| `gw-basic/04-ch4-screen-editor.md` | 1790–2110 | Ch. 4: The Screen Editor (editing keys, F-keys) |
| `gw-basic/05-ch5-creating-using-files.md` | 2111–2784 | Ch. 5: Creating and Using Files (SAVE/LOAD, disks) |
| `gw-basic/06-ch6-constants-variables-operators.md` | 2785–3646 | Ch. 6: Constants, Variables, Expressions, Operators (literal syntax, type suffixes, precedence) |
| `gw-basic/07-appendix-a-error-codes.md` | 3647–3981 | Appendix A: **Error Codes and Messages** (full ERR/ERL table) |
| `gw-basic/08-appendix-b-math-functions.md` | 3982–4050 | Appendix B: Mathematical Functions (precision/rounding notes) |
| `gw-basic/09-appendix-c-ascii-codes.md` | 4051–5114 | Appendix C: ASCII Character Codes |
| `gw-basic/10-appendix-d-assembly.md` | 5115–5878 | Appendix D: Assembly Language Subroutines (CALL/USR background) |
| `gw-basic/11-appendix-e-converting.md` | 5879–5947 | Appendix E: Converting BASIC Programs to GW-BASIC |
| `gw-basic/12-appendix-f-communications.md` | 5948–6326 | Appendix F: Communications (RS-232, COM files) |
| `gw-basic/13-appendix-g-hex-equivalents.md` | 6327–6779 | Appendix G: Hexadecimal Equivalents |
| `gw-basic/14-appendix-h-key-scan-codes.md` | 6780–7121 | Appendix H: **Key Scan Codes** (values behind `INKEY$`) |
| `gw-basic/15-appendix-i-characters.md` | 7122–7211 | Appendix I: Characters Recognized by GW-BASIC |
| `gw-basic/16-glossary.md` | 7212–8126 | Glossary |
| `gw-basic/17-reference-abs-to-csrlin.md` | 8127–9410 | Reference: ABS, ASC, ATN, AUTO, BEEP, BLOAD, BSAVE, CALL, CDBL, CHAIN, CHDIR, CHR$, CINT, CIRCLE, CLEAR, CLOSE, CLS, COLOR, COMMON, CONT, CSNG, CSRLIN |
| `gw-basic/18-reference-data-to-fix.md` | 9411–10819 | Reference: DATA, DEF FN, DEF SEG, DEF USR, DELETE, DIM, DRAW, EDIT, END, ENVIRON, ENVIRONS, EOF, ERASE, ERROR, EXP, EXTERR, FIELD, FILES, FIX, **FOR... NEXT** (heading not in TOC style); also ERR/ERL variables (p. 129–130) |
| `gw-basic/19-reference-fre-to-line.md` | 10820–12228 | Reference: FRE, GOSUB...RETURN, GOTO, **GET (Files) / GET (Graphics)**, HEX$, IF...THEN...ELSE, INKEYS, INP, INPUT, INPUT#, INPUTS, INSTR, INT, IOCTL$, KEY, KILL, LEFT$, LET, LINE |
| `gw-basic/20-reference-line-input-to-on-error.md` | 12229–13420 | Reference: LINE INPUT, LINE INPUT#, LIST, LLIST, LOAD, LOC, LOCATE, LOCK, LOF, LOG, LPOS, MERGE, MID$ (Function), MID$ (Statement), MKDIR, NAME, NEW, OCT$, ON ERROR GOTO |
| `gw-basic/21-reference-open-to-pos.md` | 13421–14827 | Reference: **OPEN**, OPTION BASE, OUT, PAINT, PCOPY, PEEK, PLAY, POINT, POKE, POS |
| `gw-basic/22-reference-print-to-screen.md` | 14828–16012 | Reference: **PRINT**, **PRINT USING**, RANDOMIZE, READ, REM, RENUM, RESET, RESTORE, RESUME, RETURN, RIGHTS, RMDIR, RND, RUN, **SAVE**, SCREEN (function) |
| `gw-basic/23-reference-screen-statement-to-tab.md` | 16013–17552 | Reference: SCREEN (statement), SGN, **SHELL**, SIN, SOUND, SPACES, SPC, SQR, STICK, STOP, STR$, STRINGS, SWAP, SYSTEM, TAB |
| `gw-basic/24-reference-tan-to-write.md` | 17553–18729 | Reference: TAN, TIMER, UNLOCK, USR, VAL, VARPTR, VARPTRS, VIEW, VIEW PRINT, WAIT, WHILE...WEND, WIDTH, WINDOW, WRITE, WRITE# |

## Where to find what (implementation map)

| Topic | Chunk(s) |
|---|---|
| `PRINT` field/number formatting rules (M0/M2 formatter) | `22` (PRINT, PRINT USING), `23` (SPC, TAB), `21` (POS) |
| Error code table / `ERR`+`ERL` semantics (error model) | `07` (App. A), `18` (ERROR, EXTERR, ERR/ERL) |
| `INKEY$` key/arrow code values (key mapping) | `14` (App. H), `19` (INKEYS) |
| Numeric literal syntax, type suffixes `%!#&$`, operator precedence | `06` (Ch. 6), `08` (App. B) |
| File I/O: OPEN modes, `INPUT#`/`PRINT#`/`LINE INPUT#`, GET/PUT, LOF/EOF/LOC | `21` (OPEN), `19` (GET/INPUT#), `20` (LINE INPUT#, LOAD, LIST), `24` (WRITE#) |
| `SAVE`/`LOAD` (text `.BAS` only; tokenized removed), BSAVE/BLOAD | `22` (SAVE), `20` (LOAD), `17` (BSAVE/BLOAD) |
| `SHELL` | `23` |
| Screen ops: CLS, COLOR, LOCATE, SCREEN, VIEW/VIEW PRINT | `17` (CLS, COLOR), `19` (LOCATE), `23` (SCREEN), `24` (VIEW) |
| Control flow: IF/FOR...NEXT/GOSUB/WHILE...WEND/ON ERROR | `19` (IF, GOSUB), `18` (FOR... NEXT), `24` (WHILE...WEND), `20` (ON ERROR GOTO), `22` (RESUME) |
| Arrays: DIM, ERASE, OPTION BASE | `18` (DIM, ERASE), `21` (OPTION BASE) |
| Random: RND, RANDOMIZE | `22` |
| Memory: PEEK/POKE/USR/VARPTR | `21` (PEEK/POKE), `24` (USR, VARPTR) |
| Character set / CP437-era glyph background | `09` (App. C), `15` (App. I) |
| Direct mode, editor keys, first-run behavior | `02`–`04` (Ch. 2–4) |

## Regenerating chunks

Chunk boundaries are the section starts listed above; the one-off script
used is not committed (see session history). If `GWBASIC.txt` is ever
replaced, re-derive boundaries from the `Chapter N` / `Appendix X` /
entry-heading lines and re-run.
