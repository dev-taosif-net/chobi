// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/UpdateManifest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/Version.h"

extern "C" {
#include "monocypher-ed25519.h"
}

namespace chobi {
namespace {

// Public half of the release signing key (docs/releasing.md). Base64 of 32 raw bytes.
constexpr char kUpdatePublicKeyBase64[] = CHOBI_UPDATE_PUBLIC_KEY;

bool isHex64(const QByteArray& s) {
  if (s.size() != 64) return false;
  for (char c : s) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

}  // namespace

QByteArray updatePublicKey() { return QByteArray::fromBase64(kUpdatePublicKeyBase64); }

bool verifyManifestSignature(const QByteArray& manifest, const QByteArray& signatureBase64,
                             const QByteArray& publicKey) {
  const QByteArray signature = QByteArray::fromBase64(signatureBase64.trimmed());
  if (signature.size() != 64 || publicKey.size() != 32) return false;
  return crypto_ed25519_check(reinterpret_cast<const uint8_t*>(signature.constData()),
                              reinterpret_cast<const uint8_t*>(publicKey.constData()),
                              reinterpret_cast<const uint8_t*>(manifest.constData()),
                              static_cast<size_t>(manifest.size())) == 0;
}

std::optional<UpdateManifest> parseManifest(const QByteArray& json, QString* error) {
  auto fail = [error](const QString& why) -> std::optional<UpdateManifest> {
    if (error) *error = why;
    return std::nullopt;
  };

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
  if (!doc.isObject()) return fail(QStringLiteral("malformed manifest: ") + parseError.errorString());
  const QJsonObject root = doc.object();
  if (root.value(QLatin1String("schema")).toInt() != 1) return fail(QStringLiteral("unknown schema"));

  UpdateManifest m;
  m.version = root.value(QLatin1String("version")).toString();
  if (!core::parseVersion(m.version.toStdString())) return fail(QStringLiteral("bad version"));
  m.notesUrl = QUrl(root.value(QLatin1String("notes_url")).toString());

  const QJsonObject assets = root.value(QLatin1String("assets")).toObject();
  for (auto it = assets.begin(); it != assets.end(); ++it) {
    const QJsonObject a = it.value().toObject();
    UpdateAsset asset;
    asset.url = QUrl(a.value(QLatin1String("url")).toString());
    asset.sha256 = a.value(QLatin1String("sha256")).toString().toLatin1().toLower();
    asset.size = static_cast<qint64>(a.value(QLatin1String("size")).toDouble());
    const QString scheme = asset.url.scheme();
    if (!asset.url.isValid() || (scheme != QLatin1String("https") && scheme != QLatin1String("file"))) {
      return fail(QStringLiteral("bad url for ") + it.key());
    }
    if (!isHex64(asset.sha256) || asset.size <= 0) return fail(QStringLiteral("bad hash/size for ") + it.key());
    m.assets.insert(it.key(), asset);
  }
  return m;
}

}  // namespace chobi
