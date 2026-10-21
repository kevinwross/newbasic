# M4 notes: the tokenized `.BAS` codec

Status: **partially implemented**. This file records exactly what is done and
what must be verified before the codec can claim byte-exactness, so nobody has
to guess (a wrong guess here corrupts files silently, which is the worst
failure mode for a codec).

## Done

- **Text `.BAS` format** — `LineStore::ToTextBas` / `ParseTextBas`, byte-exact
  (`<number>\t<source>\r\n`), property-tested for round trips
  (`tests/test_bas_text.cpp`).
- **Format auto-detection** — `Repl::LoadProgramFile` inspects the first byte; a
  tokenized file is reported as *Bad file mode* with an explicit message rather
  than being parsed as text (`tests/test_files.cpp`).
- **Token table** — `lwbasic/fstok/token_table.h`: single-byte tokens `0x81`–`0xF4`,
  the `0xFD`/`0xFE`/`0xFF` two-byte groups, and the numeric/constant token bytes;
  `CodeForKeyword` / `KeywordForCode` plus six structural tests
  (`tests/test_fstok_tokens.cpp`).

## Must be verified before implementing the encoder/decoder

These are the details that need a source, not a guess:

1. **File envelope byte.** Which leading byte marks a tokenized program, and
   which marks a protected one. The auto-detection currently keys on `0x14`;
   this needs confirming (the commonly cited values are `0xFF` for a tokenized
   file and `0xFE` for a protected one, with ASCII files distinguished
   otherwise).
2. **Line record layout.** The next-line pointer's width, byte order and
   whether it is an absolute offset from the file start; the line number's
   encoding; and the record terminator (`0x00`) placement.
3. **Numeric constant encoding.** The bit layout of the `0x1D` (single), `0x1F`
   (double), `0x1C` (integer) and `0x0F` (byte) payloads. Microsoft BASIC's
   single/double are *not* IEEE 754 — the mantissa/exponent byte order differs —
   so this must come from a source rather than from the host representation.
   The project's decision to use host floating point for *arithmetic* (see
   plan.md) does not extend to the on-disk encoding, which must be exact.
4. **Protected-file decode.** The algorithm that maps a protected program to a
   loadable one (the "unprotect" transform), and what `SAVE`, `,P` should write.
5. **Exact reassembly.** Which keywords are stored as tokens versus raw text
   (operators, `TAB(`/`SPC(`, `'` for `REM`), and how embedded line-number
   references (`0x0E`) are rewritten when a program is edited.

## Provenance / licensing

The token numbers in `token_table.h` are the format's interface facts, taken
from the GW-BASIC token list at
<http://www.chebucto.ns.ca/~af380/GW-BASIC-tokens.html> and PC-BASIC's
`token.py` (GPL-3.0). No code was copied from either; the trail is recorded so
the licensing decision stays the project's to make. The local
`docs/gw-basic/` reference documents only *that* keywords are stored as tokens,
not their values.

## Suggested order

1. Source and record items 1–3, adding tests that pin the byte layouts
   (encode a known constant, compare the exact bytes).
2. Implement the **decoder** first and test it against the encoder, then against
   a known-good file if one can be produced.
3. Implement the encoder, then `SAVE BIN`/`LOAD BIN` wiring in the REPL.
4. Add protected-file support last, since it layers on the tokenized format.
