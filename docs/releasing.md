# Releasing and updates

## One-time setup (project owner)

```bash
scripts/setup-release-key.sh
git add packaging/update-signing-key.pub
git commit -m "Add update signing public key" && git push
```

The script creates an Ed25519 signing key in `~/.config/chobi-release/` (outside the repo), stores
the secret as the GitHub Actions secret `CHOBI_UPDATE_SIGNING_SEED`, and writes the public half
to `packaging/update-signing-key.pub`, which is compiled into the app.

**Back up `~/.config/chobi-release/update-signing-seed.txt` offline.** Installed copies only
accept updates signed with this key. If it is lost, a new key is needed and every user has to
install the next version by hand once.

## Every release

1. Write what changed under `## [Unreleased]` in `CHANGELOG.md` (it becomes the release notes).
2. Run:
   ```bash
   scripts/release.sh 0.3.0
   ```
   The script checks that `main` is clean and in sync, bumps the version (CMake + AppStream),
   dates the changelog, runs the unit tests, commits `Release 0.3.0`, tags `v0.3.0` and pushes.
3. The tag runs `.github/workflows/release.yml` (about 15 minutes):
   - **AppImage:** builds on Debian 13, runs all tests, packages, then starts the AppImage on
     Ubuntu 26.04, Fedora 43 and Arch to catch missing libraries.
   - **Windows:** MSYS2 build, all tests, portable zip, Inno Setup installer, and a list of the
     bundled MSYS2 packages with links to their source archives (GPL corresponding source).
   - **Publish:** writes `update.json`, signs it, checks that the signature verifies with the key
     built into the app, writes `SHA256SUMS`, and publishes the GitHub release.

   Watch it with `gh run watch`. To rehearse without publishing, use "Run workflow" on the
   Release workflow: same builds, artifacts only.
4. Within a day, every installed copy offers the update (or immediately via Settings → Check
   now).

## How the in-app updater works

- Fetches `https://github.com/dev-taosif-net/chobi/releases/latest/download/update.json` and
  `update.json.sig` (about 1 KB in total), at most once a day and 15 s after startup. No identifiers,
  usage data or file information are sent; the request is a plain HTTPS GET with the User-Agent
  `Chobi/<version>`.
- The signature (Ed25519, Monocypher) must verify against the built-in public key, otherwise
  nothing is offered. The manifest lists each package's URL, size and SHA-256, and a download
  is only used if both match.
- **AppImage:** the new file is downloaded next to the running one and atomically renamed over
  it. "Restart now" relaunches it on the same video, at the same position.
- **Windows installer build** (has `chobi-install.ini` next to `chobi.exe`): the new per-user
  installer runs with `/SILENT /CLOSEAPPLICATIONS`, Chobi quits, and the installer relaunches it.
- **Everything else** (portable zip, distro packages, source builds): "Download" opens the
  release page.
- `/releases/latest` ignores pre-releases, so a pre-release never reaches users automatically.
- Distribution packagers should build with `-DCHOBI_ENABLE_UPDATER=OFF`.

## Key rotation

If the key ever leaks: run `scripts/setup-release-key.sh` with a fresh `CHOBI_RELEASE_KEY_DIR`,
commit the new public key, and release. Copies with the old key will refuse the new manifest, so
announce in the release notes that users must download that version by hand.
