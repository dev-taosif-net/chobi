// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <map>

#include <QStringList>

#include "engine/PlayerEngine.h"

struct mpv_handle;
struct mpv_event;
struct mpv_event_property;
struct mpv_node;

namespace chobi {

// libmpv-backed engine. All state reaches the UI through observed properties and events
// (no polling). After construction only the asynchronous, render-thread-safe parts of the
// client API are used, because the GUI thread also drives mpv_render_context_render().
class MpvEngine final : public PlayerEngine {
  Q_OBJECT

 public:
  struct Options {
    bool headless = false;           // vo=null/ao=null, for tests and benchmarks
    QString fontsDir;                // bundled fonts (e.g. Noto Sans Bengali), optional
    QStringList preferredLanguages;  // ISO 639-1, best first; drives subtitle selection
    bool verboseLog = false;         // forward mpv log lines to stderr
    double initialVolume = 100;      // 0..100, e.g. the remembered volume
  };

  explicit MpvEngine(const Options& options, QObject* parent = nullptr);
  ~MpvEngine() override;

  bool isValid() const { return mpv_ != nullptr; }
  QString initError() const { return initError_; }
  mpv_handle* handle() const { return mpv_; }

  // core::IPlayerEngine
  void open(const std::string& pathOrUrl, double startSeconds,
            std::vector<core::SubtitleFile> subtitles) override;
  void stop() override;
  void setPaused(bool paused) override;
  void seekAbsolute(double seconds, bool exact) override;
  void seekRelative(double deltaSeconds) override;
  void setVolume(double volume) override;
  void setMuted(bool muted) override;
  void setSpeed(double speed) override;
  void selectAudioTrack(std::int64_t id) override;
  void selectSubtitleTrack(std::int64_t id) override;
  void setSubtitlesVisible(bool visible) override;
  void addSubtitleFile(const std::string& path, bool select, const std::string& title,
                       const std::string& language) override;
  const core::PlaybackState& state() const override { return state_; }
  const std::vector<core::Track>& tracks() const override { return tracks_; }

 private:
  void processEvents();
  void handleEvent(const mpv_event& event);
  void handleProperty(std::uint64_t id, const mpv_event_property& property);
  void command(std::initializer_list<const char*> args, std::uint64_t replyId = 0);
  void setFlag(const char* name, bool value);
  void setDouble(const char* name, double value);
  void setString(const char* name, const char* value);
  void onLoadReply(std::uint64_t serial, int error, const mpv_node& result);
  void attachSubtitles(std::int64_t entryId);

  mpv_handle* mpv_ = nullptr;
  QString initError_;
  bool verboseLog_ = false;
  std::atomic<bool> wakeupPending_{false};
  core::PlaybackState state_;
  std::vector<core::Track> tracks_;
  QString lastLogError_;
  // Subtitles waiting for their file: keyed by open() serial until mpv tells us the playlist
  // entry id, then by entry id until that entry's file-loaded event.
  std::uint64_t openSerial_ = 0;
  std::map<std::uint64_t, std::vector<core::SubtitleFile>> pendingBySerial_;
  std::map<std::int64_t, std::vector<core::SubtitleFile>> pendingByEntry_;
  std::int64_t currentEntry_ = -1;
};

}  // namespace chobi
