// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/ResumeService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>

namespace chobi {

ResumeService::ResumeService(QString filePath) : path_(std::move(filePath)) {}

void ResumeService::ensureLoadedLocked() {
  if (loaded_) return;
  loaded_ = true;
  QFile file(path_);
  if (file.open(QIODevice::ReadOnly)) {
    const QByteArray data = file.read(4 * 1024 * 1024);  // the store is capped far below this
    store_.parse(std::string_view(data.constData(), static_cast<std::size_t>(data.size())));
  }
}

std::optional<double> ResumeService::lookup(const std::string& key) {
  QMutexLocker lock(&mutex_);
  ensureLoadedLocked();
  return store_.lookup(key);
}

void ResumeService::record(const std::string& key, double position, double duration) {
  QMutexLocker lock(&mutex_);
  ensureLoadedLocked();
  const bool known = store_.lookup(key).has_value();
  if (!known && !core::ResumeStore::shouldRemember(position, duration)) return;  // nothing to do
  store_.record(key, position, duration);

  QDir().mkpath(QFileInfo(path_).absolutePath());
  QSaveFile file(path_);
  if (!file.open(QIODevice::WriteOnly)) return;
  const std::string text = store_.serialize();
  file.write(text.data(), static_cast<qint64>(text.size()));
  file.commit();
}

}  // namespace chobi
