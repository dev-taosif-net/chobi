#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes update.json (the in-app updater's manifest) for one release.

Usage: make_update_manifest.py VERSION DIST_DIR BASE_URL > update.json

BASE_URL is where the release assets are downloadable, e.g.
https://github.com/dev-taosif-net/chobi/releases/download/v0.2.0/
The release workflow signs the output with tools/chobi_sign.cpp; see docs/releasing.md.
"""
import hashlib
import json
import os
import sys


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    version, dist, base = sys.argv[1], sys.argv[2], sys.argv[3]
    if not base.endswith("/"):
        base += "/"
    wanted = {
        "linux-x86_64-appimage": f"Chobi-{version}-x86_64.AppImage",
        "windows-x64-setup": f"Chobi-{version}-windows-x64-setup.exe",
    }
    assets = {}
    for key, name in wanted.items():
        path = os.path.join(dist, name)
        if not os.path.isfile(path):
            print(f"warning: {name} missing, not offered as an update", file=sys.stderr)
            continue
        digest = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(1 << 20), b""):
                digest.update(chunk)
        assets[key] = {"url": base + name, "sha256": digest.hexdigest(),
                       "size": os.path.getsize(path)}
    if not assets:
        sys.exit("no release assets found")
    manifest = {
        "schema": 1,
        "version": version,
        "notes_url": base.replace("/releases/download/", "/releases/tag/").rstrip("/"),
        "assets": assets,
    }
    sys.stdout.write(json.dumps(manifest, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
