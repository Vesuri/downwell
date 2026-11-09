"""Synthetic extraction checks; no original language payload embedded."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "shop_text", Path(__file__).resolve().parents[1] / "tools/generate-shop-text.py")
shop_text = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shop_text)


class ShopTextExtractionTest(unittest.TestCase):
    def test_languages_fallback_and_newlines(self):
        raw = '\ufeff[English]\nshopGreet0=Hello*there %\n[Other]\nshopGreet0=ä星\n'
        rows = shop_text.extract(raw.encode('utf-8'))
        self.assertEqual(len(rows), 2 * len(shop_text.KEYS))
        values = {(language, key): text for language, key, text in rows}
        self.assertEqual(values['English', 'shopGreet0'], 'Hello#there %')
        self.assertEqual(values['Other', 'shopGreet0'], 'ä星')
        self.assertEqual(values['Other', 'shopWhat1'], ' ')

    def test_speech_keys_are_shop_keys(self):
        self.assertTrue(set(shop_text.SPEECH_KEYS) <= set(shop_text.KEYS))
        self.assertEqual(len(set(shop_text.SPEECH_KEYS)), 10)

    def test_literal_preserves_utf8_and_adjacent_digits(self):
        text = 'ä星"\\012'
        escaped = shop_text.literal(text)
        body = escaped[1:-1]
        self.assertEqual(len(body), len(text.encode('utf-8')) * 4)
        recovered = bytes(int(body[i+1:i+4], 8) for i in range(0, len(body), 4))
        self.assertEqual(recovered.decode('utf-8'), text)


if __name__ == '__main__':
    unittest.main()
