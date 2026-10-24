# newbasic Reference Manual

This is the alphabetical reference for the newbasic dialect: its statements, functions, commands, variables, and file syntax. It follows the GW-BASIC reference style and records newbasic’s behaviour explicitly where it differs.

## How this manual is organised

Entries are grouped by the first letter of the primary keyword:

- `A.md` through `Z.md` contain the entries.
- Entries within each file are alphabetical.
- The global index below is generated from the entry headings and status lines.
- Every entry states whether newbasic supports it.
- The section before the index explains variable naming, lengths, and the dialect-wide differences from GW-BASIC.

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

## Variable names and global differences

The letter entries describe individual keywords. This section covers rules that apply across the dialect: how a variable name is formed and compared, how newbasic's naming style differs from GW-BASIC's, and the global behaviours a GW-BASIC program will notice. The status values above apply here too: **Not supported** means the keyword is recognised and deliberately refused, not that it was overlooked.

### Variable names and lengths

A variable name starts with a letter, continues with letters, digits, and periods, and may end with one type-declaration character. Names are case-insensitive: the interpreter upper-cases them for comparison. Source text keeps the case you typed and `LIST` shows that source; the debugger shows the canonical key, which includes the resolved type suffix.

Underscores are not part of a name. `ACCOUNT_TOTAL` is scanned as `ACCOUNT`, then `_`, and does not parse; write `AccountTotal` instead.

| Property | Rule |
|---|---|
| First character | A letter, `A`–`Z` or `a`–`z`. |
| Remaining characters | Letters, digits, and `.` (period). |
| Type suffix | Optional `%`, `&`, `!`, `#`, or `$`, at the end of the name. |
| Case | Ignored for lookup; names are upper-cased internally. |
| Significant characters | All of them. There is no 40-character cutoff. |
| Maximum length | No fixed limit in the name rule; the practical limit is the source line and available memory. |
| Default numeric type | Single precision, unless `DEFINT`, `DEFSNG`, `DEFDBL`, or `DEFSTR` changes the default for that initial letter. |

| Suffix | Type | Notes |
|---|---|---|
| `%` | Integer | 16-bit signed (`-32768` to `32767`). Assignment rounds, and an out-of-range value raises **Overflow**. |
| `&` | Long integer | 32-bit signed. Assignment rounds, and an out-of-range value raises **Overflow**. This suffix is a newbasic extension; GW-BASIC 3.23 does not have it. |
| `!` | Single precision | The default numeric type. Values display with 7 significant digits. |
| `#` | Double precision | Values display with 16 significant digits. |
| `$` | String | Up to 255 characters. |

The kinds are storage and display types, not separate floating-point formats. All numeric values are held as host `double`, arithmetic is host `double`, and only display formatting emulates GW-BASIC. `%` and `&` are converted to their integer ranges on assignment and display as integers; `!` and `#` differ in displayed floating-point precision.

With the default types, `A`, `a`, and `A!` are the same variable. `A%`, `A&`, `A#`, and `A$` are different. An explicit suffix beats a `DEF` default: under `DEFINT A`, `A` means `A%`, while `A!` is still a single-precision variable.

A name is reserved only when the whole scanned word is a reserved word. `TOTAL`, `FORT`, `PRINTX`, `ANDY`, `LENGTH`, and `JournalEntryCount` are names. `TO`, `PRINT`, `IF`, and `FN` are keywords. A type suffix changes the spelling, so `PRINT$` is a legal string variable even though `PRINT` is reserved; avoid such names because they read like keywords.

Array names use the same character rules. A `DEF FN` name is the `FN` prefix plus an ordinary name, so long names and type suffixes apply there too.

`FN` is also a function prefix. `FNA` is an ordinary scalar unless a `DEF FN` defines a function with that name; `FNA(...)` is then a user-function call. `DEF FNA(X)=...` and `DEF FN A(X)=...` are both accepted. With no matching definition, `FNA(...)` raises **Undefined user function** rather than naming an array.

#### Naming style: newbasic and GW-BASIC

| Topic | GW-BASIC | newbasic |
|---|---|---|
| Keyword matching | Keyword-first; a leading keyword splits the word (`FORI` → `FOR` + `I`), and run-together forms are accepted. | Whole-word only; `TOTAL`, `FORT`, and `PRINTX` are names, and keywords and operands need separation (`FOR I=1 TO 10`, `GOTO 100`). |
| Keyword inside a name | Allowed after the first character; a leading keyword is matched first. | Allowed anywhere; only the exact reserved word is unavailable. |
| Significant name length | Up to 40 characters. | Every character. |
| Type suffixes | `%`, `!`, `#`, `$`. | Those four, plus `&` for long integers. |
| Case | Case-insensitive. | Case-insensitive. |
| Function names | `DEF FNname`. | Both `DEF FNA(...)` and `DEF FN A(...)`. |

