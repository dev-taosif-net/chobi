# Changelog

All notable changes to Chobi. The format follows [Keep a Changelog](https://keepachangelog.com),
and versions follow [Semantic Versioning](https://semver.org). `scripts/release.sh` turns the
"Unreleased" section into the next release's notes.

## [Unreleased]

## [0.4.0] - 2026-10-30

### Added
- **A title bar in fullscreen.** Moving the mouse shows a bar at the top with the video's title
  and Minimize, Exit fullscreen and Close buttons. It hides together with the control bar.
- **Enter toggles fullscreen**, like F (the numpad Enter too). Leaving fullscreen returns the
  window to how it was, maximized or not.
- Settings shows who made Chobi next to the version.

### Fixed
- **Control bar buttons no longer vanish on hover.** Windows drew a light box behind the white
  icons; hovered buttons now get a soft highlight instead.

## [0.3.0] - 2026-10-30

### Added
- **Time left** on the control bar, next to the total length: `−1:19:38 / 1:32:12`. Played and
  left always add up to the total.
- **Move the window by dragging the picture**, not only the title bar.
- **Backspace** pauses, leaves fullscreen and minimizes (what Esc used to do).
- **One window for all videos.** A video opened while Chobi is running (double-click in the file
  manager, "Open With", the command line) plays in that window, which comes to the front, even
  from minimized or behind the file manager. `--new-window` opens a separate window.

### Changed
- **5-second skips (← →) are instant, like mpv.** They used to land exactly 5 s away, which
  meant decoding every frame from the previous keyframe first: 110–350 ms per press on movies
  with 10 s keyframe gaps. They now do what mpv's arrow keys do and jump to a nearby keyframe in
  the direction of the key, so a skip can go a few seconds past 5 s. The on-screen time shows
  where playback really landed. Key press to new picture on screen: about 15–28 ms (stock mpv:
  about 35–44 ms). Clicking the seek bar is still exact.
- **Holding ← or → repeats like mpv:** after 200 ms, then 40 skips a second, whatever the
  desktop's key repeat is set to (GNOME waits 500 ms), so holding a key covers about twice the
  ground in the first second it used to.
- **No more frame-late skips on Wayland.** Chobi no longer waits for the compositor inside every
  buffer swap. After a skip, the on-screen time's own repaint could make the new picture wait an
  extra screen refresh, and holding a key kept the app busy waiting. Playback smoothness is
  unchanged; 60 fps video drops fewer frames.
- Opening a second video no longer opens a second window (see "One window" above).
- **Esc now closes the player.** With Resume on, the video continues from there next time.
- **New control bar icons.** Audio and subtitles are icons instead of words, and every button
  uses one drawn icon set that stays sharp at any scaling and readable with dark desktop themes
  (some themes drew the old play, volume and fullscreen icons dark on the dark bar).

### Fixed
- **Dragging the seek bar jumped back** to where the drag started when you let go. A drag now
  stays where you drop it, and a click seeks once instead of twice. Like mpv's controls: click
  for an exact jump, drag to scrub.
- **A covered or minimized window no longer stalls playback.** Every video frame waited 200 ms
  for a repaint that never came, so skips took 250–400 ms and video fell behind until the window
  was visible again.

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
