# Chobi

**Chobi** (ছবি, Bangla for "picture" and, colloquially, "movie") is a fast, minimal video
player for **Linux and Windows**. It is built on [libmpv](https://mpv.io) and Qt 6 Widgets:
mpv does the decoding, A/V sync and subtitle rendering, and a small native UI stays out of the way.

> Status: **early development (0.1.0)**. Linux is built, tested and benchmarked on real hardware.
> Windows builds and runs its tests in CI; real-hardware Windows validation is still pending.
> See [docs/known-limitations.md](docs/known-limitations.md).

## Features

- Plays practically everything FFmpeg can decode: MP4, MKV, WebM, AVI, MOV, TS/M2TS, VOB, FLV,
  WMV, OGV, MXF, 3GP… with H.264, HEVC (8/10-bit), AV1, VP9, MPEG-2/4, ProRes, DNxHD and many
  more. The exact contract is in [docs/formats.md](docs/formats.md).
- Hardware decoding (VA-API on Linux, D3D11VA on Windows) with automatic software fallback.
- **Automatic subtitle discovery** with strict matching: `Movie.mkv` picks up `Movie.srt`,
  `Movie.en.srt`, `Movie.en-US.srt`, `Movie.bn.srt`, `Movie.ass`… and never an unrelated file.
  SRT, ASS/SSA and WebVTT, Unicode (Bangla and English), automatic encoding detection.
- Multiple audio tracks with language, codec and channel layout; switching keeps the position.
- Drag and drop, "Open With", command-line files, paths with spaces and Unicode.
- Remembers where you stopped, keyed by file name and size (survives moving the file).
- Event-driven: 0% CPU when paused or idle, no polling, no telemetry, and no network access unless you open a URL.

## Keyboard shortcuts

| Key | Action |
|---|---|
| → / ← | Seek forward / backward 5 seconds |
| ↑ / ↓ | Volume up / down 5 points (0–100) |
| Space | Play / pause |
| Esc | Pause, leave fullscreen, then minimize (never quits) |
| F | Toggle fullscreen (double-click also works) |
| M | Mute |
| S | Subtitles on / off |
| A | Next audio track |
| O, Ctrl+O | Open a file |
| Ctrl+Q | Quit |

The mouse wheel changes the volume; right-click opens the full menu (audio, subtitles, speed).

## Building

Linux (Ubuntu 26.04 or newer; Fedora/Arch equivalents in [docs/building.md](docs/building.md)):

```bash
sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev libmpv-dev ffmpeg
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/chobi video.mkv
```

Windows (MSYS2 UCRT64): see [docs/building.md](docs/building.md#windows-msys2-ucrt64).
CI also uploads a portable Windows development build as an artifact on every push.

## Documentation

- [Architecture decision record](docs/adr/0001-architecture.md): why Qt Widgets + libmpv, how the
  stack was chosen, and the Windows render-path plan
- [Supported formats](docs/formats.md): tiered "all video types" contract
- [Benchmarks](docs/benchmarks.md): measured startup, memory, CPU, with methodology
- [Manual test checklist](docs/testing.md)
- [Known limitations](docs/known-limitations.md)

## বাংলায় সংক্ষেপে

Chobi একটি হালকা, দ্রুত ভিডিও প্লেয়ার, যা Linux ও Windows-এ চলে। ভিডিওর ফোল্ডারে একই নামের
সাবটাইটেল (যেমন `Movie.bn.srt`) থাকলে নিজে থেকেই লোড করে, বাংলা লেখা সঠিকভাবে দেখায়, আর একাধিক
অডিও ট্র্যাকের মধ্যে ভিডিও না থামিয়েই বদলানো যায়। কীবোর্ড শর্টকাট উপরের টেবিলে দেওয়া আছে।

## License

Chobi is free software under the **GNU General Public License v3.0 or later**; see
[LICENSE](LICENSE). Third-party components and their licenses are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Contributions are welcome:
[CONTRIBUTING.md](CONTRIBUTING.md).