The allowed alphabet is the same as GW-BASIC's. The extensions are in how names are bounded and how words are recognised: there is no 40-character significance limit, and keyword matching is whole-word, so descriptive names are safe. In particular, names that begin with a keyword are newbasic's extension: `TOTAL` begins with `TO`, `FORT` with `FOR`, and `PRINTX` with `PRINT`, yet each is one scanned word here. Newbasic also adds the `&` long-integer suffix and accepts both spellings of `DEF FN`.

This is the style used by the samples:

```basic
10 JournalEntryCount = 0
20 PostedTotal = 0
30 FOR I = 1 TO 10
40   PostedTotal = PostedTotal + I
50 NEXT I
60 PRINT PostedTotal
```

Spacing is not optional around keywords. These are errors, not compact spellings:

```basic
10 FORI = 1 TO 10
20 GOTO100
```

If a program must also run under GW-BASIC, keep names to 40 characters. GW-BASIC ignores characters after the 40th, so two names that agree for 40 characters and differ at character 41 are two variables here and one variable there. Newbasic's own codec keeps the full spelling.

### Global differences at a glance

These differences cut across entries rather than belonging to one keyword. Follow each link for the precise behaviour and limits.

| Area | Difference |
|---|---|
| Keywords and spacing | Only exact whole words are keywords; run-together forms need spaces. This is the most common source of syntax errors when moving a program. |
| Numbers | Arithmetic is host `double`; only display formatting emulates GW-BASIC. Single precision displays 7 significant digits, double 16. |
| Random numbers | [RND](R.md#rnd-function) uses `std::mt19937`, so sequences differ from GW-BASIC but are reproducible after [RANDOMIZE](R.md#randomize-statement) with a seed. `RANDOMIZE` without an expression does not prompt. |
| Program files | [SAVE](S.md#save-command) writes plain text by default; `SAVE ,B` and `SAVE ,P` write tokenized and protected forms. [LOAD](L.md#load-command) and [RUN](R.md#run-command) detect the form. Protected and tokenized byte fidelity with Microsoft files is unproven. |
| File I/O | Sequential and random data files are supported. [BLOAD](B.md#bload-command)/[BSAVE](B.md#bsave-command), [OPEN "COM"](O.md#open-com-statement), [IOCTL](I.md#ioctl-statement), and [LOCK](L.md#lock-statement)/[UNLOCK](U.md#unlock-statement) are not. DOS commands such as [FILES](F.md#files-command), [CHDIR](C.md#chdir-command), [MKDIR](M.md#mkdir-command), [NAME](N.md#name-command), [RMDIR](R.md#rmdir-command), and [KILL](K.md#kill-command) are direct mode only. |
| Machine memory and ports | [PEEK](P.md#peek-function), [POKE](P.md#poke-statement), [INP](I.md#inp-function), [OUT](O.md#out-statement), [WAIT](W.md#wait-statement), [DEF SEG](D.md#def-seg-statement), [CALL](C.md#call-statement), [USR](U.md#usr-function), `VARPTR`/`VARPTR$`, and [FRE](F.md#fre-function) are refused. |
| Devices and events | No serial port, joystick, light pen, cassette, or note queue. [ON TIMER](O.md#on-timern-statement) works because the clock is portable. [PLAY](P.md#play-statement) and [SOUND](S.md#sound-statement) are synchronous. |
| Keyboard | The [KEY](K.md#key-statement) family and function-key traps are removed. [INKEY$](I.md#inkey-function) returns characters and arrow-key codes, but does not deliver function keys. |
| Screen and graphics | Graphics drawing is supported, including the newbasic [DRAWTEXT](D.md#drawtext-statement) canvas-text extension. [SCREEN](S.md#screen-statement) accepts 0–13 plus the 720 extension. The `SCREEN` function (text-buffer read) is absent, [WIDTH](W.md#width-statement) does not resize the physical window, and [VIEW PRINT](V.md#view-print-statement) defaults to the whole grid. |
| Program editing | [EDIT](E.md#edit-command) has no screen editor. The recommended editor is VS Code with the extension in [`editors/vscode`](../editors/vscode/README.md), which adds launch and debug; `LIST`, `DELETE`, `RENUM`, and `MERGE` remain for direct line operations. |
| Environment | [ENVIRON$](E.md#environ-function) is read-only; [ENVIRON](E.md#environ-statement) is refused so a program cannot change the host environment. |
| Host commands and paths | [SHELL](S.md#shell-statement) runs the host shell (`cmd.exe` on Windows, `$SHELL` on POSIX). DOS path separators are translated on POSIX; Linux file names remain case-sensitive. |
| Chaining and sound | [CHAIN](C.md#chain-statement) supports the basic form; `CHAIN MERGE`, `,ALL`, and [COMMON](C.md#common-statement) of arrays are refused. `PLAY` treats `MB` as `MF`, does not support `Xstring;`, and caps tones at 5 seconds. |

### Why file, memory, and device operations are absent

newbasic is a dialect interpreter, not an IBM PC emulator. It runs on the host's filesystem, memory, and devices. Where a GW-BASIC operation only makes sense against a 64K address space, segment registers, x86 I/O ports, DOS device drivers, or add-on hardware, newbasic refuses it rather than inventing a substitute. The full rationale is in the [project plan](../docs/newbasic-spec-plan.md); the host-specific differences are in [portability notes](../docs/portability.md).

#### Memory and I/O ports

There is no emulated 64K data space, no segment model, and no x86 I/O-port model. As a result:

- `PEEK`/`POKE` and `DEF SEG` have no address space to read or write.
- `VARPTR`/`VARPTR$` cannot expose a host pointer as a GW-BASIC address.
- `INP`/`OUT`/`WAIT` have no ports.
- `CALL`/`DEF USR`/`USR` would need native machine code, which would break portability and could not be made safe.
- `FRE` would report free space in a memory model that does not exist.
- `CLEAR` with the GW-BASIC memory/stack arguments is refused; the no-argument form clears variables and open files.

The machine-level decision recorded as M13 was to refuse these clearly rather than invent a 64K memory and port model whose values correspond to nothing on the host. Entries that used to be syntax errors now name the function in an **Advanced Feature** error.

#### File I/O and DOS commands

File I/O itself was not removed. [OPEN](O.md#open-statement), `CLOSE`, `PRINT#`, `WRITE#`, `INPUT#`, `LINE INPUT#`, [EOF](E.md#eof-function), `LOF`, `LOC`, [FIELD](F.md#field-statement), [LSET](L.md#lset-statement)/[RSET](R.md#rset-statement), [GET](G.md#get-statement)/[PUT](P.md#put-statement), [LOAD](L.md#load-command), [SAVE](S.md#save-command), [CHAIN](C.md#chain-statement), and [MERGE](M.md#merge-command) are implemented, with the limits noted in their entries.

What is missing falls into two groups. First, operations that depend on the PC's memory, DOS devices, or multi-user file semantics:

- `BLOAD`/`BSAVE` load and save a region of machine memory. Without an address space, their file format has nothing to describe.
- `OPEN "COM"`, `IOCTL`, and `IOCTL$` open and control serial ports and DOS device drivers; `COM(n)` traps a serial port. Newbasic has no serial device.
- `LOCK`/`UNLOCK` assume multi-user record locking. Newbasic is a single-process interpreter with no arbitration model.

Second, the DOS file and directory commands are present but restricted to direct mode:

- `FILES`, `KILL`, `NAME`, `MKDIR`, `CHDIR`, and `RMDIR` are available at the `Ok` prompt but not inside a stored program. `CHDIR` is the clearest case: a program-wide directory change would silently redirect every later relative file operation. Keeping this group at the prompt keeps ambient and destructive changes under the user's control.

#### Devices, events, and the keyboard

The trap machinery works — [ON TIMER](O.md#on-timern-statement) demonstrates it — but the event sources for the other traps do not exist. There is no serial port, joystick, light pen, cassette interface, or background note queue. `PEN`, `STICK`, `STRIG`, `ON COM`, `ON PEN`, `ON STRIG`, `MOTOR`, and `PLAY()` therefore have no underlying device or queue.

The [KEY](K.md#key-statement) family was removed for the same kind of reason: the input layer does not distinguish function keys reliably, and a partial soft-key implementation would be misleading. [INKEY$](I.md#inkey-function) remains the supported keyboard-polling function.

A stub would be worse than a refusal here. A joystick that always reports "not pressed", a COM port that never receives, or a `PLAY()` queue that never drains would let a program run and produce plausible but wrong results.

#### Editing and host environment

- `EDIT` needs GW-BASIC's screen editor. Newbasic has no such editor; edit by retyping the line number, or use `LIST`, `DELETE`, `RENUM`, and `MERGE`.
- `ENVIRON$` can read the host environment, but the `ENVIRON` statement is refused so a BASIC program cannot change the environment inherited by `SHELL` and other child processes.
- `EXTERR`, `ERDEV`/`ERDEV$`, `RESET`, `LPOS`, and the `SCREEN` function depend on DOS, device-driver, printer, or text-buffer state that newbasic does not expose. Each entry states its exact status.

Newbasic deliberately does not include a built-in screen editor. A line-numbered program does not need a line editor when a modern source editor is available: VS Code provides syntax highlighting, search, undo, source control, and — through the extension in [`editors/vscode`](../editors/vscode/README.md) — launch and debug through the Debug Adapter Protocol (breakpoints, stepping, variables, call frames, and optional reverse execution), with the program's screen shown in a separate window by default. That is a much better programming experience than GW-BASIC's `EDIT`, and it is the recommended way to work on newbasic programs. The extension is a thin client for `newbasic-dap`, not a second interpreter; see the [DAP debugger](../docs/m5-dap-debugger.md) for the supported subset.

#### Why refuse instead of ignoring

A silent no-op or a fabricated value is more dangerous than an error. `PEEK(0)` returning zero, `INP(1)` returning zero, or `ON COM` never firing would let a program continue with wrong data. Refusing at the point of use makes the porting work visible. Some refusals are **Advanced Feature**, some are **Illegal direct**, and some are syntax errors; the individual entry gives the exact behaviour.

### Porting checklist

- Add spaces around keywords and operands: `FOR I = 1 TO 10`, not `FORI=1TO10`.
- Keep names to 40 characters if the program must also run under GW-BASIC.
- Replace `PEEK`/`POKE`/`VARPTR` with arrays and strings, and `CALL`/`USR` with `GOSUB`/`RETURN`. Use `OPEN`, `GET`, and `PUT` for data files.
- Replace `INP` keyboard polling with `INKEY$`. The `KEY` family has no equivalent.
- Remove COM, joystick, light-pen, and cassette code; there is no replacement.
- Use `ON TIMER` for timer events, and [INKEY$](I.md#inkey-function) for keyboard polling.
- Expect `SAVE` to write text by default. Use `SAVE ,B` when a tokenized file is required.
- Do not expect `RND` sequences or floating-point output to match GW-BASIC exactly.
- Check host shell syntax and path separators when moving between Windows and Linux.

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
| [DRAWTEXT Statement](D.md#drawtext-statement) | Statement | Supported |
| [EDIT Command](E.md#edit-command) | Command | Not supported in newbasic |
| [END Statement](E.md#end-statement) | Statement | Supported |
| [ENVIRON Statement](E.md#environ-statement) | Statement | Not supported in newbasic |
| [ENVIRON$ Function](E.md#environ-function) | Function | Supported |
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
| [KEY Statement](K.md#key-statement) | Statement | Not supported in newbasic |
| [KEY(n) Statement](K.md#keyn-statement) | Statement | Not supported in newbasic |
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
| [MOUSE Function](M.md#mouse-function) | Function | Supported (newbasic extension) |
| [MOUSEB Function](M.md#mouseb-function) | Function | Supported (newbasic extension) |
| [MOUSEBUTTONS Function](M.md#mousebuttons-function) | Function | Supported (newbasic extension) |
| [MOUSEPOSX Function](M.md#mouseposx-function) | Function | Supported (newbasic extension) |
| [MOUSEPOSY Function](M.md#mouseposy-function) | Function | Supported (newbasic extension) |
| [MOUSEWHEEL Function](M.md#mousewheel-function) | Function | Supported (newbasic extension) |
| [MOUSEX Function](M.md#mousex-function) | Function | Supported (newbasic extension) |
| [MOUSEY Function](M.md#mousey-function) | Function | Supported (newbasic extension) |
| [NAME Command](N.md#name-command) | Command | Direct mode only |
| [NEW Command](N.md#new-command) | Command | Direct mode only |
| [NEXT Statement](N.md#next-statement) | Statement | Supported |
| [OCT$ Function](O.md#oct-function) | Function | Supported |
| [ON COM(n) Statement](O.md#on-comn-statement) | Statement | Not supported in newbasic |
| [ON ERROR GOTO Statement](O.md#on-error-goto-statement) | Statement | Supported |
| [ON expression GOSUB Statement](O.md#on-expression-gosub-statement) | Statement | Supported |
| [ON expression GOTO Statement](O.md#on-expression-goto-statement) | Statement | Supported |
| [ON KEY(n) Statement](O.md#on-keyn-statement) | Statement | Not supported in newbasic |
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
| [PALETTE USING Statement](P.md#palette-using-statement) | Statement | Supported |
| [PCOPY Command](P.md#pcopy-command) | Command | Supported |
| [PEEK Function](P.md#peek-function) | Function | Not supported in newbasic |
| [PEN Function](P.md#pen-function) | Function | Not supported in newbasic |
| [PEN Statement](P.md#pen-statement) | Statement | Not supported in newbasic |
| [PLAY Statement](P.md#play-statement) | Statement | Supported |
| [PLAY() Function](P.md#play-function) | Function | Not supported in newbasic |
| [PMAP Function](P.md#pmap-function) | Function | Supported |
| [POINT Function](P.md#point-function) | Function | Supported |
| [POKE Statement](P.md#poke-statement) | Statement | Not supported in newbasic |
| [POP VIEW Statement](P.md#pop-view-statement) | Statement | Supported (newbasic extension) |
| [POS Function](P.md#pos-function) | Function | Supported |
| [PRESET Statement](P.md#preset-statement) | Statement | Supported |
| [PRINT Statement](P.md#print-statement) | Statement | Supported |
| [PRINT USING Statement](P.md#print-using-statement) | Statement | Supported |
| [PRINT# Statement](P.md#print-statement-2) | Statement | Supported |
| [PRINT# USING Statement](P.md#print-using-statement-2) | Statement | Supported |
| [PSET Statement](P.md#pset-statement) | Statement | Supported |
| [PUSH VIEW Statement](P.md#push-view-statement) | Statement | Supported (newbasic extension) |
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
| [SCREENH Function](S.md#screenh-function) | Function | Supported (newbasic extension) |
| [SCREENW Function](S.md#screenw-function) | Function | Supported (newbasic extension) |
| [SELECT CASE Statement](S.md#select-case-statement) | Statement | Supported (newbasic extension) |
| [SGN Function](S.md#sgn-function) | Function | Supported |
| [SHELL Statement](S.md#shell-statement) | Statement | Supported |
| [SIN Function](S.md#sin-function) | Function | Supported |
| [SLEEP Statement](S.md#sleep-statement) | Statement | Supported (newbasic extension) |
| [SOUND Statement](S.md#sound-statement) | Statement | Supported |
| [SPACE$ Function](S.md#space-function) | Function | Supported |
| [SPC Function](S.md#spc-function) | Function | Supported in PRINT only |
| [SQR Function](S.md#sqr-function) | Function | Supported |
| [STACK Statement](S.md#stack-statement) | Statement | Supported |
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
| [TEXTHEIGHT Function](T.md#textheight-function) | Function | Supported (newbasic extension) |
| [TEXTWIDTH Function](T.md#textwidth-function) | Function | Supported (newbasic extension) |
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
| [VIEWX1 Function](V.md#viewx1-function) | Function | Supported (newbasic extension) |
| [VIEWX2 Function](V.md#viewx2-function) | Function | Supported (newbasic extension) |
| [VIEWY1 Function](V.md#viewy1-function) | Function | Supported (newbasic extension) |
| [VIEWY2 Function](V.md#viewy2-function) | Function | Supported (newbasic extension) |
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
| [ERDEV Variable](E.md#erdev-variable) |
| [ERDEV$ Variable](E.md#erdev-variable-2) |
| [EXTERR Function](E.md#exterr-function) |
| [FRE Function](F.md#fre-function) |
| [INP Function](I.md#inp-function) |
| [IOCTL Statement](I.md#ioctl-statement) |
| [IOCTL$ Function](I.md#ioctl-function) |
| [KEY Statement](K.md#key-statement) |
| [KEY(n) Statement](K.md#keyn-statement) |
| [LOCK Statement](L.md#lock-statement) |
| [LPOS Function](L.md#lpos-function) |
| [MOTOR Statement](M.md#motor-statement) |
| [ON COM(n) Statement](O.md#on-comn-statement) |
| [ON KEY(n) Statement](O.md#on-keyn-statement) |
| [ON PEN Statement](O.md#on-pen-statement) |
| [ON PLAY(n) Statement](O.md#on-playn-statement) |
| [ON STRIG(n) Statement](O.md#on-strign-statement) |
| [OPEN "COM" Statement](O.md#open-com-statement) |
| [OUT Statement](O.md#out-statement) |
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
- [VS Code debugger extension](../editors/vscode/README.md)
- [Build instructions for Windows](../buildonwindows.md)
