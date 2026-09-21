#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
INPUT="$ROOT/data/data.win"
OUTPUT="$ROOT/extracted/utmt-0.9.2.0"
EXPECTED_SHA256=c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8

[ -f "$INPUT" ] || { echo "missing input: $INPUT" >&2; exit 1; }
ACTUAL_SHA256=$(shasum -a 256 "$INPUT" | awk '{print $1}')
if [ "$ACTUAL_SHA256" != "$EXPECTED_SHA256" ]; then
    echo "Refusing to extract an unrecognized data.win" >&2
    echo "expected: $EXPECTED_SHA256" >&2
    echo "actual:   $ACTUAL_SHA256" >&2
    exit 1
fi

UTMT=$($ROOT/tools/bootstrap-utmt.sh)
mkdir -p "$OUTPUT" "$ROOT/generated"

"$UTMT" info "$INPUT" > "$OUTPUT/info.txt"
"$UTMT" dump "$INPUT" -o "$OUTPUT" -c UMT_DUMP_ALL -s

export DOWNWELL_EXPORT_ROOT="$OUTPUT"
export DOWNWELL_DATA_WIN_SHA256="$ACTUAL_SHA256"
"$UTMT" load "$INPUT" \
    -s "$ROOT/tools/utmt-export.csx" \
    -o "$ROOT/generated/utmt-roundtrip.win" \
    -f

printf 'input data.win remains %s\n' "$ACTUAL_SHA256"
