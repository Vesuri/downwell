# Fractional integer-range arguments

The Steam runner registers `irandom_range` at `0x53f7b2` with wrapper
`0x53c2e0`. The wrapper converts both arguments through `0x56d0c0` before
sorting them and computing the inclusive range width. Real values (type zero)
dispatch to `0x56d0f6`, then `0x715a80`.

For magnitudes below 2^31, the ordinary conversion path uses `cvttsd2si`
followed by sign extension. The AVX-capable branch also uses truncation
(`vcvttpd2qq`). Thus both -0.5 and -0.8 become integer zero, independently
of the floating-point rounding mode.

The range wrapper passes width one to `0x5410c0`. That helper calls the RNG
at `0x438dc0` twice before performing the modulo operation; equal bounds do
not bypass those draws. The result is zero plus the lower bound, also zero.

Consequently `fxTimeShard` Step's `irandom_range(-0.5,-0.8)` sets `ysp` to
zero but consumes two words. The subsequent `choose` of animation speed
consumes one more. Upward/zero vertical collisions instead multiply `ysp` by
zero and consume only the animation choice.

Currency's ordinary Step uses `irandom_range(-0.5,-1.2)` instead. Truncation
produces 0 and -1, which are sorted into [-1,0]. Its bounce therefore uses
`irandomSmall(1)-1`, consuming two words and returning either -1 or 0; it is
not a continuous draw between the fractional endpoints. An upward/zero
collision multiplies the existing speed by zero without drawing RNG.

`tools/verify-runner-integer-range.py` pins the reviewed machine-code regions
and executable hash. This is static runner evidence, not an executed Windows
encounter test. The portable Step tests independently check arithmetic and
query/RNG ordering; live world integration has separate verification requirements.
