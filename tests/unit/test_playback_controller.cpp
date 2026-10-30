// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/PlaybackController.h"

using namespace chobi::core;

namespace {

struct RecordingEngine : IPlayerEngine {
  PlaybackState s;
  std::vector<Track> t;
  std::vector<std::string> log;

  void open(const std::string& p, double, std::vector<SubtitleFile>) override {
    log.push_back("open " + p);
  }
  void stop() override { log.push_back("stop"); }
  void setPaused(bool p) override {
    s.paused = p;
    log.push_back(p ? "pause" : "play");
  }
  void seekAbsolute(double sec, bool exact) override {
    log.push_back("seek " + std::to_string(sec) + (exact ? " exact" : " key"));
  }
  void seekRelative(double delta) override {
    log.push_back("seek " + std::to_string(delta) + " relative");
  }
  void setVolume(double v) override { s.volume = v; }
  void setMuted(bool m) override { s.muted = m; }
  void setSpeed(double v) override { s.speed = v; }
  void selectAudioTrack(std::int64_t id) override {
    for (Track& x : t) {
      if (x.type == TrackType::Audio) x.selected = x.id == id;
    }
  }
  void selectSubtitleTrack(std::int64_t id) override {
    for (Track& x : t) {
      if (x.type == TrackType::Subtitle) x.selected = x.id == id;
    }
  }
  void setSubtitlesVisible(bool v) override { s.subtitlesVisible = v; }
  void addSubtitleFile(const std::string&, bool, const std::string&, const std::string&) override {}
  const PlaybackState& state() const override { return s; }
  const std::vector<Track>& tracks() const override { return t; }
};

struct RecordingWindow : IWindowController {
  bool fullscreen = false;
  std::vector<std::string> log;
  bool isFullscreen() const override { return fullscreen; }
  void setFullscreen(bool f) override {
    fullscreen = f;
    log.push_back(f ? "fullscreen" : "windowed");
  }
  void minimize() override { log.push_back("minimize"); }
  void requestOpenFile() override { log.push_back("open-dialog"); }
  void requestQuit() override { log.push_back("quit"); }
  void showMessage(const std::string& m) override { log.push_back("msg " + m); }
  void showPosition() override { log.push_back("position"); }
};

Track audio(std::int64_t id, bool selected, std::string lang = {}) {
  Track t;
  t.id = id;
  t.type = TrackType::Audio;
  t.selected = selected;
  t.language = std::move(lang);
  return t;
}

}  // namespace

class PlaybackControllerTest : public QObject {
  Q_OBJECT

  RecordingEngine engine;
  RecordingWindow window;

 private slots:
  void init() {
    engine = {};
    window = {};
    engine.s.loaded = true;
    engine.s.duration = 100;
    engine.s.position = 50;
  }

  void clampHelpers() {
    QCOMPARE(clampSeekTarget(50, 5, 100), 55.0);
    QCOMPARE(clampSeekTarget(2, -5, 100), 0.0);
    QCOMPARE(clampSeekTarget(98, 5, 100), 100.0);
    QCOMPARE(clampSeekTarget(98, 5, 0), 103.0);  // unknown duration: only lower bound
    QCOMPARE(clampVolume(103), 100.0);
    QCOMPARE(clampVolume(-2), 0.0);
  }

  void seekFiveSecondsBothWays() {
    PlaybackController c(engine, window);
    c.trigger(Action::SeekForward);
    QCOMPARE(engine.log.back(), std::string("seek 5.000000 relative"));
    QCOMPARE(window.log.back(), std::string("position"));
    c.onSeekFinished();
    engine.s.position = 55;
    c.trigger(Action::SeekBackward);
    QCOMPARE(engine.log.back(), std::string("seek -5.000000 relative"));
  }

  void arrowKeysNeverSendAnExactSeek() {
    // An exact seek decodes from the previous keyframe up to its target: 110-350 ms per press
    // on a movie with 10 s keyframe gaps, where stock mpv's relative seek takes about 25 ms.
    PlaybackController c(engine, window);
    for (Action a : {Action::SeekForward, Action::SeekBackward, Action::SeekBackward}) {
      c.trigger(a);
      c.onSeekFinished();
    }
    QCOMPARE(engine.log.size(), std::size_t{3});
    for (const std::string& line : engine.log) {
      QVERIFY2(line.find(" relative") != std::string::npos, line.c_str());
    }
  }

  void repeatedSeeksAccumulateBeforeEngineCatchesUp() {
    PlaybackController c(engine, window);
    engine.s.position = 88;
    c.trigger(Action::SeekForward);
    c.trigger(Action::SeekForward);
    c.trigger(Action::SeekForward);  // 88 + 5 + 5 is past the end: only the 2 s that are left
    c.trigger(Action::SeekForward);  // already heading for the end: no command
    QCOMPARE(engine.log, (std::vector<std::string>{"seek 5.000000 relative",
                                                   "seek 5.000000 relative",
                                                   "seek 2.000000 relative"}));
  }

