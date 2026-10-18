#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("wind", ROOT / "tools/generate-amiga-wind-bank.py")
wind = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wind)


class WindTests(unittest.TestCase):
    def test_exact_admission(self):
        samples = {i: b"\x80\0\x7f" for i in wind.IDS}
        packed = wind.pack(samples)
        self.assertEqual(wind.HEADER.unpack_from(packed), (b"DWWA", 1, 7, 22030, 161, len(packed)))
        for index, resource in enumerate(wind.IDS):
            key, offset, size = wind.ENTRY.unpack_from(packed, 20 + index * 12)
            self.assertEqual((key, offset, size), (resource, 104 + index * 4, 3))
            self.assertEqual(packed[offset:offset+4], b"\x80\0\x7f\0")
        for invalid in ({}, {**samples, 348: b"x"}, {**samples, 349: b""}):
            with self.assertRaises(ValueError):
                wind.pack(invalid)
        with self.assertRaises(ValueError):
            wind.pack({i: bytes(wind.BUDGET // 7) for i in wind.IDS})

    def test_decode_signed_pcm_without_normalisation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "amplitudes.wav"
            # At the target rate no resampling is necessary. Distinct quarter
            # amplitudes detect unsigned PCM, normalisation and a gain bake.
            source = [-16384] * 128 + [0] * 128 + [8192] * 128
            with wave.open(str(path), "wb") as output:
                output.setnchannels(1)
                output.setsampwidth(2)
                output.setframerate(wind.RATE)
                output.writeframes(struct.pack("<384h", *source))
            actual = wind.decode(path)
            self.assertEqual(actual, b"\xc0" * 128 + b"\0" * 128 + b"\x20" * 128)
            self.assertEqual(actual, wind.decode(path))


if __name__ == "__main__":
    unittest.main()
