#!/usr/bin/env python3
"""Generate mathematical Q64 arctangent samples; no game assets are read or copied."""
from decimal import Decimal, localcontext, ROUND_HALF_EVEN
from pathlib import Path
import hashlib
import json

VERSION = "1.1.0"
ROOT = Path(__file__).resolve().parents[1]
COUNT = 65  # atan(k/64), k = 0 .. 64


def series(x):
    term = total = x
    square = x * x
    denominator = 1
    while abs(term) > Decimal("1e-85"):
        term *= -square
        denominator += 2
        total += term / denominator
    return total


def arctangent(x):
    # |x| <= 1; reduce above 1/2 so the series converges quickly.
    if x <= Decimal("0.5"):
        return series(x)
    return PI / 4 + series((x - 1) / (x + 1))


def main():
    global PI
    with localcontext() as context:
        context.prec = 100
        PI = 16 * series(Decimal(1) / 5) - 4 * series(Decimal(1) / 239)

        def quantize(value, bits):
            scaled = value * (Decimal(2) ** bits)
            result = scaled.to_integral_value(rounding=ROUND_HALF_EVEN)
            if abs(abs(scaled - result) - Decimal("0.5")) < Decimal("1e-60"):
                raise ValueError("Uncertain table quantization")
            return int(result)

        samples = [quantize(arctangent(Decimal(k) / 64), 64) for k in range(COUNT)]
        pi = quantize(PI, 62)
        half_pi = quantize(PI / 2, 62)
        # 1/3 in Q64, 1/5 scaled by 2^32 and 1/7 by 2^18 (16 bits).
        inverses = {"inverse3": quantize(Decimal(1) / 3, 64), "inverse5w": quantize(Decimal(1) / 5, 32),
                    "inverse7h": quantize(Decimal(1) / 7, 18)}
        assert inverses["inverse7h"] < 1 << 16

    def words(v):
        return f"0x{v:016x}ull"

    text = "// Generated mathematical Q64 arctangent table; see generator manifest.\n"
    text += f"static constexpr std::uint64_t pi={words(pi)},halfPi={words(half_pi)}; // Q62\n"
    text += "static constexpr std::uint64_t " + ",".join(
        f"{k}={words(v)}" for k, v in inverses.items()) + ";\n"
    text += f"static const std::uint64_t arctangents[{COUNT}]={{\n"
    text += "\n".join(",".join(words(v) for v in samples[i:i + 4]) + "," for i in range(0, COUNT, 4))
    text += "\n};\n"
    output = ROOT / "generated/narrow-atan-q64.inc"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text)
    manifest = {"schemaVersion": 1, "generator": {"name": "tools/generate-narrow-atan-table.py",
        "version": VERSION, "sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
        "inputs": [], "samples": COUNT, "fractionBits": 64, "decimalPrecision": 100,
        "outputSha256": hashlib.sha256(output.read_bytes()).hexdigest()}
    output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {COUNT} Q64 arctangent samples ({COUNT * 8} bytes)")


if __name__ == "__main__":
    main()
