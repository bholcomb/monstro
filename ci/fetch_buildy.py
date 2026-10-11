#!/usr/bin/env python3
"""Downloads the buildy release pinned in ci/buildy.json for this platform, checks the archive's
SHA-256 against the pin, and unpacks buildy into the given directory. CI builds with it; local
development uses a buildy on PATH. Fails while the pin is a placeholder or on any mismatch."""

import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PIN = ROOT / "ci/buildy.json"
# Each platform: its runtime identifier, archive type and buildy's file name.
PLATFORMS = {
    ("linux", "x86_64"): ("linux-x64", "tar.gz", "buildy"),
    ("win32", "AMD64"): ("win-x64", "zip", "buildy.exe"),
    ("darwin", "arm64"): ("osx-arm64", "tar.gz", "buildy"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path, help="Directory to unpack buildy into")
    args = parser.parse_args()
    key = (sys.platform, platform.machine())
    if key not in PLATFORMS:
        parser.error(f"buildy is released for Linux x86_64, Windows x64 and macOS arm64, not {key}")
    rid, kind, name = PLATFORMS[key]
    pin = json.loads(PIN.read_text())
    version = pin["version"]
    expected = pin["sha256"].get(rid)
    if version is None or expected is None:
        raise SystemExit(f"{PIN.relative_to(ROOT)} pins no buildy release for {rid} yet: set its version and SHA-256s")
    archive_name = f"buildy-{version}-{rid}.{kind}"
    url = f"https://github.com/{pin['repository']}/releases/download/v{version}/{archive_name}"
    with tempfile.TemporaryDirectory() as temporary:
        archive = Path(temporary) / archive_name
        print(f"+ curl {url}", flush=True)
        subprocess.run(["curl", "--fail", "--location", "--silent", "--show-error", "--output", str(archive), url], check=True)
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != expected:
            raise SystemExit(f"{archive_name} has SHA-256 {digest}, not the pinned {expected}")
        if kind == "zip":
            with zipfile.ZipFile(archive) as source:
                data = source.read(name)
        else:
            with tarfile.open(archive) as source:
                member = source.extractfile(name)
                if member is None:
                    raise SystemExit(f"{archive_name} has no file {name}")
                data = member.read()
    args.destination.mkdir(parents=True, exist_ok=True)
    buildy = args.destination / name
    buildy.write_bytes(data)
    buildy.chmod(0o755)
    print(f"buildy {version} for {rid}: {buildy}")


if __name__ == "__main__":
    main()