  void seekRespectsBoundaries() {
    PlaybackController c(engine, window);
    engine.s.position = 98;
    c.trigger(Action::SeekForward);
    QCOMPARE(engine.log.back(), std::string("seek 2.000000 relative"));
    c.onSeekFinished();
    engine.s.position = 100;
    const auto before = engine.log.size();
    c.trigger(Action::SeekForward);  // already at the end: no command
    QCOMPARE(engine.log.size(), before);
    engine.s.position = 3;
    c.trigger(Action::SeekBackward);
    QCOMPARE(engine.log.back(), std::string("seek -3.000000 relative"));
  }

  void seekBarStaysExact() {
    PlaybackController c(engine, window);
    c.seekTo(42);
    QCOMPARE(engine.log.back(), std::string("seek 42.000000 exact"));
    QCOMPARE(window.log.back(), std::string("position"));
  }

  void seekIgnoredWithoutFile() {
    PlaybackController c(engine, window);
    engine.s.loaded = false;
    c.trigger(Action::SeekForward);
    QVERIFY(engine.log.empty());
  }

  void volumeStepsAndClamps() {
    PlaybackController c(engine, window);
    engine.s.volume = 50;
    c.trigger(Action::VolumeUp);
    QCOMPARE(engine.s.volume, 55.0);
    c.trigger(Action::VolumeDown);
    c.trigger(Action::VolumeDown);
    QCOMPARE(engine.s.volume, 45.0);
    engine.s.volume = 98;
    c.trigger(Action::VolumeUp);
    QCOMPARE(engine.s.volume, 100.0);
    c.trigger(Action::VolumeUp);
    QCOMPARE(engine.s.volume, 100.0);
    engine.s.volume = 3;
    c.trigger(Action::VolumeDown);
    QCOMPARE(engine.s.volume, 0.0);
  }

  void backspacePausesThenLeavesFullscreenThenMinimizes() {
    PlaybackController c(engine, window);
    engine.s.paused = false;
    window.fullscreen = true;
    c.trigger(Action::PauseAndMinimize);
    QCOMPARE(engine.log.back(), std::string("pause"));
    QCOMPARE(window.log, (std::vector<std::string>{"windowed", "minimize"}));
    QVERIFY(engine.s.paused);
  }

  void backspaceWindowedJustMinimizes() {
    PlaybackController c(engine, window);
    c.trigger(Action::PauseAndMinimize);
    QCOMPARE(window.log, (std::vector<std::string>{"minimize"}));
  }

  void togglePauseAndReplayAtEnd() {
    PlaybackController c(engine, window);
    engine.s.paused = false;
    c.trigger(Action::TogglePause);
    QVERIFY(engine.s.paused);
    engine.s.eofReached = true;
    c.trigger(Action::TogglePause);
    QCOMPARE(engine.log[engine.log.size() - 2], std::string("seek 0.000000 exact"));
    QCOMPARE(engine.log.back(), std::string("play"));
  }

  void cycleAudioWraps() {
    PlaybackController c(engine, window);
    engine.t = {audio(1, true, "en"), audio(2, false, "bn"), audio(3, false)};
    c.trigger(Action::CycleAudioTrack);
    QVERIFY(engine.t[1].selected);
    QCOMPARE(window.log.back(), std::string("msg Audio: Bangla"));
    c.trigger(Action::CycleAudioTrack);
    QVERIFY(engine.t[2].selected);
    QCOMPARE(window.log.back(), std::string("msg Audio: Unknown language"));
    c.trigger(Action::CycleAudioTrack);
    QVERIFY(engine.t[0].selected);
  }

  void cycleAudioSingleOrNone() {
    PlaybackController c(engine, window);
    c.trigger(Action::CycleAudioTrack);
    QCOMPARE(window.log.back(), std::string("msg No audio track"));
    engine.t = {audio(1, true, "en")};
    c.trigger(Action::CycleAudioTrack);
    QCOMPARE(window.log.back(), std::string("msg Audio: English (only track)"));
  }

  void subtitleToggle() {
    PlaybackController c(engine, window);
    c.trigger(Action::ToggleSubtitles);
    QCOMPARE(window.log.back(), std::string("msg No subtitles"));

    Track sub;
    sub.id = 4;
    sub.type = TrackType::Subtitle;
    sub.language = "bn";
    sub.selected = true;
    engine.t = {sub};
    c.onTracksChanged();
    c.trigger(Action::ToggleSubtitles);
    QVERIFY(!engine.s.subtitlesVisible);
    c.trigger(Action::ToggleSubtitles);
    QVERIFY(engine.s.subtitlesVisible);

    engine.selectSubtitleTrack(0);  // turned off from the menu
    c.trigger(Action::ToggleSubtitles);
    QVERIFY(engine.t[0].selected);  // S brings the last one back
    QVERIFY(engine.s.subtitlesVisible);
  }

  void fullscreenMuteOpenQuit() {
    PlaybackController c(engine, window);
    c.trigger(Action::ToggleFullscreen);
    QVERIFY(window.fullscreen);
    c.trigger(Action::ToggleMute);
    QVERIFY(engine.s.muted);
    c.trigger(Action::OpenFile);
    QCOMPARE(window.log.back(), std::string("open-dialog"));
    c.trigger(Action::Quit);
    QCOMPARE(window.log.back(), std::string("quit"));
  }
};

QTEST_APPLESS_MAIN(PlaybackControllerTest)
#include "test_playback_controller.moc"
