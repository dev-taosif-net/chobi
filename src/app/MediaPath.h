// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>

#include <QString>
#include <QUrl>

namespace chobi {

struct MediaSource {
  QString location;  // absolute local path (Qt '/' separators) or a network URL
  bool isLocalFile = true;
};

// Interprets a command-line argument or "Open With" value: plain or relative paths,
// file:// URIs (percent-encoded, as some file managers pass them) and network URLs.
std::optional<MediaSource> mediaSourceFromArgument(const QString& argument);

// Interprets a dropped URL.
std::optional<MediaSource> mediaSourceFromUrl(const QUrl& url);

QString lowerSuffix(const QString& path);

}  // namespace chobi
