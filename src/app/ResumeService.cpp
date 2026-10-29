// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/ResumeService.h"

#include <utility>
#include <vector>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>

namespace chobi {
namespace {

// The file at `path` is gone or is no longer the file the key was made for.
bool isDeletedOrReplaced(const std::string& key, const QString& path) {
  const QFileInfo info(path);
  if (!info.isFile()) return true;
  return core::ResumeStore::makeKey(info.fileName().toStdString(),
                                    static_cast<std::uint64_t>(info.size())) != key;
}

}  // namespace

ResumeService::ResumeService(QString filePath, Clock clock)
    : path_(std::move(filePath)), clock_(std::move(clock)) {}

std::int64_t ResumeService::now() const {
  return clock_ ? clock_() : QDateTime::currentSecsSinceEpoch();
}

void ResumeService::ensureLoadedLocked() {
  if (loaded_) return;
  loaded_ = true;
  QFile file(path_);
  if (file.open(QIODevice::ReadOnly)) {
    const QByteArray data = file.read(4 * 1024 * 1024);  // the store is capped far below this
    store_.parse(std::string_view(data.constData(), static_cast<std::size_t>(data.size())),
                 now());
  }
}

void ResumeService::pruneExpiredLocked() {
  if (store_.pruneExpired(now())) writeLocked();
}

std::optional<double> ResumeService::lookup(const std::string& key) {
  QMutexLocker lock(&mutex_);
  ensureLoadedLocked();
  pruneExpiredLocked();
  return store_.lookup(key, now());
}

void ResumeService::record(const std::string& key, const QString& videoPath, double position,
                           double duration) {
  // Deleted while it was playing: forget it instead of saving a position for a missing file.
  const bool gone = !videoPath.isEmpty() && isDeletedOrReplaced(key, videoPath);

  QMutexLocker lock(&mutex_);
  ensureLoadedLocked();
  pruneExpiredLocked();
  const bool known = store_.lookup(key, now()).has_value();
  if (gone) {
    if (known) {
      store_.remove(key);
      writeLocked();
    }
    return;
  }
  if (!known && !core::ResumeStore::shouldRemember(position, duration)) return;  // nothing to do
  store_.record(key, videoPath.toStdString(), position, duration, now());
  writeLocked();
}

void ResumeService::clear() {
  QMutexLocker lock(&mutex_);
  loaded_ = true;  // nothing on disk is worth reading any more
  store_ = core::ResumeStore();
  writeLocked();
}

int ResumeService::pruneDeletedVideos() {
  std::vector<std::pair<std::string, QString>> candidates;
  {
    QMutexLocker lock(&mutex_);
    ensureLoadedLocked();
    pruneExpiredLocked();
    for (const core::ResumeStore::Entry& e : store_.entries()) {
      if (!e.path.empty()) candidates.emplace_back(e.key, QString::fromStdString(e.path));
    }
  }

  // Filesystem checks happen unlocked: a slow network path must not block a lookup.
  std::vector<std::string> stale;
  for (const auto& [key, path] : candidates) {
    if (!QFileInfo(QFileInfo(path).absolutePath()).isDir()) continue;  // drive/share offline
    if (isDeletedOrReplaced(key, path)) stale.push_back(key);
  }
  if (stale.empty()) return 0;

  QMutexLocker lock(&mutex_);
  for (const std::string& key : stale) store_.remove(key);
  writeLocked();
  return static_cast<int>(stale.size());
}

void ResumeService::writeLocked() {
  QDir().mkpath(QFileInfo(path_).absolutePath());
  QSaveFile file(path_);
  if (!file.open(QIODevice::WriteOnly)) return;
  const std::string text = store_.serialize();
  file.write(text.data(), static_cast<qint64>(text.size()));
  file.commit();
}

}  // namespace chobi
