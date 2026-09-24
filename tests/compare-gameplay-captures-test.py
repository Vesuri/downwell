#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import unittest
from PIL import Image

spec = importlib.util.spec_from_file_location(
    "capture_compare", Path(__file__).resolve().parents[1] / "tools/compare-gameplay-captures.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CaptureComparisonTest(unittest.TestCase):
    def test_crop_and_exact_difference(self):
        host = Image.new("RGB", (380, 284))
        native = Image.new("RGB", (352, 284))
        host.putpixel((0, 0), (255, 0, 0))  # outside native DMA window
        host.putpixel((16, 0), (255, 255, 255))
        native.putpixel((0, 0), (255, 255, 255))
        host = host.resize((760, 568), Image.Resampling.NEAREST)
        self.assertEqual(module.compare(host, native), 0)
        native.putpixel((351, 283), (255, 0, 0))
        self.assertEqual(module.compare(host, native), 1)

    def test_filtered_capture_rejected(self):
        host = Image.new("RGB", (760, 568))
        host.putpixel((33, 1), (255, 0, 0))
        with self.assertRaisesRegex(ValueError, "nonuniform"):
            module.compare(host, Image.new("RGB", (352, 284)))

    def test_wrong_dimensions_rejected(self):
        with self.assertRaises(ValueError):
            module.compare(Image.new("RGB", (759, 568)), Image.new("RGB", (352, 284)))
        with self.assertRaises(ValueError):
            module.compare(Image.new("RGB", (380, 284)), Image.new("RGB", (380, 284)))


if __name__ == "__main__":
    unittest.main()
