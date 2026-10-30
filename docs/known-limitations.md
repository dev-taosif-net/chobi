# Known limitations

## Platform

- **Windows is not yet verified on real hardware.** CI builds Chobi with MSYS2 and runs the
  automated tests, but playback, hardware decoding, drag-and-drop and file associations still
  need the manual checklist on a Windows PC.
- **Windows hardware decoding copies frames back to system memory** (`d3d11va-copy`), because
  mpv's OpenGL render API has no zero-copy D3D11 interop without ANGLE. A D3D11 render-API
  surface is planned once mpv 0.42 is released (see the ADR). Expect higher CPU use for 4K on
  Windows until then.
- **Wayland and minimize:** Backspace asks the compositor to minimize the window. GNOME and KDE
  honour this; some compositors (e.g. Sway, tiling setups) ignore minimize requests. Playback is
  still paused first, and the app never quits on Backspace.
- **Wayland, hidden windows:** when the compositor stops sending frame callbacks (window fully
  hidden on another workspace), video frames are not rendered. Audio continues and mpv may log
  render timeouts. Backspace pauses before minimizing, so the common case is unaffected.
- One file at a time; there is no playlist. A video opened while Chobi is running replaces the
  current one in the same window (`--new-window` opens another window).
- **Dragging from a maximized file manager:** clicking in the file manager brings it in front of
  Chobi, so there may be nothing of Chobi left to drop onto, and on GNOME Wayland an app can't
  keep itself on top. Double-click the video instead (it plays in the open Chobi window), or drag
  it to the top-left corner and hold it over Chobi's preview until Chobi comes forward.
- **X11 startup feedback:** handing a video to the running Chobi doesn't end the launcher's
  "starting" feedback, so on X11 the busy cursor may show for a few seconds. Wayland is
  unaffected.

## Rendering and formats

- Embedded playback uses mpv's legacy `vo_gpu` renderer on every render-API path: **no Dolby
  Vision profile 5 reshaping (colours are wrong) and no HDR passthrough**; HDR10/HLG are
  tone-mapped to SDR.
- No DRM (Widevine, PlayReady, FairPlay, AACS/BD+), no DVD/Blu-ray menus.
- AV1 and VVC are software-decoded on GPUs without those decoders (e.g. AMD Renoir); 4K AV1
  then needs a fast CPU.
- MP4/MOV files whose index (`moov` atom) is at the end cannot play until fully downloaded;
  partial MKV/TS files play up to the downloaded part.
- No youtube-dl/yt-dlp integration (by design: no helper processes).

## Subtitles

- Auto-discovery only looks in the video's own folder (no `Subs/` subfolder scan yet).
- Bijoy/SutonnyMJ "ASCII" Bangla subtitles are not Unicode and render as Latin gibberish.
- Language tags in filenames must be ISO 639-1/639-2 codes or common English names
  (`Movie.en.srt`, `Movie.ben.srt`, `Movie.Bangla.srt`). Other tags are deliberately rejected so an
  unrelated file is never picked.
- Inside Flatpak (when available), the sandbox may hide sibling subtitle files unless the
  package grants host read access.

## Packages and updates

- The AppImage needs glibc 2.41+ (it is built on Debian 13). Ubuntu 24.04 LTS and older can't
  run it.
- Packages are large (AppImage about 115 MB, Windows installer of similar size) because they bundle
  Debian's or MSYS2's full FFmpeg (including encoders Chobi never uses). A slim, pinned
  libmpv/FFmpeg build is planned.
- The Windows installer and in-app update have not yet been tested on a real Windows PC (CI
  only). The installer is not code-signed yet, so SmartScreen shows a warning.
- The portable Windows zip and distribution packages don't update themselves; they show a
  download link.

## Resume positions

- Saved positions last 7 days after the video was last watched, then they are removed.
- Deleted videos are detected at the next start: moving or renaming a video looks the same as
  deleting it, so its position is forgotten too.
- If a video's whole folder is missing at start (unplugged USB drive, offline network share),
  its position is kept (it may come back) and only the 7-day expiry applies.

## Resources

- Idle memory is about 130 MB PSS on the reference machine, mostly the GPU driver stack (see
  benchmarks). The original 80 MB target was not met.
