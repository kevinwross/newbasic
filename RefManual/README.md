# newbasic Reference Manual

This is the alphabetical reference for the newbasic dialect: its statements, functions, commands, variables, and file syntax. It follows the GW-BASIC reference style and records newbasic’s behaviour explicitly where it differs.

## How this manual is organised

Entries are grouped by the first letter of the primary keyword:

- `A.md` through `Z.md` contain the entries.
- Entries within each file are alphabetical.
- The global index below is generated from the entry headings and status lines.
- Every entry states whether newbasic supports it.

Quick letter links:

[A](A.md) · [B](B.md) · [C](C.md) · [D](D.md) · [E](E.md) · [F](F.md) · [G](G.md) · [H](H.md) · [I](I.md) · [J](J.md) · [K](K.md) · [L](L.md) · [M](M.md) · [N](N.md) · [O](O.md) · [P](P.md) · [Q](Q.md) · [R](R.md) · [S](S.md) · [T](T.md) · [U](U.md) · [V](V.md) · [W](W.md) · [X](X.md) · [Y](Y.md) · [Z](Z.md)

The manual is a reference, not a tutorial. For worked programs, see `samples/` and `fixtures/`.

## Status values

| Status | Meaning |
|---|---|
| **Supported** | Implemented and covered by the test suite or runnable examples. |
| **Partial** | Implemented with a documented limitation. |
| **Not supported** | Recognised as a GW-BASIC keyword, but deliberately unresolved or unavailable in newbasic. The entry gives the GW-BASIC syntax and the newbasic status. |
| **Supported in PRINT only** | Valid only inside a `PRINT`/`LPRINT`/file-print item list. |
| **Direct mode only** | Works at the `Ok` prompt, but not inside a stored program. |

## Alphabetical index

