# Windows floating-point startup evidence

`make verify-runner-fp-startup` checks the untouched executable hash and the
reviewed byte ranges. It does not execute Windows code or establish the control
register values at a gameplay event.

## Reachable precision initialization

The CRT startup call at `0x716d88` walks the initializer range
`[0x79572c,0x795754)`. Its first non-null entry is `0x716c68`; the successful
path calls `0x717e8a` at `0x716cdd`. That routine calls `0x76a6cf` with a null
output pointer, new value `0x10000`, and mask `0x30000`.

These are `_PC_53` and `_MCW_PC`, respectively. The rounding mask `_MCW_RC`
is `0x300` and is absent from this call. Selecting 53-bit precision therefore
does **not** demonstrate nearest-even rounding. See Microsoft's
[_controlfp_s constants and masked-update contract](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/controlfp-s?view=msvc-170).

The executable's translator at `0x77ee78` and control-word update routine at
`0x77eb25` are pinned separately. The translator maps the abstract precision
value to x87 precision bits; it is not valid to confuse abstract CRT rounding
bits with hardware rounding bits.

## Other reviewed control-word writes

- `0x77ee40` contains a reset helper: `FNINIT`, the precision initialization
  above, and an MXCSR setter with `0x1f80`. Its presence does not establish that
  the game reaches it before every collision or that later code preserves it.
- The math path at `0x757002` saves the x87 control word, ORs its precision
  field with `0x300`, loads it, performs arithmetic, and restores the saved
  word at `0x757072`. This is a temporary precision change, not evidence of a
  permanent gameplay rounding mode.

A linear scan also encounters instruction-looking bytes in data. Such matches
are discovery leads only; the gate includes reviewed instruction boundaries,
not a claim to enumerate all writes, indirect calls or library behavior.

## Consequence for the port

Keep the existing nearest-even arithmetic model conditional. Do not change
collision or animation rounding merely because CRT startup sets precision.
To close the runtime-mode work item, capture x87 control and MXCSR on the actual
game thread at initialization, collision/mask sampling, sprite-field updates,
and after relevant graphics/library calls. A CRT reset routine, host scalar
tests and agreement between the two ports cannot substitute for that evidence.
