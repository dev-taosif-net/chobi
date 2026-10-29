// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/MediaProbe.h"

#include <QDir>
#include <QFileInfo>

#include "app/ResumeService.h"
#include "core/MediaFormats.h"
#include "core/ResumeStore.h"

namespace chobi {

ProbeResult probeMedia(const QString& path, const std::vector<std::string>& preferredLanguages,
                       ResumeService* resume) {
  ProbeResult result;
  result.path = path;

  const QFileInfo info(path);
  if (!info.exists()) {
    result.error = QStringLiteral("File not found.");
    return result;
  }
  if (info.isDir()) {
    result.error = QStringLiteral("This is a folder, not a video file.");
    return result;
  }
  result.exists = true;

  const QString fileName = info.fileName();
  result.resumeKey = core::ResumeStore::makeKey(fileName.toStdString(),
                                                static_cast<std::uint64_t>(info.size()));
  if (resume) result.resumePosition = resume->lookup(result.resumeKey);

  QStringList filters;
  for (std::string_view ext : core::autoSubtitleExtensions()) {
    filters << QStringLiteral("*.") + QString::fromLatin1(ext.data(), static_cast<int>(ext.size()));
  }
  const QDir dir = info.absoluteDir();
  const QStringList names = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::NoSort);

  std::vector<std::string> entries;
  entries.reserve(static_cast<std::size_t>(names.size()));
  for (const QString& n : names) entries.push_back(n.toStdString());

  core::SubtitleMatchOptions options;
  options.preferredLanguages = preferredLanguages;
  for (core::SubtitleCandidate& c : core::findSubtitles(fileName.toStdString(), entries, options)) {
    const QString subPath = dir.filePath(QString::fromStdString(c.fileName));
    result.subtitles.push_back({subPath, std::move(c)});
  }
  return result;
}

}  // namespace chobi
