// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <vector>

#include "engine/PlayerEngine.h"

// Synchronous stand-in for MpvEngine: applies commands immediately and records them.
class FakeEngine final : public chobi::PlayerEngine {
  Q_OBJECT

 public:
  struct Opened {
    std::string location;
    double start;
    std::vector<chobi::core::SubtitleFile> subtitles;
  };

  chobi::core::PlaybackState s;
  std::vector<chobi::core::Track> t;
  std::vector<Opened> opened;
  std::vector<double> seeks;

  void open(const std::string& location, double start,
            std::vector<chobi::core::SubtitleFile> subs) override {
    opened.push_back({location, start, std::move(subs)});
  }
  void stop() override {}
  void setPaused(bool p) override {
    s.paused = p;
    emit pausedChanged(p);
  }
  void seekAbsolute(double sec, bool) override {
    seeks.push_back(sec);
    s.position = sec;
    emit positionChanged(sec);
  }
  void seekRelative(double delta) override { seekAbsolute(s.position + delta, false); }
  void setVolume(double v) override {
    s.volume = v;
    emit volumeChanged(v);
  }
  void setMuted(bool m) override {
    s.muted = m;
    emit mutedChanged(m);
  }
  void setSpeed(double v) override {
    s.speed = v;
    emit speedChanged(v);
  }
  void selectAudioTrack(std::int64_t id) override {
    for (auto& x : t) {
      if (x.type == chobi::core::TrackType::Audio) x.selected = x.id == id;
    }
    emit tracksChanged();
  }
  void selectSubtitleTrack(std::int64_t id) override {
    for (auto& x : t) {
      if (x.type == chobi::core::TrackType::Subtitle) x.selected = x.id == id;
    }
    emit tracksChanged();
  }
  void setSubtitlesVisible(bool v) override {
    s.subtitlesVisible = v;
    emit subtitlesVisibleChanged(v);
  }
  void addSubtitleFile(const std::string&, bool, const std::string&, const std::string&) override {}
  const chobi::core::PlaybackState& state() const override { return s; }
  const std::vector<chobi::core::Track>& tracks() const override { return t; }

  // Test helper: pretend the last opened file finished loading.
  void finishLoading(double duration) {
    s.loaded = true;
    s.duration = duration;
    s.position = 0;
    emit durationChanged(duration);
    emit fileLoaded();
  }
};
