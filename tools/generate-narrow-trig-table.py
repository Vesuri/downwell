#!/usr/bin/env python3
"""Generate mathematical Q64 sine/cosine samples; no game assets are read or copied."""
from decimal import Decimal, localcontext, ROUND_HALF_EVEN
from pathlib import Path
import hashlib
import json

VERSION = "1.1.0"
ROOT = Path(__file__).resolve().parents[1]
STEP_BITS = 6      # samples every 2^-6 radians
COUNT = 51         # 0 .. 50/64, covering [0, pi/4]


def atan_series(x):
    term = total = x
    square = x * x
    denominator = 1
    while abs(term) > Decimal("1e-85"):
        term *= -square
        denominator += 2
        total += term / denominator
    return total


def sin_cos(x):
    s = term = x
    c = Decimal(1)
    cterm = Decimal(1)
    n = 1
    while abs(term) > Decimal("1e-85") or abs(cterm) > Decimal("1e-85"):
        cterm *= -x * x / ((n) * (n + 1))
        c += cterm
        term *= -x * x / ((n + 1) * (n + 2))
        s += term
        n += 2
    return s, c


def main():
    with localcontext() as context:
        context.prec = 100
        pi = 16 * atan_series(Decimal(1) / 5) - 4 * atan_series(Decimal(1) / 239)

        def quantize(value, bits):
            scaled = value * (Decimal(2) ** bits)
            result = scaled.to_integral_value(rounding=ROUND_HALF_EVEN)
            if abs(abs(scaled - result) - Decimal("0.5")) < Decimal("1e-60"):
                raise ValueError("Uncertain table quantization")
            return int(result)

        sines, cosines = [], []
        for i in range(COUNT):
            s, c = sin_cos(Decimal(i) / (1 << STEP_BITS))
            sines.append(quantize(s, 64))
            # cos 0 = 1 is one unit above Q64; the evaluation's error bound
            # covers the one-unit shortfall.
            cosines.append(min(quantize(c, 64), (1 << 64) - 1))
        half_pi = quantize(pi / 2, 60)
        quarter_pi = quantize(pi / 4, 60)
        # 1/6 in Q64; the smaller terms' reciprocals in 32 and 16 bits,
        # scaled by 2^32, 2^32, 2^25 and 2^28.
        reciprocals = {"inverse6": quantize(Decimal(1) / 6, 64), "inverse24w": quantize(Decimal(1) / 24, 32),
                       "inverse120w": quantize(Decimal(1) / 120, 32), "inverse720h": quantize(Decimal(1) / 720, 25),
                       "inverse5040h": quantize(Decimal(1) / 5040, 28)}
        assert all(v < 1 << 16 for k, v in reciprocals.items() if k.endswith("h"))

    def words(v):
        return f"0x{v:016x}ull"

    text = "// Generated mathematical Q64 sine/cosine table; see generator manifest.\n"
    text += f"static constexpr std::uint64_t halfPi={words(half_pi)},quarterPi={words(quarter_pi)}; // Q60\n"
    text += "static constexpr std::uint64_t " + ",".join(
        f"{k}={words(v)}" for k, v in reciprocals.items()) + ";\n"
    for name, values in (("sines", sines), ("cosines", cosines)):
        text += f"static const std::uint64_t {name}[{COUNT}]={{\n"
        text += "\n".join(",".join(words(v) for v in values[i:i + 4]) + "," for i in range(0, COUNT, 4))
        text += "\n};\n"
    output = ROOT / "generated/narrow-trig-q64.inc"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text)
    manifest = {"schemaVersion": 1, "generator": {"name": "tools/generate-narrow-trig-table.py",
        "version": VERSION, "sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
        "inputs": [], "samples": COUNT, "stepBits": STEP_BITS, "fractionBits": 64, "decimalPrecision": 100,
        "outputSha256": hashlib.sha256(output.read_bytes()).hexdigest()}
    output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {COUNT} Q64 sine/cosine samples ({COUNT * 16} bytes)")


if __name__ == "__main__":
    main()
