#!/usr/bin/env python3
"""Check sprite playback facts, not complete animation path equivalence."""
from pathlib import Path
from hashlib import sha256
import json
import struct
root = Path(__file__).resolve().parents[1]
source = (root / "data/data.win").read_bytes()
digest = sha256(source).hexdigest()
assert digest == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert source[8:12] == b"GEN8"
assert struct.unpack_from("<I", source, 16 + 0x2c)[0] == 2
metadata = json.loads((root / "generated/animation-metadata.json").read_text())
assert metadata["sourceSha256"] == digest and metadata["version"] == "0.9.2.0"
assert metadata["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
sprites = metadata["sprites"]
assert len(sprites) == 741
assert [s["id"] for s in sprites] == list(range(741))
for index, name, frames in (
    (84,"sprSmoke",9),
    (91,"sprPoisonPotPieces",6),
    (96,"sprFlashBall",3),
    (205,"sprEnmSkeletonHit",1), (206,"sprSkeletonDead",1),
    (207,"sprEnmSkeletonThrow",8), (208,"sprEnmSkeletonIdle",6),
    (460,"sprSkeletonBone",4),
):
    s = sprites[index]
    assert (s["name"],s["frames"],s["spriteType"]) == (name,frames,"Normal")
    assert s["playbackSpeed"] == 1 and s["playbackSpeedType"] == "FramesPerGameFrame"

# Audit every original track, not only the first actors using the evaluator.
for s in sprites:
    frames = s["frames"]
    assert s["spriteType"] == "Normal"
    assert s["hasSequence"]  # Do not confuse normal textures with no sequence.
    sequence = s["sequence"]
    assert sequence["Playback"] == 1 and sequence["Length"] == frames
    assert sequence["PlaybackSpeed"] == s["playbackSpeed"]
    assert sequence["PlaybackSpeedType"] == (0 if s["playbackSpeedType"] == "FramesPerSecond" else 1)
    assert not sequence["BroadcastMessages"] and not sequence["Moments"]
    tracks = sequence["Tracks"]
    assert len(tracks) == 1 and tracks[0]["ModelName"]["Content"] == "GMSpriteFramesTrack"
    assert not tracks[0]["Tracks"]
    keys = tracks[0]["Keyframes"]["List"]
    assert len(keys) == frames
    for frame, key in enumerate(keys):
        assert key["Key"] == frame and key["Length"] == 1
        assert not key["Stretch"] and not key["Disabled"]
        assert key["Channels"] == [{"Channel": 0, "Value": {"Value": frame}}]
assert [s["id"] for s in sprites if s["playbackSpeedType"] == "FramesPerSecond"] == [399,626,738,739,740]
assert [s["id"] for s in sprites if not s["frames"]] == [125,159,582]
assert sum(s["frames"] for s in sprites) == 3152
print("741 original unit sequence tracks and 3152 keys verified; three empty sprites and five per-second playback rates retained")
