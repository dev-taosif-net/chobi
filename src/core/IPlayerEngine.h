// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/Track.h"

namespace chobi::core {

struct PlaybackState {
  bool loaded = false;  // a file is open and its streams are known
  bool paused = false;
  bool muted = false;
  bool eofReached = false;
  bool subtitlesVisible = true;
  bool buffering = false;
  double position = 0;  // seconds
  double duration = 0;  // seconds, 0 when unknown (live streams, broken files)
  double volume = 100;  // 0..100
  double speed = 1;
  std::string mediaTitle;
  std::string hwdec;  // active hardware decoder, "" for software decoding
};

// An external subtitle file to attach to a specific open() call.
struct SubtitleFile {
  std::string path;
  std::string title;
  std::string language;  // normalized tag, may be empty
  bool select = false;
};

// Commands the UI and controller may issue. Implementations must never block the caller:
// with libmpv's render API the UI thread is also the render thread and may only use the
// asynchronous client API.
class IPlayerEngine {
 public:
  virtual ~IPlayerEngine() = default;

  // Subtitles are bound to this particular load, so rapid file switching can never attach
  // one video's subtitles to another.
  virtual void open(const std::string& pathOrUrl, double startSeconds,
                    std::vector<SubtitleFile> subtitles) = 0;
  virtual void stop() = 0;
  virtual void setPaused(bool paused) = 0;
  // exact=false seeks to the nearest keyframe (fast, used while dragging the seek bar).
  virtual void seekAbsolute(double seconds, bool exact) = 0;
  // Stock mpv's arrow-key seek: lands on a keyframe in the seek direction, so it is instant
  // but may overshoot the target. Repeated calls before mpv catches up add up.
  virtual void seekRelative(double deltaSeconds) = 0;
  virtual void setVolume(double volume) = 0;
  virtual void setMuted(bool muted) = 0;
  virtual void setSpeed(double speed) = 0;
  virtual void selectAudioTrack(std::int64_t id) = 0;
  virtual void selectSubtitleTrack(std::int64_t id) = 0;  // 0 disables subtitles
  virtual void setSubtitlesVisible(bool visible) = 0;
  virtual void addSubtitleFile(const std::string& path, bool select, const std::string& title,
                               const std::string& language) = 0;

  virtual const PlaybackState& state() const = 0;
  virtual const std::vector<Track>& tracks() const = 0;
};

}  // namespace chobi::core
