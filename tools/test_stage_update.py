import hashlib
import io
import unittest
import zipfile

from stage_update import validate_archive


def fixture(path="kplayer/kplayer", content=None):
    elf = bytearray(64)
    elf[:6] = b"\x7fELF\x02\x01"
    elf[18:20] = b"\xb7\x00"
    result = io.BytesIO()
    with zipfile.ZipFile(result, "w") as archive:
        archive.writestr(path, bytes(elf) if content is None else content)
    return result.getvalue()


class StageTests(unittest.TestCase):
    def test_valid(self):
        blob = fixture()
        self.assertEqual(len(validate_archive(blob, hashlib.sha256(blob).hexdigest())), 64)

    def test_checksum(self):
        with self.assertRaises(ValueError):
            validate_archive(fixture(), "0" * 64)

    def test_path_traversal(self):
        blob = fixture("../kplayer")
        with self.assertRaises(ValueError):
            validate_archive(blob, hashlib.sha256(blob).hexdigest())

    def test_wrong_architecture(self):
        blob = fixture(content=b"x" * 64)
        with self.assertRaises(ValueError):
            validate_archive(blob, hashlib.sha256(blob).hexdigest())


if __name__ == "__main__":
    unittest.main()
