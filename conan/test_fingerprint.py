"""Run with: python3 conan/test_fingerprint.py"""

import pathlib
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from fingerprint import fingerprint  # noqa: E402

# The same files and value as dependency-builder's tests/test_fingerprint.py::test_shared_fixture_value:
# the archives on ghcr.io are tagged with that copy's fingerprint, so this one must agree.
FILES = {
    "conanfile.py": b"conanfile\n",
    "conan.lock": b'{"version": "0.5"}\n',
    "conan/recipes-commit": b"96c4f46\n",
    "conan/profiles/linux-x86_64": b"[settings]\nos=Linux\n",
}
EXPECTED = "1c0aa38d5b43d726d6900ca346ef2c2b005fa44b7b1f1d7c8c96d1945d919abb"


class FingerprintTest(unittest.TestCase):
    def test_shared_fixture_value(self):
        self.assertEqual(fingerprint("linux-x86_64", FILES), EXPECTED)

    def test_command_line_reads_the_checkout(self):
        with tempfile.TemporaryDirectory() as root:
            for path, content in FILES.items():
                (pathlib.Path(root) / path).parent.mkdir(parents=True, exist_ok=True)
                (pathlib.Path(root) / path).write_bytes(content)
            out = subprocess.run([sys.executable, str(HERE / "fingerprint.py"), "--profile", "linux-x86_64",
                                  "--root", root], check=True, capture_output=True, text=True).stdout
        self.assertEqual(out, EXPECTED + "\n")

    def test_missing_input_fails(self):
        with self.assertRaises(KeyError):
            fingerprint("linux-x86_64", {k: v for k, v in FILES.items() if k != "conan.lock"})


if __name__ == "__main__":
    unittest.main()
