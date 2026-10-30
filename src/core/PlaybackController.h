// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "core/IPlayerEngine.h"
#include "core/KeyMap.h"

namespace chobi::core {

// Window-management hooks the controller needs. Implemented by the main window and mocked in
// tests, so the Backspace ordering (pause -> leave fullscreen -> minimize) is verifiable headless.
class IWindowController {
 public:
  virtual ~IWindowController() = default;
  virtual bool isFullscreen() const = 0;
  virtual void setFullscreen(bool fullscreen) = 0;
  virtual void minimize() = 0;
  virtual void requestOpenFile() = 0;
  virtual void requestQuit() = 0;
  virtual void showMessage(const std::string& text) = 0;
  // Shows the playback position on the OSD and keeps it current until the OSD hides, like
  // mpv's seek OSD. A keyframe seek lands past its target, so a fixed text would be wrong.
  virtual void showPosition() = 0;
};

inline constexpr double kSeekStepSeconds = 5.0;
inline constexpr double kVolumeStep = 5.0;

// Clamps current + delta into [0, duration]. A duration <= 0 means "unknown" and only the
// lower bound applies.
double clampSeekTarget(double current, double delta, double duration);
double clampVolume(double volume);

// Turns user actions into engine commands. Holds no UI types.
class PlaybackController {
 public:
  PlaybackController(IPlayerEngine& engine, IWindowController& window);

  void trigger(Action action);

  void seekBy(double deltaSeconds);
  void seekTo(double seconds);
  void changeVolume(double delta);
  void togglePause();
  void toggleMute();
  void toggleSubtitles();
  void cycleAudioTrack();
  void pauseAndMinimize();
  void toggleFullscreen();

  // Engine notifications.
  void onSeekFinished();
  void onFileChanged();
  void onTracksChanged();

 private:
  IPlayerEngine& engine_;
  IWindowController& window_;
  std::optional<double> pendingSeek_;  // target of an in-flight seek, so repeated keys add up
  std::int64_t lastSubtitleId_ = 0;
};

}  // namespace chobi::core
