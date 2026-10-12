#!/usr/bin/env python3
"""Sets version, download url and hash in the Scoop manifest for a new release.

Usage: update-scoop-manifest.py <manifest> <version> <sha256>

The url comes from the manifest's autoupdate url with $version filled in. All other fields
and their order stay as they are.
"""
import json
import re
import sys


def main(argv):
    if len(argv) != 4:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    path, version, sha256 = argv[1:]
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
        print(f"Not a version x.y.z: {version!r}", file=sys.stderr)
        return 1
    if not re.fullmatch(r"[0-9a-fA-F]{64}", sha256):
        print(f"Not a SHA-256 hash: {sha256}", file=sys.stderr)
        return 1
    with open(path, encoding="utf-8") as f:
        manifest = json.load(f)
    manifest["version"] = version
    manifest["url"] = manifest["autoupdate"]["url"].replace("$version", version)
    manifest["hash"] = sha256.lower()
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
