#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
INPUT="$ROOT/data/data.win"
OUTPUT="$ROOT/extracted/utmt-0.9.2.0"
MANIFEST="$OUTPUT/manifest.json"
ROUNDTRIP="$ROOT/generated/utmt-roundtrip.win"
EXPECTED_SHA256=c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8

for path in "$INPUT" "$MANIFEST" "$OUTPUT/assembly-status.json" "$ROUNDTRIP"; do
    [ -e "$path" ] || { echo "missing extraction output: $path" >&2; exit 1; }
done

INPUT_SHA256=$(shasum -a 256 "$INPUT" | awk '{print $1}')
ROUNDTRIP_SHA256=$(shasum -a 256 "$ROUNDTRIP" | awk '{print $1}')
[ "$INPUT_SHA256" = "$EXPECTED_SHA256" ] || {
    echo "unexpected input hash: $INPUT_SHA256" >&2
    exit 1
}
[ "$ROUNDTRIP_SHA256" = "$INPUT_SHA256" ] || {
    echo "UndertaleModTool round trip differs from input" >&2
    exit 1
}
cmp -s "$INPUT" "$ROUNDTRIP" || {
    echo "round-trip bytes differ despite matching digest" >&2
    exit 1
}

jq -e --arg hash "$INPUT_SHA256" '
    . as $root |
    .schemaVersion == 3 and
    .source.sha256 == $hash and
    .source.immutable == true and
    .gameMaker.bytecodeVersion == 17 and
    (.gameMaker.lastObjectInstanceId | type == "number") and
    (.gameMaker.lastTileInstanceId | type == "number") and
    .gameMaker.isYyc == false and
    (.gameMaker.roomOrder | length) == .counts.rooms and
    .counts.objects == (.resources.objects | length) and
    .counts.rooms == (.resources.rooms | length) and
    .counts.sounds == (.resources.sounds | length) and
    .counts.audioGroups == (.resources.audioGroups | length) and
    .counts.sprites == (.resources.sprites | length) and
    .counts.fonts == (.resources.fonts | length) and
    .counts.scripts == (.resources.scripts | length) and
    .counts.texturePageItems == (.resources.texturePageItems | length) and
    .counts.embeddedTextures == (.resources.embeddedTextures | length) and
    .counts.constants == (.resources.constants | length) and
    .counts.codeEntries == (.resources.code | length) and
    .counts.variables == (.resources.variables | length) and
    .counts.functions == (.resources.functions | length) and
    ([.resources.objects[] | .id] == [range(0; .counts.objects)]) and
    ([.resources.rooms[] | .id] == [range(0; .counts.rooms)]) and
    ([.resources.code[] | .id] == [range(0; .counts.codeEntries)]) and
    (all(.gameMaker.roomOrder[];
         .roomId >= 0 and .roomId < $root.counts.rooms)) and
    (all(.resources.sprites[].frames[];
         .texturePageItemId >= -1 and .texturePageItemId < $root.counts.texturePageItems)) and
    (all(.resources.texturePageItems[];
         .texturePageId >= -1 and .texturePageId < $root.counts.embeddedTextures)) and
    (all(.resources.fonts[];
         .texturePageItemId >= -1 and .texturePageItemId < $root.counts.texturePageItems)) and
    (all(.resources.scripts[];
         .codeId >= -1 and .codeId < $root.counts.codeEntries)) and
    (all(.resources.objects[]; .parentId >= -1 and .parentId < $root.counts.objects)) and
    (all(.resources.objects[].events[].actions[];
         .codeId >= -1 and .codeId < $root.counts.codeEntries)) and
    (all(.resources.rooms[].instances[];
         .objectId >= -1 and .objectId < $root.counts.objects)) and
    (all(.resources.rooms[];
         ([.instances[].instanceId] | sort) ==
         ([.instanceCreationOrder[].instanceId] | sort))) and
    (all(.resources.rooms[];
         ([.instances[].instanceId] | sort) ==
         ([.layers[].instanceIds[]] | sort)))
' "$MANIFEST" >/dev/null

ASSEMBLY_EXPECTED=$(jq '[.resources.code[] | select(.parentId == -1)] | length' "$MANIFEST")
ASSEMBLY_ACTUAL=$(find "$OUTPUT/Assembly" -type f -name '*.asm' | wc -l | tr -d ' ')
GML_ACTUAL=$(find "$OUTPUT/CodeEntries" -type f -name '*.gml' | wc -l | tr -d ' ')
ASSEMBLY_FAILURES=$(jq '[.[] | select(.success == false)] | length' "$OUTPUT/assembly-status.json")
GML_FAILURES=$( (rg -l 'DECOMPILER FAILED' "$OUTPUT/CodeEntries" || true) | wc -l | tr -d ' ')

[ "$ASSEMBLY_ACTUAL" = "$ASSEMBLY_EXPECTED" ] || {
    echo "assembly count mismatch: expected $ASSEMBLY_EXPECTED, got $ASSEMBLY_ACTUAL" >&2
    exit 1
}
[ "$GML_ACTUAL" = "$ASSEMBLY_EXPECTED" ] || {
    echo "GML count mismatch: expected $ASSEMBLY_EXPECTED, got $GML_ACTUAL" >&2
    exit 1
}
[ "$ASSEMBLY_FAILURES" = 0 ] || { echo "$ASSEMBLY_FAILURES assembly failures" >&2; exit 1; }
[ "$GML_FAILURES" = 0 ] || { echo "$GML_FAILURES decompiler failures" >&2; exit 1; }

OBJECT_EVENTS=$(jq '[.resources.objects[].events[]] | length' "$MANIFEST")
ROOM_INSTANCES=$(jq '[.resources.rooms[].instances[]] | length' "$MANIFEST")
ROOM_LAYERS=$(jq '[.resources.rooms[].layers[]] | length' "$MANIFEST")

echo "Extraction verified"
echo "  byte-identical round trip: $INPUT_SHA256"
echo "  root code entries:         $ASSEMBLY_EXPECTED"
echo "  object events:             $OBJECT_EVENTS"
echo "  room instances:            $ROOM_INSTANCES"
echo "  room layers:               $ROOM_LAYERS"
