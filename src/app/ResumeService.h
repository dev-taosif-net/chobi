// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <QMutex>
#include <QString>

#include "core/ResumeStore.h"

namespace chobi {

// Thread-safe, file-backed wrapper around core::ResumeStore. Called from worker threads only,
// so the UI thread never touches the resume file. Saved positions are removed when they are
// older than 7 days (core::ResumeStore::kRetentionSeconds) or when the video was deleted.
class ResumeService {
 public:
  using Clock = std::function<std::int64_t()>;  // Unix seconds; injectable for tests

  explicit ResumeService(QString filePath, Clock clock = {});

  std::optional<double> lookup(const std::string& key);
  void record(const std::string& key, const QString& videoPath, double position,
              double duration);
  void clear();  // forget every saved position

  // Drops positions whose video file was deleted or replaced by a different file. Entries
  // whose whole folder is missing (unplugged drive, offline share) are kept; the 7-day expiry
  // removes them if they never come back. Stats files without holding the lock. Returns the
  // number of entries removed.
  int pruneDeletedVideos();

 private:
  std::int64_t now() const;
  void ensureLoadedLocked();
  void pruneExpiredLocked();
  void writeLocked();

  QMutex mutex_;
  QString path_;
  Clock clock_;
  core::ResumeStore store_;
  bool loaded_ = false;
};

}  // namespace chobi
