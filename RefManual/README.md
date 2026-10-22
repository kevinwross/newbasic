# newbasic Reference Manual

This is the alphabetical reference for the newbasic dialect: its statements, functions, commands, variables, and file syntax. It follows the GW-BASIC reference style and records newbasic’s behaviour explicitly where it differs.

## How this manual is organised

Entries are grouped by the first letter of the primary keyword:

- `A.md` through `Z.md` contain the entries.
- Entries within each file are alphabetical.
- The global index below is generated from the entry headings and status lines.
- Every entry states whether newbasic supports it.

Quick letter links:

[A.md](A.md) · [B.md](B.md) · [C.md](C.md) · [D.md](D.md) · [E.md](E.md) · [F.md](F.md) · [G.md](G.md) · [H.md](H.md) · [I.md](I.md) · [J.md](J.md) · [K.md](K.md) · [L.md](L.md) · [M.md](M.md) · [N.md](N.md) · [O.md](O.md) · [P.md](P.md) · [Q.md](Q.md) · [R.md](R.md) · [S.md](S.md) · [T.md](T.md) · [U.md](U.md) · [V.md](V.md) · [W.md](W.md) · [X.md](X.md) · [Y.md](Y.md) · [Z.md](Z.md)

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
| Entry | Type | Status |
|---|---|---|
| [ABS Function](A.md#abs-function) | Function | Supported |
| [ASC Function](A.md#asc-function) | Function | Supported |
| [ATN Function](A.md#atn-function) | Function | Supported |
| [AUTO Command](A.md#auto-command) | Command | Direct mode only |
| [BEEP Statement](B.md#beep-statement) | Statement | Supported |
| [BLOAD Command](B.md#bload-command) | Command | Not supported in newbasic |
| [BSAVE Command](B.md#bsave-command) | Command | Not supported in newbasic |
| [CALL Statement](C.md#call-statement) | Statement | Not supported in newbasic |
| [CDBL Function](C.md#cdbl-function) | Function | Supported |
| [CHAIN Statement](C.md#chain-statement) | Statement | Partial |
| [CHDIR Command](C.md#chdir-command) | Command | Direct mode only |
| [CHR$ Function](C.md#chr-function) | Function | Supported |
| [CINT Function](C.md#cint-function) | Function | Supported |
| [CIRCLE Statement](C.md#circle-statement) | Statement | Supported |
| [CLEAR Statement](C.md#clear-statement) | Statement / Command | Partial |
| [CLOSE Statement](C.md#close-statement) | Statement | Supported |
| [CLS Statement](C.md#cls-statement) | Statement | Supported |
| [COLOR Statement](C.md#color-statement) | Statement | Supported |
| [COM(n) Statement](C.md#comn-statement) | Statement | Not supported in newbasic |
| [COMMON Statement](C.md#common-statement) | Statement | Partial |
| [CONT Command](C.md#cont-command) | Command | Direct mode only |
| [COS Function](C.md#cos-function) | Function | Supported |
| [CSNG Function](C.md#csng-function) | Function | Supported |
| [CSRLIN Function](C.md#csrlin-function) | Function | Supported |
| [CVD Function](C.md#cvd-function) | Function | Supported |
| [CVI Function](C.md#cvi-function) | Function | Supported |
| [CVS Function](C.md#cvs-function) | Function | Supported |
| [DATA Statement](D.md#data-statement) | Statement | Supported |
| [DATE$ Function](D.md#date-function) | Function | Supported |
| [DEF FN Statement](D.md#def-fn-statement) | Statement | Supported |
| [DEF SEG Statement](D.md#def-seg-statement) | Statement | Not supported in newbasic |
| [DEF USR Statement](D.md#def-usr-statement) | Statement | Not supported in newbasic |
| [DEFDBL Statement](D.md#defdbl-statement) | Statement | Supported |
| [DEFINT Statement](D.md#defint-statement) | Statement | Supported |
| [DEFSNG Statement](D.md#defsng-statement) | Statement | Supported |
| [DEFSTR Statement](D.md#defstr-statement) | Statement | Supported |
| [DELETE Command](D.md#delete-command) | Command | Direct mode only |
| [DIM Statement](D.md#dim-statement) | Statement | Supported |
| [DRAW Statement](D.md#draw-statement) | Statement | Supported |
| [EDIT Command](E.md#edit-command) | Command | Not supported in newbasic |
| [END Statement](E.md#end-statement) | Statement | Supported |
| [ENVIRON Statement](E.md#environ-statement) | Statement | Not supported in newbasic |
| [ENVIRON$ Function](E.md#environ-function) | Function | Not supported in newbasic |
| [EOF Function](E.md#eof-function) | Function | Supported |
| [ERASE Statement](E.md#erase-statement) | Statement | Supported |
| [ERDEV Variable](E.md#erdev-variable) | Variable | Not supported in newbasic |
| [ERDEV$ Variable](E.md#erdev-variable-2) | Variable | Not supported in newbasic |
| [ERL Variable](E.md#erl-variable) | Variable | Supported |
| [ERR Variable](E.md#err-variable) | Variable | Supported |
| [ERROR Statement](E.md#error-statement) | Statement | Supported |
| [EXP Function](E.md#exp-function) | Function | Supported |
| [EXTERR Function](E.md#exterr-function) | Function | Not supported in newbasic |
| [FIELD Statement](F.md#field-statement) | Statement | Supported |
| [FILES Command](F.md#files-command) | Command | Direct mode only |
| [FIX Function](F.md#fix-function) | Function | Supported |
| [FOR Statement](F.md#for-statement) | Statement | Supported |
| [FRE Function](F.md#fre-function) | Function | Not supported in newbasic |
| [GET Statement](G.md#get-statement) | Statement | Supported |
| [GOSUB Statement](G.md#gosub-statement) | Statement | Supported |
| [GOTO Statement](G.md#goto-statement) | Statement | Supported |
| [HEX$ Function](H.md#hex-function) | Function | Supported |
| [IF Statement](I.md#if-statement) | Statement | Supported |
| [INKEY$ Function](I.md#inkey-function) | Function | Supported |
| [INP Function](I.md#inp-function) | Function | Not supported in newbasic |
| [INPUT Statement](I.md#input-statement) | Statement | Supported |
| [INPUT# Statement](I.md#input-statement-2) | Statement | Supported |
| [INPUT$ Function](I.md#input-function) | Function | Supported |
| [INSTR Function](I.md#instr-function) | Function | Supported |
| [INT Function](I.md#int-function) | Function | Supported |
| [IOCTL Statement](I.md#ioctl-statement) | Statement | Not supported in newbasic |
| [IOCTL$ Function](I.md#ioctl-function) | Function | Not supported in newbasic |
| [KEY Statement](K.md#key-statement) | Statement | Partial |
| [KEY(n) Statement](K.md#keyn-statement) | Statement | Partial |
| [KILL Command](K.md#kill-command) | Command | Direct mode only |
| [LEFT$ Function](L.md#left-function) | Function | Supported |
| [LEN Function](L.md#len-function) | Function | Supported |
| [LET Statement](L.md#let-statement) | Statement | Supported |
| [LINE INPUT Statement](L.md#line-input-statement) | Statement | Supported |
| [LINE INPUT# Statement](L.md#line-input-statement-2) | Statement | Supported |
| [LINE Statement](L.md#line-statement) | Statement | Supported |
| [LIST Command](L.md#list-command) | Command | Direct mode only |
| [LLIST Command](L.md#llist-command) | Command | Direct mode only |
| [LOAD Command](L.md#load-command) | Command | Supported |
| [LOC Function](L.md#loc-function) | Function | Supported |
| [LOCATE Statement](L.md#locate-statement) | Statement | Supported |
| [LOCK Statement](L.md#lock-statement) | Statement | Not supported in newbasic |
| [LOF Function](L.md#lof-function) | Function | Supported |
| [LOG Function](L.md#log-function) | Function | Supported |
| [LPOS Function](L.md#lpos-function) | Function | Not supported in newbasic |
| [LPRINT Statement](L.md#lprint-statement) | Statement | Supported |
| [LPRINT USING Statement](L.md#lprint-using-statement) | Statement | Supported |
| [LSET Statement](L.md#lset-statement) | Statement | Supported |
| [MERGE Command](M.md#merge-command) | Command | Direct mode only |
| [MID$ Function](M.md#mid-function) | Function | Supported |
| [MKD$ Function](M.md#mkd-function) | Function | Supported |
| [MKDIR Command](M.md#mkdir-command) | Command | Direct mode only |
| [MKI$ Function](M.md#mki-function) | Function | Supported |
| [MKS$ Function](M.md#mks-function) | Function | Supported |
| [MOTOR Statement](M.md#motor-statement) | Statement | Not supported in newbasic |
| [NAME Command](N.md#name-command) | Command | Direct mode only |
| [NEW Command](N.md#new-command) | Command | Direct mode only |
| [NEXT Statement](N.md#next-statement) | Statement | Supported |
| [OCT$ Function](O.md#oct-function) | Function | Supported |
| [ON COM(n) Statement](O.md#on-comn-statement) | Statement | Not supported in newbasic |
| [ON ERROR GOTO Statement](O.md#on-error-goto-statement) | Statement | Supported |
| [ON expression GOSUB Statement](O.md#on-expression-gosub-statement) | Statement | Supported |
| [ON expression GOTO Statement](O.md#on-expression-goto-statement) | Statement | Supported |
| [ON KEY(n) Statement](O.md#on-keyn-statement) | Statement | Partial |
| [ON PEN Statement](O.md#on-pen-statement) | Statement | Not supported in newbasic |
| [ON PLAY(n) Statement](O.md#on-playn-statement) | Statement | Not supported in newbasic |
| [ON STRIG(n) Statement](O.md#on-strign-statement) | Statement | Not supported in newbasic |
| [ON TIMER(n) Statement](O.md#on-timern-statement) | Statement | Supported |
| [OPEN "COM" Statement](O.md#open-com-statement) | Statement | Not supported in newbasic |
| [OPEN Statement](O.md#open-statement) | Statement | Supported |
| [OPTION BASE Statement](O.md#option-base-statement) | Statement | Supported |
| [OUT Statement](O.md#out-statement) | Statement | Not supported in newbasic |
| [PAINT Statement](P.md#paint-statement) | Statement | Supported |
| [PALETTE Statement](P.md#palette-statement) | Statement | Supported |
| [PALETTE USING Statement](P.md#palette-using-statement) | Statement | Not supported in newbasic |
| [PCOPY Command](P.md#pcopy-command) | Command | Supported |
| [PEEK Function](P.md#peek-function) | Function | Not supported in newbasic |
| [PEN Function](P.md#pen-function) | Function | Not supported in newbasic |
| [PEN Statement](P.md#pen-statement) | Statement | Not supported in newbasic |
| [PLAY Statement](P.md#play-statement) | Statement | Supported |
| [PLAY() Function](P.md#play-function) | Function | Not supported in newbasic |
| [PMAP Function](P.md#pmap-function) | Function | Supported |
| [POINT Function](P.md#point-function) | Function | Supported |
| [POKE Statement](P.md#poke-statement) | Statement | Not supported in newbasic |
| [POS Function](P.md#pos-function) | Function | Supported |
| [PRESET Statement](P.md#preset-statement) | Statement | Supported |
| [PRINT Statement](P.md#print-statement) | Statement | Supported |
| [PRINT USING Statement](P.md#print-using-statement) | Statement | Supported |
| [PRINT# Statement](P.md#print-statement-2) | Statement | Supported |
| [PRINT# USING Statement](P.md#print-using-statement-2) | Statement | Supported |
| [PSET Statement](P.md#pset-statement) | Statement | Supported |
| [PUT Statement](P.md#put-statement) | Statement | Supported |
| [RANDOMIZE Statement](R.md#randomize-statement) | Statement | Supported |
| [READ Statement](R.md#read-statement) | Statement | Supported |
| [REM Statement](R.md#rem-statement) | Statement | Supported |
| [RENUM Command](R.md#renum-command) | Command | Direct mode only |
| [RESET Command](R.md#reset-command) | Command | Not supported in newbasic |
| [RESTORE Statement](R.md#restore-statement) | Statement | Supported |
| [RESUME Statement](R.md#resume-statement) | Statement | Supported |
| [RETURN Statement](R.md#return-statement) | Statement | Supported |
| [RIGHT$ Function](R.md#right-function) | Function | Supported |
| [RMDIR Command](R.md#rmdir-command) | Command | Direct mode only |
| [RND Function](R.md#rnd-function) | Function | Supported |
| [RSET Statement](R.md#rset-statement) | Statement | Supported |
| [RUN Command](R.md#run-command) | Command | Supported |
| [SAVE Command](S.md#save-command) | Command | Supported |
| [SCREEN Function](S.md#screen-function) | Function | Not supported in newbasic |
| [SCREEN Statement](S.md#screen-statement) | Statement | Supported |
| [SGN Function](S.md#sgn-function) | Function | Supported |
| [SHELL Statement](S.md#shell-statement) | Statement | Supported |
| [SIN Function](S.md#sin-function) | Function | Supported |
| [SOUND Statement](S.md#sound-statement) | Statement | Supported |
| [SPACE$ Function](S.md#space-function) | Function | Supported |
| [SPC Function](S.md#spc-function) | Function | Supported in PRINT only |
| [SQR Function](S.md#sqr-function) | Function | Supported |
| [STICK Function](S.md#stick-function) | Function | Not supported in newbasic |
| [STOP Statement](S.md#stop-statement) | Statement | Supported |
| [STR$ Function](S.md#str-function) | Function | Supported |
| [STRIG Function](S.md#strig-function) | Function | Not supported in newbasic |
| [STRIG Statement](S.md#strig-statement) | Statement | Not supported in newbasic |
| [STRING$ Function](S.md#string-function) | Function | Supported |
| [SWAP Statement](S.md#swap-statement) | Statement | Supported |
| [SYSTEM Command](S.md#system-command) | Command | Direct mode only |
| [TAB Function](T.md#tab-function) | Function | Supported in PRINT only |
| [TAN Function](T.md#tan-function) | Function | Supported |
| [TIME$ Function](T.md#time-function) | Function | Supported |
| [TIMER Function](T.md#timer-function) | Function | Supported |
| [TIMER Statement](T.md#timer-statement) | Statement | Supported |
| [TROFF Command](T.md#troff-command) | Command | Supported |
| [TRON Command](T.md#tron-command) | Command | Supported |
| [UNLOCK Statement](U.md#unlock-statement) | Statement | Not supported in newbasic |
| [USR Function](U.md#usr-function) | Function | Not supported in newbasic |
| [VAL Function](V.md#val-function) | Function | Supported |
| [VARPTR Function](V.md#varptr-function) | Function | Not supported in newbasic |
| [VARPTR$ Function](V.md#varptr-function-2) | Function | Not supported in newbasic |
| [VIEW PRINT Statement](V.md#view-print-statement) | Statement | Supported |
| [VIEW Statement](V.md#view-statement) | Statement | Supported |
| [WAIT Statement](W.md#wait-statement) | Statement | Not supported in newbasic |
| [WEND Statement](W.md#wend-statement) | Statement | Supported |
| [WHILE Statement](W.md#while-statement) | Statement | Supported |
| [WIDTH Statement](W.md#width-statement) | Statement | Partial |
| [WINDOW Statement](W.md#window-statement) | Statement | Supported |
| [WRITE Statement](W.md#write-statement) | Statement | Supported |
| [WRITE# Statement](W.md#write-statement-2) | Statement | Supported |
<!-- INDEX:END -->

## Unsupported keywords at a glance

The full reasons are in each letter file. This list is for quick lookup.

<!-- UNSUPPORTED:START -->
| Entry |
|---|
| [BLOAD Command](B.md#bload-command) |
| [BSAVE Command](B.md#bsave-command) |
| [CALL Statement](C.md#call-statement) |
| [COM(n) Statement](C.md#comn-statement) |
| [DEF SEG Statement](D.md#def-seg-statement) |
| [DEF USR Statement](D.md#def-usr-statement) |
| [EDIT Command](E.md#edit-command) |
| [ENVIRON Statement](E.md#environ-statement) |
| [ENVIRON$ Function](E.md#environ-function) |
| [ERDEV Variable](E.md#erdev-variable) |
| [ERDEV$ Variable](E.md#erdev-variable-2) |
| [EXTERR Function](E.md#exterr-function) |
| [FRE Function](F.md#fre-function) |
| [INP Function](I.md#inp-function) |
| [IOCTL Statement](I.md#ioctl-statement) |
| [IOCTL$ Function](I.md#ioctl-function) |
| [LOCK Statement](L.md#lock-statement) |
| [LPOS Function](L.md#lpos-function) |
| [MOTOR Statement](M.md#motor-statement) |
| [ON COM(n) Statement](O.md#on-comn-statement) |
| [ON PEN Statement](O.md#on-pen-statement) |
| [ON PLAY(n) Statement](O.md#on-playn-statement) |
| [ON STRIG(n) Statement](O.md#on-strign-statement) |
| [OPEN "COM" Statement](O.md#open-com-statement) |
| [OUT Statement](O.md#out-statement) |
| [PALETTE USING Statement](P.md#palette-using-statement) |
| [PEEK Function](P.md#peek-function) |
| [PEN Function](P.md#pen-function) |
| [PEN Statement](P.md#pen-statement) |
| [PLAY() Function](P.md#play-function) |
| [POKE Statement](P.md#poke-statement) |
| [RESET Command](R.md#reset-command) |
| [SCREEN Function](S.md#screen-function) |
| [STICK Function](S.md#stick-function) |
| [STRIG Function](S.md#strig-function) |
| [STRIG Statement](S.md#strig-statement) |
| [UNLOCK Statement](U.md#unlock-statement) |
| [USR Function](U.md#usr-function) |
| [VARPTR Function](V.md#varptr-function) |
| [VARPTR$ Function](V.md#varptr-function-2) |
| [WAIT Statement](W.md#wait-statement) |
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
