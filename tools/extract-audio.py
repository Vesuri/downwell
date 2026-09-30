#!/usr/bin/env python3
"""Extract original Downwell audio using the hash-verified UTMT resource map."""
import hashlib
import json
import platform
import re
import struct
from pathlib import Path

VERSION = "1.0.0"
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "extracted/audio"
METADATA = ROOT / "extracted/utmt-0.9.2.0/manifest.json"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def audio_entries(data):
    def u32(offset):
        return struct.unpack_from("<I", data, offset)[0]

    if data[:4] != b"FORM" or u32(4) != len(data) - 8:
        raise ValueError("Invalid FORM container")
    pos = 8
    result = None
    while pos < len(data):
        end = pos + 8 + u32(pos + 4)
        if end > len(data):
            raise ValueError("Chunk exceeds container")
        if data[pos:pos + 4] == b"AUDO":
            if result is not None:
                raise ValueError("Duplicate AUDO chunk")
            count = u32(pos + 8)
            table_end = pos + 12 + 4 * count
            if table_end > end:
                raise ValueError("Invalid AUDO pointer table")
            result = []
            previous_end = table_end
            for index in range(count):
                offset = u32(pos + 12 + 4 * index)
                if offset < previous_end or offset + 4 > end:
                    raise ValueError("Invalid AUDO entry offset")
                size = u32(offset)
                if offset + 4 + size > end:
                    raise ValueError("Audio payload exceeds chunk")
                result.append((offset + 4, data[offset + 4:offset + 4 + size]))
                previous_end = offset + 4 + size
        pos = end
    if pos != len(data) or result is None:
        raise ValueError("Missing or malformed AUDO chunk")
    return result


def main():
    metadata_bytes = METADATA.read_bytes()
    metadata = json.loads(metadata_bytes)
    sources = {}
    containers = {}
    for group in range(7):
        relative = "data/data.win" if group == 0 else f"data/audiogroup{group}.dat"
        data = (ROOT / relative).read_bytes()
        sources[relative] = {"sha256": sha256(data), "bytes": len(data)}
        if group == 0 and sha256(data) != metadata["source"]["sha256"]:
            raise ValueError("data.win does not match the resource metadata")
        containers[group] = (relative, audio_entries(data))

    pending = []
    used = set()
    for sound in metadata["resources"]["sounds"]:
        name = sound["name"]
        if not re.fullmatch(r"[A-Za-z0-9_]+", name):
            raise ValueError(f"Unsafe sound name: {name}")
        group, audio_id = sound["groupId"], sound["audioId"]
        if audio_id < 0:
            filename = sound["file"]
            if Path(filename).name != filename:
                raise ValueError("Unsafe external audio filename")
            source = f"data/{filename}"
            payload = (ROOT / source).read_bytes()
            offset = 0
            sources[source] = {"sha256": sha256(payload), "bytes": len(payload)}
        else:
            source, entries = containers[group]
            offset, payload = entries[audio_id]
            used.add((group, audio_id))
        if payload.startswith(b"OggS"):
            extension = "ogg"
        elif payload.startswith(b"RIFF") and payload[8:12] == b"WAVE":
            extension = "wav"
        else:
            raise ValueError(f"Unknown audio format: {name}")
        category = "music" if name.startswith("bgm") else "sound-effects"
        relative = f"{category}/{sound['id']:03d}_{name}.{extension}"
        pending.append((relative, payload, {
            **sound, "output": relative, "source": source,
            "sourceOffset": offset, "bytes": len(payload), "sha256": sha256(payload)
        }))
    expected = {(group, index) for group, (_, entries) in containers.items()
                for index in range(len(entries))}
    if used != expected:
        raise ValueError("Resource map does not cover every embedded audio entry")
    output = OUTPUT.resolve()
    if not output.is_relative_to((ROOT / "extracted").resolve()) or output.is_relative_to((ROOT / "data").resolve()):
        raise ValueError("Output must be under extracted/ and outside data/")
    for relative, payload, _ in pending:
        destination = output / relative
        if not destination.resolve().is_relative_to(output):
            raise ValueError("Output symlink escapes extraction directory")
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(payload)
    manifest = {
        "schemaVersion": 1,
        "extractor": {"name": "tools/extract-audio.py", "version": VERSION,
                      "sha256": sha256(Path(__file__).read_bytes()), "python": platform.python_version()},
        "resourceMetadata": {"path": str(METADATA.relative_to(ROOT)),
                             "sha256": sha256(metadata_bytes), "extractor": metadata["extractor"]},
        "sources": sources, "audioGroups": metadata["resources"]["audioGroups"],
        "sounds": [record for _, _, record in pending]
    }
    manifest_path = output / "manifest.json"
    if manifest_path.is_symlink():
        raise ValueError("Refusing manifest symlink")
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    for source, fingerprint in sources.items():
        if sha256((ROOT / source).read_bytes()) != fingerprint["sha256"]:
            raise ValueError(f"Source changed during extraction: {source}")
    print(f"Extracted {len(pending)} original audio files to {output}")


if __name__ == "__main__":
    main()
