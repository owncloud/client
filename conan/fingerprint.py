#!/usr/bin/env python3
"""Fingerprint of the Conan inputs of one profile: the tag of its dependency archive on ghcr.io.

The archive is ghcr.io/owncloud/client-deps:<profile>-<fingerprint>, published by the GitLab dependency
builder, which computes the same value (its depbuilder/fingerprint.py; conan/test_fingerprint.py pins
both to one fixture). Any change to these files needs new dependency binaries.

Usage: python3 conan/fingerprint.py --profile NAME --root .
"""

import argparse
import hashlib
import pathlib
import re
import sys

INPUT_PATHS = ("conanfile.py", "conan.lock", "conan/recipes-commit")
PROFILE_RE = re.compile(r"[a-z0-9][a-z0-9_-]*")


def fingerprint(profile, files):
    paths = (*INPUT_PATHS, f"conan/profiles/{profile}")
    missing = [path for path in paths if path not in files]
    if missing:
        raise KeyError(f"missing fingerprint input: {', '.join(missing)}")
    lines = sorted(f"{path}\0{hashlib.sha256(files[path]).hexdigest()}\n" for path in paths)
    digest = hashlib.sha256(profile.encode() + b"\n")
    digest.update("".join(lines).encode())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--profile", required=True)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    args = parser.parse_args()
    if not PROFILE_RE.fullmatch(args.profile):
        parser.error(f"invalid profile name {args.profile!r}")
    paths = (*INPUT_PATHS, f"conan/profiles/{args.profile}")
    try:
        files = {path: (args.root / path).read_bytes() for path in paths}
    except OSError as error:
        print(f"error: cannot read fingerprint input: {error}", file=sys.stderr)
        return 2
    print(fingerprint(args.profile, files))
    return 0


if __name__ == "__main__":
    sys.exit(main())
