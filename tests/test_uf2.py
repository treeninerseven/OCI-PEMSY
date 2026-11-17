import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from bin_to_uf2 import convert


class UF2Test(unittest.TestCase):
    def test_roundtrip_and_headers(self):
        original = bytes(range(256)) * 2 + b"tail"
        uf2 = convert(original)
        self.assertEqual(len(uf2), 3 * 512)
        recovered = b""
        for i in range(3):
            block = uf2[i * 512:(i + 1) * 512]
            self.assertEqual(struct.unpack("<8I", block[:32]),
                             (0x0A324655, 0x9E5D5157, 0x2000,
                              0x10000000 + i * 256, 256, i, 3, 0xE48BFF56))
            self.assertEqual(struct.unpack("<I", block[508:])[0], 0x0AB16F30)
            recovered += block[32:288]
        self.assertEqual(recovered[:len(original)], original)
        self.assertEqual(recovered[len(original):], b"\xff" * 252)

    def test_flash_bounds(self):
        for data in (b"", bytes(2 * 1024 * 1024 - 8192 + 1)):
            with self.assertRaises(ValueError):
                convert(data)


if __name__ == "__main__":
    unittest.main()
