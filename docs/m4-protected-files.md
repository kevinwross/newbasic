> **2026-10-24:** Tokenized and protected program-file support was removed. `SAVE`/`LOAD`/`MERGE` are plain-text only. Historical notes below are retained for provenance.

# M4 notes: protected `.BAS` files (the `0xFE` format)

Collected in round 49 from PC-BASIC's `protect.py` (GPL-3.0), which cites Paul
Kocher's reverse engineering published in *The Cryptogram* computer supplement
#19, American Cryptogram Association, Summer 1994. As with the token table, what
is recorded here are the algorithm's interface facts (two constant keys and the
byte transformation), no code is copied, and the provenance is named so the
licensing call stays the project's to make.

## The keys

```
key1 (13 bytes) = A9 84 8D CD 75 83 43 63 24 83 19 F7 9A
key2 (11 bytes) = 1E 1D C4 77 26 97 E0 74 59 88 7C
```

13 and 11 are coprime, so the combined period is 143 and the two keys walk out
of phase with each other, which is what makes the scheme non-trivial.

## Kocher's algorithm

Working modulo 256, with `i` starting at 0 and stepping `i = (i + 1) % 143`:

```c
/* decrypt one byte */
c  = (byte - (11 - i % 11)) % 256;
c ^= key1[i % 13];
c ^= key2[i % 11];
c  = (c + (13 - i % 13)) % 256;

/* encrypt one byte (the inverse: the two keys and the two offsets swap roles) */
c  = (byte - (13 - i % 13)) % 256;
c ^= key1[i % 13];
c ^= key2[i % 11];
c  = (c + (11 - i % 11)) % 256;
```

## File shape

A protected file is the tokenized format with the envelope byte changed from
`0xFF` to `0xFE` and the remainder encrypted; the trailing `0x1A` stays as the
EOF marker (`protect.py`'s reader deliberately drops the last byte before
transforming, and the decryptor stops one byte early for the same reason).

## Intended implementation

`fstok/protect.h`:

- `UnprotectTokenized(bytes)`: decrypt `bytes[1..]` with `i` from 0, set
  `[0] = 0xFF`, return a tokenized program that `DecodeTokenized` accepts.
- `ProtectTokenized(tokenized)`: the inverse, setting `[0] = 0xFE`.
- Wire into `Repl::LoadProgramFile` (replacing the current "protected not
  supported" message) and add a `SAVE "file",P` modifier next to `,B`.

**Open fidelity question to be honest about:** a round-trip test proves only
that our protect/unprotect pair are mutual inverses. It does *not* prove our
byte-level agreement with Microsoft's own files, because the index offset and
the exact treatment of the envelope and the trailing `0x1A` are inferred from
the description above. If a real protected GW-BASIC file becomes available, a
byte-comparison test against it is the way to settle that; until then the
implementation should say so rather than imply compatibility.
