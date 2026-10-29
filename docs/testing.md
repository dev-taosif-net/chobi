# Testing

## Automated (run on every push, Linux + Windows)

```bash
ctest --test-dir build --output-on-failure        # everything
ctest --test-dir build -L unit                    # core logic only, < 1 s
ctest --test-dir build -L integration             # real libmpv on generated media
```

| Suite | What it proves |
|---|---|
| `test_keymap` | The exact shortcut table; Esc never maps to quit; unlisted modifiers do nothing |
| `test_playback_controller` | ±5 s seeks clamped to [0, duration], repeated keys accumulate, ±5 volume clamped to 0–100, Esc = pause → leave fullscreen → minimize, audio cycling, subtitle toggle |
| `test_subtitle_finder` | Spec examples, deterministic order, unrelated files rejected, Unicode/spaces, flags (`forced`, `sdh`), case handling |
| `test_language`, `test_track_label` | Language-tag parsing, labels with missing metadata |
| `test_resume_store`, `test_time_format` | Resume policy and file format, time display |
| `test_main_window` | Real window (offscreen) + fake engine: keys, Esc/minimize, Ctrl+Q, drag-and-drop of a Bangla/space/`%` path, subtitle discovery, file switching, error display, resume on/off, position saved on switch, forget positions, volume remembered on close (and not when off), settings file round trip, settings dialog |
| `test_mpv_engine` | Real libmpv headless: 16-clip format matrix, 3 audio tracks with metadata, switching without reload, exact seeks, end-of-file, resume start, external subtitles on a Unicode path, no subtitle leak across rapid switches, legacy encoding, broken files, partial download, 100-switch memory growth |

CI also runs the suite under AddressSanitizer + UndefinedBehaviorSanitizer on Linux.

## Manual checklist (per platform, per release)

Record OS, desktop/compositor, GPU and driver. Use your own clips or `tests/fixtures` output.

**Opening**
- [ ] Double-click a video in the file manager / "Open With → Chobi" plays it
- [ ] `chobi "path with spaces/ছবি ✓.mkv"` plays
- [ ] Drag a video from the file manager onto the window: it plays
- [ ] Drag a second video while playing: switches cleanly, old position remembered
- [ ] Drag a `.srt` onto a playing video: it loads as subtitle
- [ ] O and Ctrl+O open the file dialog; cancelling does nothing

**Keys** (with no menu open)
- [ ] →/← seek exactly 5 s; at the start/end they stop at the boundary
- [ ] ↑/↓ change volume by 5; never above 100 or below 0
- [ ] Space toggles play/pause; holding it does not flicker
- [ ] Esc while playing fullscreen: pauses, leaves fullscreen, minimizes; app still running
- [ ] F toggles fullscreen; M mutes; S toggles subtitles; A cycles audio tracks (OSD shows the name)
- [ ] Ctrl+Q quits; plain Q does nothing
- [ ] With the audio or subtitle menu open, arrow keys move in the menu, not the video

**Tracks and subtitles**
- [ ] Multi-audio MKV: menu lists language, codec, channels; switching keeps position
- [ ] Folder with `Movie.srt`, `Movie.en.srt`, `Movie.bn.srt`, `Other.srt`: three listed, `Other` not
- [ ] Bangla subtitle renders with correct conjuncts (e.g. "বাংলা", "ক্ষ")
- [ ] Subtitles → Off, then S: subtitles come back

**Robustness**
- [ ] Random/garbage file: clear error, player still usable
- [ ] Partially downloaded MKV: plays the available part
- [ ] Unplug/replug a monitor, or move the window between monitors with different DPI
- [ ] Suspend and resume while paused, then continue playback

**Settings** (⚙ button or right-click → Settings…)
- [ ] Watch a video past 30 s, close the app, reopen the same video: continues from there
- [ ] Turn "Resume…" off: the same video now starts from 0:00, and new positions aren't saved
- [ ] "Forget saved positions" clears them (reopen a resumed video: starts at 0:00)
- [ ] Set volume to 86 (slider), close the app, open another video: volume is 86
- [ ] Turn "Remember volume" off, restart: volume starts at 100

**UI**
- [ ] Controls auto-hide after about 2.5 s of playback without mouse movement, cursor hides; any mouse
      movement brings them back; paused = always visible
- [ ] "Loading…" only appears for slow opens (network/slow disk), never as a flash
