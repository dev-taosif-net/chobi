// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/PlaybackController.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/TrackLabel.h"

namespace chobi::core {
namespace {

constexpr double kEpsilon = 1e-3;

std::vector<const Track*> tracksOfType(const std::vector<Track>& tracks, TrackType type) {
  std::vector<const Track*> out;
  for (const Track& t : tracks) {
    if (t.type == type) out.push_back(&t);
  }
  return out;
}

}  // namespace

double clampSeekTarget(double current, double delta, double duration) {
  double target = current + delta;
  if (!std::isfinite(target) || target < 0) target = 0;
  if (duration > 0 && target > duration) target = duration;
  return target;
}

double clampVolume(double volume) {
  if (!std::isfinite(volume)) return 0;
  return std::clamp(volume, 0.0, 100.0);
}

PlaybackController::PlaybackController(IPlayerEngine& engine, IWindowController& window)
    : engine_(engine), window_(window) {}

void PlaybackController::trigger(Action action) {
  switch (action) {
    case Action::SeekForward: seekBy(kSeekStepSeconds); break;
    case Action::SeekBackward: seekBy(-kSeekStepSeconds); break;
    case Action::VolumeUp: changeVolume(kVolumeStep); break;
    case Action::VolumeDown: changeVolume(-kVolumeStep); break;
    case Action::TogglePause: togglePause(); break;
    case Action::PauseAndMinimize: pauseAndMinimize(); break;
    case Action::ToggleFullscreen: toggleFullscreen(); break;
    case Action::ToggleMute: toggleMute(); break;
    case Action::ToggleSubtitles: toggleSubtitles(); break;
    case Action::CycleAudioTrack: cycleAudioTrack(); break;
    case Action::OpenFile: window_.requestOpenFile(); break;
    case Action::Quit: window_.requestQuit(); break;
    case Action::None: break;
  }
}

void PlaybackController::seekBy(double delta) {
  const PlaybackState& s = engine_.state();
  if (!s.loaded) return;
  const double base = pendingSeek_.value_or(s.position);
  const double target = clampSeekTarget(base, delta, s.duration);
  if (std::fabs(target - base) < kEpsilon) return;  // already at a boundary
  // Never an exact seek here: those decode from the previous keyframe up to the target, which
  // made every press lag on real movies. This is what stock mpv's arrow keys do.
  engine_.seekRelative(target - base);
  pendingSeek_ = target;
  window_.showPosition();
}

void PlaybackController::seekTo(double seconds) {
  const PlaybackState& s = engine_.state();
  if (!s.loaded) return;
  const double target = clampSeekTarget(seconds, 0, s.duration);
  engine_.seekAbsolute(target, true);
  pendingSeek_ = target;
  window_.showPosition();
}

void PlaybackController::changeVolume(double delta) {
  const double v = clampVolume(engine_.state().volume + delta);
  engine_.setVolume(v);
  window_.showMessage("Volume " + std::to_string(static_cast<int>(std::lround(v))) + "%" +
                      (engine_.state().muted ? " (muted)" : ""));
}

void PlaybackController::togglePause() {
  const PlaybackState& s = engine_.state();
  if (!s.loaded) return;
  if (s.paused && s.eofReached) {  // replay from the start after the end
    seekTo(0);
    engine_.setPaused(false);
    return;
  }
  engine_.setPaused(!s.paused);
}

void PlaybackController::toggleMute() {
  const bool muted = !engine_.state().muted;
  engine_.setMuted(muted);
  window_.showMessage(muted ? "Muted" : "Unmuted");
}

void PlaybackController::toggleSubtitles() {
  const auto subs = tracksOfType(engine_.tracks(), TrackType::Subtitle);
  if (subs.empty()) {
    window_.showMessage("No subtitles");
    return;
  }
  const auto selected = std::find_if(subs.begin(), subs.end(),
                                     [](const Track* t) { return t->selected; });
  if (selected != subs.end()) {
    const bool visible = !engine_.state().subtitlesVisible;
    engine_.setSubtitlesVisible(visible);
    window_.showMessage(visible ? "Subtitles: " + trackLabel(**selected) : "Subtitles off");
    return;
  }
  // Nothing selected (turned off from the menu): bring back the last one, else the first.
  const Track* pick = subs.front();
  for (const Track* t : subs) {
    if (t->id == lastSubtitleId_) pick = t;
  }
  engine_.selectSubtitleTrack(pick->id);
  engine_.setSubtitlesVisible(true);
  window_.showMessage("Subtitles: " + trackLabel(*pick));
}

void PlaybackController::cycleAudioTrack() {
  const auto audio = tracksOfType(engine_.tracks(), TrackType::Audio);
  if (audio.empty()) {
    window_.showMessage("No audio track");
    return;
  }
  if (audio.size() == 1) {
    window_.showMessage("Audio: " + trackLabel(*audio.front()) + " (only track)");
    return;
  }
  std::size_t current = audio.size() - 1;  // nothing selected -> start at the first track
  for (std::size_t i = 0; i < audio.size(); ++i) {
    if (audio[i]->selected) current = i;
  }
  const Track* next = audio[(current + 1) % audio.size()];
  engine_.selectAudioTrack(next->id);
  window_.showMessage("Audio: " + trackLabel(*next));
}

void PlaybackController::pauseAndMinimize() {
  if (engine_.state().loaded) engine_.setPaused(true);
  if (window_.isFullscreen()) window_.setFullscreen(false);
  window_.minimize();
}

void PlaybackController::toggleFullscreen() { window_.setFullscreen(!window_.isFullscreen()); }

void PlaybackController::onSeekFinished() { pendingSeek_.reset(); }

void PlaybackController::onFileChanged() {
  pendingSeek_.reset();
  lastSubtitleId_ = 0;
}

void PlaybackController::onTracksChanged() {
  for (const Track& t : engine_.tracks()) {
    if (t.type == TrackType::Subtitle && t.selected) lastSubtitleId_ = t.id;
  }
}

}  // namespace chobi::core
