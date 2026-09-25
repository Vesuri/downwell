#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
expected=c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
actual=$(shasum -a 256 data/data.win | awk '{print $1}')
[[ "$actual" == "$expected" ]] || { echo "Unrecognized data.win" >&2; exit 1; }
mkdir -p generated
export DOWNWELL_DATA_WIN_SHA256="$actual"
export DOWNWELL_SHADER_OUTPUT=generated/default-shader.json
utmt=$(tools/bootstrap-utmt.sh)
"$utmt" load data/data.win -s tools/export-default-shader.csx
[[ "$(shasum -a 256 data/data.win | awk '{print $1}')" == "$actual" ]]
