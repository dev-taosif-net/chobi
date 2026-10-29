# Changelog

All notable changes to Chobi. The format follows [Keep a Changelog](https://keepachangelog.com),
and versions follow [Semantic Versioning](https://semver.org). `scripts/release.sh` turns the
"Unreleased" section into the next release's notes.

## [Unreleased]

## [0.2.0] - 2026-10-29

### Added
- **Automatic updates.** Chobi checks GitHub once a day for a new release (can be turned off in
  Settings, or "Check now"). Update information is signed with the project's Ed25519 key and
  every download is checked against it. The Linux AppImage and the Windows installer version update
  themselves and restart into the same video at the same position; other installs get a
  download link.
- **Release packages:** Linux AppImage (x86_64), Windows installer (per user, no administrator
  rights, registers "Open with") and a portable Windows zip.
- **Settings window** (⚙ on the control bar, or right-click → Settings…).
- **Resume where you stopped** can be turned on/off, with "Forget saved positions". A position is
  kept for 7 days after the video was last watched and is removed when the video is deleted.
- **Remember volume** between videos and sessions.

### Fixed
- Windows build: find libmpv correctly in MSYS2 (pkg-config reports MSYS-style paths).
- Build with Qt 6.8 (Debian 13).

## [0.1.0] - 2026-10-29

### Added
- First version: libmpv + Qt 6 Widgets player for Linux and Windows. It plays practically every
  format FFmpeg decodes, with hardware decoding (VA-API zero-copy on Linux), the specified keyboard
  shortcuts, drag and drop, "Open With", strict automatic subtitle matching (Bangla and
  English), multiple audio tracks, and clear errors for broken files.
