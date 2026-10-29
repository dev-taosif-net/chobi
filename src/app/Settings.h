// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMutex>
#include <QString>

namespace chobi {

struct AppSettings {
  bool resumePlayback = true;  // continue each video where it was stopped
  bool rememberVolume = true;  // start new videos and sessions at the last volume
  double volume = 100;         // last volume (0..100), used when rememberVolume is on

  // Volume the engine should start with.
  double startupVolume() const;
};

// INI file in the user's config directory (Linux: ~/.config/Chobi/settings.ini,
// Windows: %LOCALAPPDATA%/Chobi/settings.ini). load() is one tiny read at startup; save() is
// thread-safe and is called from the thread pool so the UI thread never writes to disk.
class SettingsStore {
 public:
  explicit SettingsStore(QString filePath);

  AppSettings load() const;
  void save(const AppSettings& settings);
  QString filePath() const { return path_; }

 private:
  QString path_;
  mutable QMutex mutex_;
};

}  // namespace chobi
