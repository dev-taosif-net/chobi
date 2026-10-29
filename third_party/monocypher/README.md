# Monocypher 4.0.3 (vendored)

Used only to verify (in the app) and create (in the release workflow, `tools/chobi_sign.cpp`)
the Ed25519 signature on update manifests.

- Upstream: https://monocypher.org, https://github.com/LoupVaillant/Monocypher
- Version: 4.0.3 (release tarball `monocypher-4.0.3.tar.gz`, 2026-06-15)
- Files: `src/monocypher.{c,h}`, `src/optional/monocypher-ed25519.{c,h}`, `LICENCE.md`, unmodified
- Verified on import: identical to the git tag `4.0.3` except the release script's version
  stamp (`__git__` → `4.0.3`)
- License: BSD-2-Clause or CC0-1.0 (dual; see `LICENCE.md`). GPL-3.0-compatible.

To update: replace the five files from a new release tarball, re-run the comparison against the
tag, and update this file.
