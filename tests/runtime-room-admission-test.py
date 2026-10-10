#!/usr/bin/env python3
"""Unrelated room growth must not grow the shipping room payload."""
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("rooms", ROOT / "tools/generate-room-pack.py")
rooms = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rooms)


def catalogue(unrelated_instances):
    ids = sorted(rooms.RUNTIME_ROOMS) + [999]
    records = []
    for ident in ids:
        records.append(rooms.ROOM.pack(ident, 160, 284, 60, -1, 0, 0,
                                      unrelated_instances if ident == 999 else 0, 0, 0)
                       + bytes(8 * rooms.VIEW.size))
    header = rooms.HEADER.pack(rooms.MAGIC, 4, 40, 3, len(ids), unrelated_instances,
                               0, unrelated_instances, 0, 105187)
    return header + b"".join(records) + bytes(unrelated_instances * 52)


class Admission(unittest.TestCase):
    def test_unrelated_growth(self):
        baseline = rooms.runtime_projection(catalogue(0))
        for count in (1, 100, 10000):
            self.assertEqual(baseline, rooms.runtime_projection(catalogue(count)))

    def test_missing_room_fails(self):
        data = bytearray(catalogue(0))
        data[40:44] = (1000).to_bytes(4, "little")
        with self.assertRaisesRegex(ValueError, "missing admitted"):
            rooms.runtime_projection(data)

    def test_budget_fails_closed(self):
        with patch.object(rooms, "RUNTIME_BUDGET", 1):
            with self.assertRaisesRegex(ValueError, "budget exceeded"):
                rooms.runtime_projection(catalogue(0))

    def test_source_payloads(self):
        full = (ROOT / "generated/downwell-rooms.dwrm").read_bytes()
        projected = rooms.runtime_projection(full)
        self.assertLessEqual(len(projected), rooms.RUNTIME_BUDGET)

        def decode(data):
            h = rooms.HEADER.unpack_from(data)
            stride = rooms.ROOM.size + 8 * rooms.VIEW.size
            instances = h[2] + h[4] * stride
            layers = instances + h[5] * rooms.INSTANCE.size
            members = layers + h[6] * rooms.LAYER.size
            decoded = {}
            for i in range(h[4]):
                at = h[2] + i * stride
                r = rooms.ROOM.unpack_from(data, at)
                ls = []
                for j in range(r[8], r[8] + r[9]):
                    layer = rooms.LAYER.unpack_from(data, layers + j * rooms.LAYER.size)
                    ls.append((layer[:3] + layer[4:], data[members + layer[3]*4:
                                                          members + (layer[3]+layer[4])*4]))
                decoded[r[0]] = (r[:6], data[at+40:at+stride], ls,
                    data[instances+r[6]*48:instances+(r[6]+r[7])*48])
            return decoded

        reference, actual = decode(full), decode(projected)
        self.assertEqual(set(actual), rooms.RUNTIME_ROOMS)
        for ident in actual:
            self.assertEqual(reference[ident], actual[ident])
        self.assertEqual(full[28:40], projected[28:40])


if __name__ == "__main__":
    unittest.main()
