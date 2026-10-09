#!/usr/bin/env python3
import subprocess, sys

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <elf-binary>", file=sys.stderr)
    sys.exit(1)

out = subprocess.check_output(["readelf", "-d", sys.argv[1]], text=True)
for line in out.splitlines():
    if "RPATH" in line or "RUNPATH" in line:
        paths = line.split("[")[1].rstrip("]")
        print(":".join(p for p in paths.split(":") if "conan2" in p))
        break
