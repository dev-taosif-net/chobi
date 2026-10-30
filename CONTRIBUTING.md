# Contributing to Chobi

Thanks for helping! Bug reports, test files that don't play, translations and code are all welcome.

## Reporting a bug

Please include your OS and version, the Chobi version (`chobi --version`), and for playback
problems the output of `CHOBI_MPV_LOG=1 chobi yourfile.mkv 2> log.txt`. Do not attach copyrighted
media; describe it (container, codecs) or share a short clip you made yourself.

## Building and testing

See [docs/building.md](docs/building.md). Before opening a pull request:

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```

The integration tests generate their own media with ffmpeg; nothing binary is committed.
A sanitizer build is available with `-DCHOBI_SANITIZE=ON` (CI runs it too).

## Code layout and rules

- `src/core/`: pure C++17, **no Qt, no libmpv**. Every behavior rule (key map, seeking and volume
  clamps, Backspace order, subtitle matching, resume policy) lives here and has unit tests.
- `src/engine/`: libmpv integration. After initialization only the asynchronous,
  render-thread-safe libmpv API may be used, because the GUI thread also renders video
  (see the comment in `MpvEngine.h`). Never add a synchronous `mpv_get_property`/`mpv_command`.
- `src/app/`: Qt Widgets UI. No filesystem work on the UI thread; use the thread pool.
- Event-driven only: no polling timers for playback state.
- Format with `clang-format` (config in the repo). Match the surrounding style.
- Every source file starts with `// SPDX-License-Identifier: GPL-3.0-or-later`.

## License of contributions

By submitting a contribution you agree that it is licensed under the GNU GPL v3.0 or later,
the same license as the project.
