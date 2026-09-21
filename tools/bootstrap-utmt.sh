#!/usr/bin/env bash
set -euo pipefail

# Pinned, self-contained Apple-silicon UndertaleModTool CLI. The release digest
# comes from the official GitHub release metadata for tag 0.9.2.0.
VERSION=0.9.2.0
ARCHIVE="UTMT_CLI_v${VERSION}-macOS.zip"
URL="https://github.com/UnderminersTeam/UndertaleModTool/releases/download/${VERSION}/${ARCHIVE}"
SHA256=03042fa77f93e9b7fcb06592b77fb8369876e6d6fff7680a63dc0fa2df5444ef

ROOT=$(cd "$(dirname "$0")/.." && pwd)
VENDOR="$ROOT/tools/vendor/undertale-mod-tool-$VERSION"
ZIP="$ROOT/tools/vendor/$ARCHIVE"

mkdir -p "$ROOT/tools/vendor"

if [ ! -f "$ZIP" ]; then
    echo "Downloading UndertaleModTool CLI $VERSION..." >&2
    curl -fL "$URL" -o "$ZIP"
fi

ACTUAL=$(shasum -a 256 "$ZIP" | awk '{print $1}')
if [ "$ACTUAL" != "$SHA256" ]; then
    echo "UndertaleModTool archive hash mismatch" >&2
    echo "expected: $SHA256" >&2
    echo "actual:   $ACTUAL" >&2
    exit 1
fi

if [ ! -x "$VENDOR/UndertaleModCli" ]; then
    rm -rf "$VENDOR"
    mkdir -p "$VENDOR"
    unzip -q "$ZIP" -d "$VENDOR"
    chmod +x "$VENDOR/UndertaleModCli"
fi

echo "$VENDOR/UndertaleModCli"
