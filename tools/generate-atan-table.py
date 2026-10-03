#!/usr/bin/env python3
"""Generate mathematical atan samples; no game assets are read or copied."""
from decimal import Decimal, localcontext, ROUND_HALF_EVEN
from pathlib import Path
import hashlib
import json

VERSION = "2.0.0"
ROOT = Path(__file__).resolve().parents[1]


def series(x):
    term = total = x
    square = x * x
    denominator = 1
    while abs(term) > Decimal("1e-75"):
        term *= -square
        denominator += 2
        total += term / denominator
    return total


def main():
    with localcontext() as context:
        context.prec = 90
        pi = 16 * series(Decimal(1) / 5) - 4 * series(Decimal(1) / 239)
        scale = Decimal(1 << 16)

        def quantize(value):
            scaled = value * scale
            result = scaled.to_integral_value(rounding=ROUND_HALF_EVEN)
            # 90-digit operations and the series tail are far below this
            # conservative enclosure. Refuse any uncertain integer rounding.
            if abs(abs(scaled - result) - Decimal("0.5")) < Decimal("1e-60"):
                raise ValueError("Uncertain table quantization")
            return int(result)

        samples = []
        for i in range(257):
            x = Decimal(i) / 256
            angle = series(x) if x < Decimal("0.5") else pi / 4 + series((x - 1) / (x + 1))
            samples.append(quantize(angle))
        text = "// Generated mathematical Q16 atan table; see generator manifest.\n"
        text += f"static constexpr std::uint32_t pi={quantize(pi)}u,halfPi={quantize(pi / 2)}u;\n"
        text += "static const std::uint16_t samples[257]={\n"
        text += "\n".join(",".join(f"{v}u" for v in samples[i:i+8]) + "," for i in range(0, len(samples), 8))
        text += "\n};\n"
    output = ROOT / "generated/atan-q16.inc"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text)
    manifest = {"schemaVersion": 1, "generator": {"name": "tools/generate-atan-table.py",
        "version": VERSION, "sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
        "inputs": [], "samples": len(samples), "fractionBits": 16, "decimalPrecision": 90,
        "outputSha256": hashlib.sha256(output.read_bytes()).hexdigest()}
    output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {len(samples)} Q16 samples ({len(samples)*2} bytes)")


if __name__ == "__main__":
    main()