<!-- INDEX:START -->
| Entry | Type | Status | Location |
|---|---|---|---|
| ABS Function | Function | Supported | [A.md](A.md#abs-function) |
| ASC Function | Function | Supported | [A.md](A.md#asc-function) |
| ATN Function | Function | Supported | [A.md](A.md#atn-function) |
| AUTO Command | Command | Direct mode only | [A.md](A.md#auto-command) |
| BEEP Statement | Statement | Supported | [B.md](B.md#beep-statement) |
| BLOAD Command | Command | Not supported in newbasic | [B.md](B.md#bload-command) |
| BSAVE Command | Command | Not supported in newbasic | [B.md](B.md#bsave-command) |
| CALL Statement | Statement | Not supported in newbasic | [C.md](C.md#call-statement) |
| CDBL Function | Function | Supported | [C.md](C.md#cdbl-function) |
| CHAIN Statement | Statement | Partial | [C.md](C.md#chain-statement) |
| CHDIR Command | Command | Direct mode only | [C.md](C.md#chdir-command) |
| CHR$ Function | Function | Supported | [C.md](C.md#chr-function) |
| CINT Function | Function | Supported | [C.md](C.md#cint-function) |
| CIRCLE Statement | Statement | Supported | [C.md](C.md#circle-statement) |
| CLEAR Statement | Statement / Command | Partial | [C.md](C.md#clear-statement) |
| CLOSE Statement | Statement | Supported | [C.md](C.md#close-statement) |
| CLS Statement | Statement | Supported | [C.md](C.md#cls-statement) |
| COLOR Statement | Statement | Supported | [C.md](C.md#color-statement) |
| COM(n) Statement | Statement | Not supported in newbasic | [C.md](C.md#comn-statement) |
| COMMON Statement | Statement | Partial | [C.md](C.md#common-statement) |
| CONT Command | Command | Direct mode only | [C.md](C.md#cont-command) |
| COS Function | Function | Supported | [C.md](C.md#cos-function) |
| CSNG Function | Function | Supported | [C.md](C.md#csng-function) |
| CSRLIN Function | Function | Supported | [C.md](C.md#csrlin-function) |
| CVD Function | Function | Supported | [C.md](C.md#cvd-function) |
| CVI Function | Function | Supported | [C.md](C.md#cvi-function) |
| CVS Function | Function | Supported | [C.md](C.md#cvs-function) |
| DATA Statement | Statement | Supported | [D.md](D.md#data-statement) |
| DATE$ Function | Function | Supported | [D.md](D.md#date-function) |
| DEF FN Statement | Statement | Supported | [D.md](D.md#def-fn-statement) |
| DEF SEG Statement | Statement | Not supported in newbasic | [D.md](D.md#def-seg-statement) |
| DEF USR Statement | Statement | Not supported in newbasic | [D.md](D.md#def-usr-statement) |
| DEFDBL Statement | Statement | Supported | [D.md](D.md#defdbl-statement) |
| DEFINT Statement | Statement | Supported | [D.md](D.md#defint-statement) |
| DEFSNG Statement | Statement | Supported | [D.md](D.md#defsng-statement) |
| DEFSTR Statement | Statement | Supported | [D.md](D.md#defstr-statement) |
| DELETE Command | Command | Direct mode only | [D.md](D.md#delete-command) |
| DIM Statement | Statement | Supported | [D.md](D.md#dim-statement) |
| DRAW Statement | Statement | Supported | [D.md](D.md#draw-statement) |
| EDIT Command | Command | Not supported in newbasic | [E.md](E.md#edit-command) |
| END Statement | Statement | Supported | [E.md](E.md#end-statement) |
| ENVIRON Statement | Statement | Not supported in newbasic | [E.md](E.md#environ-statement) |
| ENVIRON$ Function | Function | Not supported in newbasic | [E.md](E.md#environ-function) |
| EOF Function | Function | Supported | [E.md](E.md#eof-function) |
| ERASE Statement | Statement | Supported | [E.md](E.md#erase-statement) |
| ERDEV Variable | Variable | Not supported in newbasic | [E.md](E.md#erdev-variable) |
| ERDEV$ Variable | Variable | Not supported in newbasic | [E.md](E.md#erdev-variable-2) |
| ERL Variable | Variable | Supported | [E.md](E.md#erl-variable) |
| ERR Variable | Variable | Supported | [E.md](E.md#err-variable) |
| ERROR Statement | Statement | Supported | [E.md](E.md#error-statement) |
| EXP Function | Function | Supported | [E.md](E.md#exp-function) |
| EXTERR Function | Function | Not supported in newbasic | [E.md](E.md#exterr-function) |
| FIELD Statement | Statement | Supported | [F.md](F.md#field-statement) |
| FILES Command | Command | Direct mode only | [F.md](F.md#files-command) |
| FIX Function | Function | Supported | [F.md](F.md#fix-function) |
| FOR Statement | Statement | Supported | [F.md](F.md#for-statement) |
| FRE Function | Function | Not supported in newbasic | [F.md](F.md#fre-function) |
| GET Statement | Statement | Supported | [G.md](G.md#get-statement) |
| GOSUB Statement | Statement | Supported | [G.md](G.md#gosub-statement) |
| GOTO Statement | Statement | Supported | [G.md](G.md#goto-statement) |
| HEX$ Function | Function | Supported | [H.md](H.md#hex-function) |
| IF Statement | Statement | Supported | [I.md](I.md#if-statement) |
| INKEY$ Function | Function | Supported | [I.md](I.md#inkey-function) |
| INP Function | Function | Not supported in newbasic | [I.md](I.md#inp-function) |
| INPUT Statement | Statement | Supported | [I.md](I.md#input-statement) |
| INPUT# Statement | Statement | Supported | [I.md](I.md#input-statement-2) |
| INPUT$ Function | Function | Supported | [I.md](I.md#input-function) |
| INSTR Function | Function | Supported | [I.md](I.md#instr-function) |
| INT Function | Function | Supported | [I.md](I.md#int-function) |
| IOCTL Statement | Statement | Not supported in newbasic | [I.md](I.md#ioctl-statement) |
| IOCTL$ Function | Function | Not supported in newbasic | [I.md](I.md#ioctl-function) |
| KEY Statement | Statement | Partial | [K.md](K.md#key-statement) |
| KEY(n) Statement | Statement | Partial | [K.md](K.md#keyn-statement) |
| KILL Command | Command | Direct mode only | [K.md](K.md#kill-command) |
| LEFT$ Function | Function | Supported | [L.md](L.md#left-function) |
| LEN Function | Function | Supported | [L.md](L.md#len-function) |
| LET Statement | Statement | Supported | [L.md](L.md#let-statement) |
| LINE INPUT Statement | Statement | Supported | [L.md](L.md#line-input-statement) |
| LINE INPUT# Statement | Statement | Supported | [L.md](L.md#line-input-statement-2) |
| LINE Statement | Statement | Supported | [L.md](L.md#line-statement) |
| LIST Command | Command | Direct mode only | [L.md](L.md#list-command) |
| LLIST Command | Command | Direct mode only | [L.md](L.md#llist-command) |
| LOAD Command | Command | Supported | [L.md](L.md#load-command) |
| LOC Function | Function | Supported | [L.md](L.md#loc-function) |
| LOCATE Statement | Statement | Supported | [L.md](L.md#locate-statement) |
| LOCK Statement | Statement | Not supported in newbasic | [L.md](L.md#lock-statement) |
| LOF Function | Function | Supported | [L.md](L.md#lof-function) |
| LOG Function | Function | Supported | [L.md](L.md#log-function) |
| LPOS Function | Function | Not supported in newbasic | [L.md](L.md#lpos-function) |
| LPRINT Statement | Statement | Supported | [L.md](L.md#lprint-statement) |
| LPRINT USING Statement | Statement | Supported | [L.md](L.md#lprint-using-statement) |
| LSET Statement | Statement | Supported | [L.md](L.md#lset-statement) |
| MERGE Command | Command | Direct mode only | [M.md](M.md#merge-command) |
| MID$ Function | Function | Supported | [M.md](M.md#mid-function) |
| MKD$ Function | Function | Supported | [M.md](M.md#mkd-function) |
| MKDIR Command | Command | Direct mode only | [M.md](M.md#mkdir-command) |
| MKI$ Function | Function | Supported | [M.md](M.md#mki-function) |
| MKS$ Function | Function | Supported | [M.md](M.md#mks-function) |
| MOTOR Statement | Statement | Not supported in newbasic | [M.md](M.md#motor-statement) |
| NAME Command | Command | Direct mode only | [N.md](N.md#name-command) |
| NEW Command | Command | Direct mode only | [N.md](N.md#new-command) |
| NEXT Statement | Statement | Supported | [N.md](N.md#next-statement) |
| OCT$ Function | Function | Supported | [O.md](O.md#oct-function) |
| ON COM(n) Statement | Statement | Not supported in newbasic | [O.md](O.md#on-comn-statement) |
| ON ERROR GOTO Statement | Statement | Supported | [O.md](O.md#on-error-goto-statement) |
| ON expression GOSUB Statement | Statement | Supported | [O.md](O.md#on-expression-gosub-statement) |
| ON expression GOTO Statement | Statement | Supported | [O.md](O.md#on-expression-goto-statement) |
| ON KEY(n) Statement | Statement | Partial | [O.md](O.md#on-keyn-statement) |
| ON PEN Statement | Statement | Not supported in newbasic | [O.md](O.md#on-pen-statement) |
| ON PLAY(n) Statement | Statement | Not supported in newbasic | [O.md](O.md#on-playn-statement) |
| ON STRIG(n) Statement | Statement | Not supported in newbasic | [O.md](O.md#on-strign-statement) |
| ON TIMER(n) Statement | Statement | Supported | [O.md](O.md#on-timern-statement) |
| OPEN "COM" Statement | Statement | Not supported in newbasic | [O.md](O.md#open-com-statement) |
| OPEN Statement | Statement | Supported | [O.md](O.md#open-statement) |
| OPTION BASE Statement | Statement | Supported | [O.md](O.md#option-base-statement) |
| OUT Statement | Statement | Not supported in newbasic | [O.md](O.md#out-statement) |
| PAINT Statement | Statement | Supported | [P.md](P.md#paint-statement) |
| PALETTE Statement | Statement | Supported | [P.md](P.md#palette-statement) |
| PALETTE USING Statement | Statement | Not supported in newbasic | [P.md](P.md#palette-using-statement) |
| PCOPY Command | Command | Supported | [P.md](P.md#pcopy-command) |
| PEEK Function | Function | Not supported in newbasic | [P.md](P.md#peek-function) |
| PEN Function | Function | Not supported in newbasic | [P.md](P.md#pen-function) |
| PEN Statement | Statement | Not supported in newbasic | [P.md](P.md#pen-statement) |
| PLAY Statement | Statement | Supported | [P.md](P.md#play-statement) |
| PLAY() Function | Function | Not supported in newbasic | [P.md](P.md#play-function) |
| PMAP Function | Function | Supported | [P.md](P.md#pmap-function) |
| POINT Function | Function | Supported | [P.md](P.md#point-function) |
| POKE Statement | Statement | Not supported in newbasic | [P.md](P.md#poke-statement) |
| POS Function | Function | Supported | [P.md](P.md#pos-function) |
| PRESET Statement | Statement | Supported | [P.md](P.md#preset-statement) |
| PRINT Statement | Statement | Supported | [P.md](P.md#print-statement) |
| PRINT USING Statement | Statement | Supported | [P.md](P.md#print-using-statement) |
| PRINT# Statement | Statement | Supported | [P.md](P.md#print-statement-2) |
| PRINT# USING Statement | Statement | Supported | [P.md](P.md#print-using-statement-2) |
| PSET Statement | Statement | Supported | [P.md](P.md#pset-statement) |
| PUT Statement | Statement | Supported | [P.md](P.md#put-statement) |
| RANDOMIZE Statement | Statement | Supported | [R.md](R.md#randomize-statement) |
| READ Statement | Statement | Supported | [R.md](R.md#read-statement) |
| REM Statement | Statement | Supported | [R.md](R.md#rem-statement) |
| RENUM Command | Command | Direct mode only | [R.md](R.md#renum-command) |
| RESET Command | Command | Not supported in newbasic | [R.md](R.md#reset-command) |
| RESTORE Statement | Statement | Supported | [R.md](R.md#restore-statement) |
| RESUME Statement | Statement | Supported | [R.md](R.md#resume-statement) |
| RETURN Statement | Statement | Supported | [R.md](R.md#return-statement) |
| RIGHT$ Function | Function | Supported | [R.md](R.md#right-function) |
| RMDIR Command | Command | Direct mode only | [R.md](R.md#rmdir-command) |
| RND Function | Function | Supported | [R.md](R.md#rnd-function) |
| RSET Statement | Statement | Supported | [R.md](R.md#rset-statement) |
| RUN Command | Command | Supported | [R.md](R.md#run-command) |
| SAVE Command | Command | Supported | [S.md](S.md#save-command) |
| SCREEN Function | Function | Not supported in newbasic | [S.md](S.md#screen-function) |
| SCREEN Statement | Statement | Supported | [S.md](S.md#screen-statement) |
| SGN Function | Function | Supported | [S.md](S.md#sgn-function) |
| SHELL Statement | Statement | Supported | [S.md](S.md#shell-statement) |
| SIN Function | Function | Supported | [S.md](S.md#sin-function) |
| SOUND Statement | Statement | Supported | [S.md](S.md#sound-statement) |
| SPACE$ Function | Function | Supported | [S.md](S.md#space-function) |
| SPC Function | Function | Supported in PRINT only | [S.md](S.md#spc-function) |
| SQR Function | Function | Supported | [S.md](S.md#sqr-function) |
| STICK Function | Function | Not supported in newbasic | [S.md](S.md#stick-function) |
| STOP Statement | Statement | Supported | [S.md](S.md#stop-statement) |
| STR$ Function | Function | Supported | [S.md](S.md#str-function) |
| STRIG Function | Function | Not supported in newbasic | [S.md](S.md#strig-function) |
| STRIG Statement | Statement | Not supported in newbasic | [S.md](S.md#strig-statement) |
| STRING$ Function | Function | Supported | [S.md](S.md#string-function) |
| SWAP Statement | Statement | Supported | [S.md](S.md#swap-statement) |
| SYSTEM Command | Command | Direct mode only | [S.md](S.md#system-command) |
| TAB Function | Function | Supported in PRINT only | [T.md](T.md#tab-function) |
| TAN Function | Function | Supported | [T.md](T.md#tan-function) |
| TIME$ Function | Function | Supported | [T.md](T.md#time-function) |
| TIMER Function | Function | Supported | [T.md](T.md#timer-function) |
| TIMER Statement | Statement | Supported | [T.md](T.md#timer-statement) |
| TROFF Command | Command | Supported | [T.md](T.md#troff-command) |
| TRON Command | Command | Supported | [T.md](T.md#tron-command) |
| UNLOCK Statement | Statement | Not supported in newbasic | [U.md](U.md#unlock-statement) |
| USR Function | Function | Not supported in newbasic | [U.md](U.md#usr-function) |
| VAL Function | Function | Supported | [V.md](V.md#val-function) |
| VARPTR Function | Function | Not supported in newbasic | [V.md](V.md#varptr-function) |
| VARPTR$ Function | Function | Not supported in newbasic | [V.md](V.md#varptr-function-2) |
| VIEW PRINT Statement | Statement | Supported | [V.md](V.md#view-print-statement) |
| VIEW Statement | Statement | Supported | [V.md](V.md#view-statement) |
| WAIT Statement | Statement | Not supported in newbasic | [W.md](W.md#wait-statement) |
| WEND Statement | Statement | Supported | [W.md](W.md#wend-statement) |
| WHILE Statement | Statement | Supported | [W.md](W.md#while-statement) |
| WIDTH Statement | Statement | Partial | [W.md](W.md#width-statement) |
| WINDOW Statement | Statement | Supported | [W.md](W.md#window-statement) |
| WRITE Statement | Statement | Supported | [W.md](W.md#write-statement) |
| WRITE# Statement | Statement | Supported | [W.md](W.md#write-statement-2) |
<!-- INDEX:END -->

## Unsupported keywords at a glance

The full reasons are in each letter file. This list is for quick lookup.

<!-- UNSUPPORTED:START -->
| Entry | Location |
|---|---|
| BLOAD Command | [B.md](B.md#bload-command) |
| BSAVE Command | [B.md](B.md#bsave-command) |
| CALL Statement | [C.md](C.md#call-statement) |
| COM(n) Statement | [C.md](C.md#comn-statement) |
| DEF SEG Statement | [D.md](D.md#def-seg-statement) |
| DEF USR Statement | [D.md](D.md#def-usr-statement) |
| EDIT Command | [E.md](E.md#edit-command) |
| ENVIRON Statement | [E.md](E.md#environ-statement) |
| ENVIRON$ Function | [E.md](E.md#environ-function) |
| ERDEV Variable | [E.md](E.md#erdev-variable) |
| ERDEV$ Variable | [E.md](E.md#erdev-variable-2) |
| EXTERR Function | [E.md](E.md#exterr-function) |
| FRE Function | [F.md](F.md#fre-function) |
| INP Function | [I.md](I.md#inp-function) |
| IOCTL Statement | [I.md](I.md#ioctl-statement) |
| IOCTL$ Function | [I.md](I.md#ioctl-function) |
| LOCK Statement | [L.md](L.md#lock-statement) |
| LPOS Function | [L.md](L.md#lpos-function) |
| MOTOR Statement | [M.md](M.md#motor-statement) |
| ON COM(n) Statement | [O.md](O.md#on-comn-statement) |
| ON PEN Statement | [O.md](O.md#on-pen-statement) |
| ON PLAY(n) Statement | [O.md](O.md#on-playn-statement) |
| ON STRIG(n) Statement | [O.md](O.md#on-strign-statement) |
| OPEN "COM" Statement | [O.md](O.md#open-com-statement) |
| OUT Statement | [O.md](O.md#out-statement) |
| PALETTE USING Statement | [P.md](P.md#palette-using-statement) |
| PEEK Function | [P.md](P.md#peek-function) |
| PEN Function | [P.md](P.md#pen-function) |
| PEN Statement | [P.md](P.md#pen-statement) |
| PLAY() Function | [P.md](P.md#play-function) |
| POKE Statement | [P.md](P.md#poke-statement) |
| RESET Command | [R.md](R.md#reset-command) |
| SCREEN Function | [S.md](S.md#screen-function) |
| STICK Function | [S.md](S.md#stick-function) |
| STRIG Function | [S.md](S.md#strig-function) |
| STRIG Statement | [S.md](S.md#strig-statement) |
| UNLOCK Statement | [U.md](U.md#unlock-statement) |
| USR Function | [U.md](U.md#usr-function) |
| VARPTR Function | [V.md](V.md#varptr-function) |
| VARPTR$ Function | [V.md](V.md#varptr-function-2) |
| WAIT Statement | [W.md](W.md#wait-statement) |
<!-- UNSUPPORTED:END -->

## Conventions

- Basic source is shown with line numbers, as it appears in a stored program.
- `[...]` marks optional syntax.
- `expression` means any numeric or string expression accepted in that position.
- Examples are intended to run in a normal build. Some require interactive input, a companion data or program file, or a graphics window; those dependencies are noted in the entry.
- `Not supported` examples are shown only to document the GW-BASIC form. They are not expected to run.
- Line numbers in examples are BASIC line numbers, not editor line numbers.

Use [_template.md](_template.md) when adding an entry. The index can be regenerated with `pwsh -File RefManual/_build-index.ps1`.

## Related documentation

- [GW-BASIC reference source](../docs/gw-basic/)
- [newbasic specification and milestone plan](../docs/newbasic-spec-plan.md)
- [DAP debugger](../docs/m5-dap-debugger.md)
- [Build instructions for Windows](../buildonwindows.md)
