# Windows choose: one word, not irandom

## Evidence

Read-only inspection of the original `data/Downwell.exe`, SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`,
resolves the `choose` name at VA `0x8139d8`. The registration at `0x53fab5`
supplies function `0x53b860` to the registration routine.

The wrapper tests argument count for **zero**, not one, at `0x53b87b`.
Every nonempty call invokes `0x438da0` at `0x53b885`, passing argument count.
That helper calls WELL at `0x438da1`, retains the unsigned result, computes
the absolute divisor and executes unsigned division at `0x438db7`; the return
value is its remainder. There is no singleton early return. The wrapper then
converts this index through floating point, adds the zero constant at
`0x7fcc88`, calls its rounding helper and clamps to count-1. For the small
lists used here those integer indices are exactly representable.

Thus `choose(a)` consumes one word, and `choose(a,b,...)` selects
`nextU32() % count`. It is **not** the two-word `irandom(count-1)` algorithm.
Identical-valued choices still consume a word.

`python3 tools/verify-runner-choose.py` pins the executable and reviewed
registration/wrapper/helper regions without committing recovered code bytes.
This hash check guards the reviewed evidence; it is not independent execution
of the original runner or a general claim about other GameMaker versions.

## Shared primitive and outstanding migration

`GameMakerRandom::chooseIndexSmall` now implements this behavior for nonempty
lists of up to 65,535 entries. The RNG unit test covers all those counts over
32 seeds, comparing selection, draw count, index and all sixteen state words.
A singleton case also pins the next word of the existing WELL test vector.

**Existing translations have not yet been migrated.** For example, the
area-one builder's room-count choice and `BuilderRewardChoice` use
`irandomSmall` where the VM calls `choose`. Other sites mix genuine `irandom`
with translated `choose`, so globally replacing `irandomSmall` is incorrect.
Audit each call against VM instructions and correct both adapters together.
Generated fixtures/checkpoints must then be rebuilt from the corrected stream,
and natural host/native routes reverified; do not retain old hashes by
artificially burning extra random words.

This finding supersedes the unresolved singleton question in area-two recovery.
It also limits previous parity evidence: matching two translations with the
same RNG mistake does not prove original-runner fidelity. Correcting existing
choices takes priority over enabling catacomb content.
