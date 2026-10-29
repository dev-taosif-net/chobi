// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/MediaPath.h"

#include <QDir>
#include <QFileInfo>

namespace chobi {
namespace {

bool isNetworkScheme(const QString& scheme) {
  static const QStringList schemes = {
      QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("rtsp"),
      QStringLiteral("rtsps"), QStringLiteral("rtmp"), QStringLiteral("rtmps"),
      QStringLiteral("rtp"), QStringLiteral("udp"), QStringLiteral("srt"),
      QStringLiteral("ftp"), QStringLiteral("mms")};
  return schemes.contains(scheme.toLower());
}

}  // namespace

std::optional<MediaSource> mediaSourceFromUrl(const QUrl& url) {
  if (!url.isValid() || url.isEmpty()) return std::nullopt;
  if (url.isLocalFile()) {
    const QString path = url.toLocalFile();
    if (path.isEmpty()) return std::nullopt;
    return MediaSource{QDir::cleanPath(QFileInfo(path).absoluteFilePath()), true};
  }
  if (isNetworkScheme(url.scheme())) {
    return MediaSource{url.toString(QUrl::FullyEncoded), false};
  }
  return std::nullopt;
}

std::optional<MediaSource> mediaSourceFromArgument(const QString& argument) {
  if (argument.isEmpty()) return std::nullopt;
  if (argument.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
    return mediaSourceFromUrl(QUrl(argument));
  }
  // "C:\video.mkv" parses as scheme "c"; real URL schemes are longer than one letter.
  const QUrl url(argument);
  if (url.scheme().size() > 1 && isNetworkScheme(url.scheme())) {
    return mediaSourceFromUrl(url);
  }
  return MediaSource{QDir::cleanPath(QFileInfo(argument).absoluteFilePath()), true};
}

QString lowerSuffix(const QString& path) {
  const int slash = qMax(path.lastIndexOf(QLatin1Char('/')), path.lastIndexOf(QLatin1Char('\\')));
  const int dot = path.lastIndexOf(QLatin1Char('.'));
  if (dot <= slash + 1) return {};
  return path.mid(dot + 1).toLower();
}

}  // namespace chobi
