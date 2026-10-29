# ADR 0001: Qt 6 Widgets + libmpv

- Status: accepted
- Date: 2026-10-29

## Context

The product is an ultra-lightweight video player for Linux and Windows. In priority order, the
selection criteria were: playback reliability and codec coverage ("all types of video"),
Linux + Windows support, startup time, memory, CPU/GPU use, hardware decoding, subtitles and
multi-audio, packaging and licensing, and long-term maintainability.

Ten options and perspectives were argued against each other in a structured two-round review
(one advocate per option, plus a performance/format specialist and a red team), and the
disputed facts were then checked against upstream source and documentation:

| Option | Outcome |
|---|---|
| **Qt 6 Widgets + libmpv render API** | **Chosen**: ranked first by all ten reviewers |
| Qt 6 Quick/QML + libmpv | Runner-up; revisit only if Widgets fails the input-latency gate |
| Qt Multimedia (FFmpeg backend) | No API to load external subtitle files, no ASS styling; Qt's bundled FFmpeg lacks dav1d, so AV1 fails without hardware decode |
| libVLC | libVLC 4 not yet stable; `IPlayerEngine` keeps a VLC engine possible later (disc menus) |
| GStreamer | Narrower auto-plugged demuxing, weaker subtitles, harder Windows packaging |
| SDL3 / minimal shell + libmpv | Little real saving on GNOME (pulls in GTK anyway); hand-built dialogs and menus. Kept as a benchmark challenger |
| Rust (winit-based UIs) + libmpv | Stable winit lacks Wayland file drop; no D3D11 host for mpv's new render API |
| Separate native UIs (Win32 + GTK4) | Two UIs to build and test; mpv's D3D11 render API removes the main advantage |

## Decision

- **Engine:** libmpv. FFmpeg covers formats; libass (with HarfBuzz/FriBidi) renders ASS/SRT,
  including Bangla shaping; tracks switch seamlessly; hardware decoding falls back to software.
- **UI:** C++17 + Qt 6 Widgets, one codebase for both OSes (native dialogs, drag-and-drop,
  Unicode paths, Wayland and X11).
- **Seams:** `core::IPlayerEngine` (engine-neutral commands and track model),
  `PlayerEngine` (Qt signals), `IVideoSurface` (where pixels go), `core::IWindowController`
  (fullscreen/minimize, mockable). `src/core` has no Qt or libmpv types.
- **Video surface, Linux:** `QOpenGLWidget` + OpenGL render API over EGL, passing the
  Wayland/X11 display; `hwdec=vaapi,auto-safe`. VA-API frames are imported zero-copy.
  (`QOpenGLWidget` was picked over `QRhiWidget` for the GL path as the more proven option; both
  composite through one extra GPU blit.)
- **Video surface, Windows (gated):**
  1. v1: OpenGL render API with `d3d11va-copy`. It works on released mpv 0.41, but copies frames
     back, because GL interop on Windows needs ANGLE (verified in mpv's source).
  2. Target: `QRhiWidget` (D3D11) + `MPV_RENDER_API_TYPE_D3D11`. This exists only on mpv master
     (commit `4255130`, 2026-10-15; no release yet). Enable it on a pinned commit or 0.42 after
     a soak test.
  3. Last resort: native-window `wid` embedding. It gets gpu-next and HDR, but controls must live
     in their own window; `ControlsBar` is self-contained for that reason.

  The default is chosen by measurement on real Windows hardware (1080p H.264, 4K HEVC 10-bit).
- **Threading:** with `ADVANCED_CONTROL` the GUI thread is mpv's render thread, so after
  initialization only the render-thread-safe client API is used (`*_async`, `observe_property`,
  `wait_event(0)`). Rendering never blocks (`BLOCK_FOR_TARGET_TIME=0` with
  `video-timing-offset=0`). State comes from observed properties; there are no polling timers.
- **Subtitles:** mpv's `sub-auto` is off. `SubtitleFinder` runs on a worker thread with one
  non-recursive directory listing. Matches are bound to the exact `loadfile` (via its playlist
  entry id), so rapid file switching can never attach one video's subtitles to another.
- **License:** GPL-3.0-or-later (compatible with GPL libmpv builds and LGPL Qt).

## Consequences

- Embedded rendering uses mpv's legacy `vo_gpu` renderer, not `gpu-next`, on every render-API
  path. Dolby Vision profile 5 colours and HDR passthrough are therefore unavailable in embedded
  mode (documented in known limitations).
- Idle memory is dominated by Qt and the GPU driver (Mesa's LLVM alone is ~43 MB RSS on the
  reference machine), not by Chobi; see [benchmarks](../benchmarks.md).
- Windows release builds must bundle a pinned libmpv/FFmpeg built in our own CI with a
  corresponding-source archive. The MSYS2-based CI artifact is a development build only.
