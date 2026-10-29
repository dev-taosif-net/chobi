// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string>

#include <QMutex>
#include <QString>

#include "core/ResumeStore.h"

namespace chobi {

// Thread-safe, file-backed wrapper around core::ResumeStore. Called from worker threads only,
// so the UI thread never touches the resume file.
class ResumeService {
 public:
  explicit ResumeService(QString filePath);

  std::optional<double> lookup(const std::string& key);
  void record(const std::string& key, double position, double duration);
  void clear();  // forget every saved position

 private:
  void ensureLoadedLocked();
  void writeLocked();

  QMutex mutex_;
  QString path_;
  core::ResumeStore store_;
  bool loaded_ = false;
};

}  // namespace chobi
