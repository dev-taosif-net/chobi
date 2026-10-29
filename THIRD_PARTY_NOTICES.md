# Third-party components

Chobi's own code is GPL-3.0-or-later. It links against the following libraries. Source builds use
the copies installed on your system; any binary package we publish must ship the corresponding
license texts and offer the exact source of these components (GPLv3 section 6).

| Component | Used for | License |
|---|---|---|
| [Qt 6](https://www.qt.io) (Core, Gui, Widgets, OpenGL, OpenGLWidgets) | UI | LGPL-3.0 (dynamically linked) |
| [libmpv](https://mpv.io) | Playback engine | GPL-2.0-or-later as normally built (LGPL-2.1-or-later only when built with `-Dgpl=false`) |
| [FFmpeg](https://ffmpeg.org) (via libmpv) | Demuxing, decoding | LGPL-2.1-or-later, or GPL when built with GPL components |
| [libass](https://github.com/libass/libass) (via libmpv) | Subtitle rendering | ISC |
| [HarfBuzz](https://harfbuzz.github.io), [FriBidi](https://github.com/fribidi/fribidi) (via libass) | Complex-script shaping (Bangla) | MIT / LGPL-2.1-or-later |
| [Monocypher](https://monocypher.org) 4.0.3 (vendored in `third_party/monocypher`) | Verifying signed update manifests (Ed25519) | BSD-2-Clause or CC0-1.0 |
| [Noto Sans Bengali](https://fonts.google.com/noto) (bundled only in Windows packages) | Bangla subtitle fallback font | SIL Open Font License 1.1 |

Patent note: some codecs (e.g. HEVC, AAC, AC-3) may be covered by patents in some countries.
Chobi does not include any encoder; decoding support comes entirely from the FFmpeg build you use.
