// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

namespace chobi {

// update.json, published with every release next to update.json.sig (an Ed25519 signature of
// the exact file bytes, base64). Example:
//   {
//     "schema": 1,
//     "version": "0.3.0",
//     "notes_url": "https://github.com/dev-taosif-net/chobi/releases/tag/v0.3.0",
//     "assets": {
//       "linux-x86_64-appimage": { "url": "https://…AppImage", "sha256": "…", "size": 123 },
//       "windows-x64-setup":     { "url": "https://…setup.exe", "sha256": "…", "size": 456 }
//     }
//   }
struct UpdateAsset {
  QUrl url;
  QByteArray sha256;  // lowercase hex, 64 chars
  qint64 size = 0;
};

struct UpdateManifest {
  QString version;
  QUrl notesUrl;
  QMap<QString, UpdateAsset> assets;  // keyed by platform asset id
};

// Raw 32-byte Ed25519 public key that release manifests are signed with.
QByteArray updatePublicKey();

bool verifyManifestSignature(const QByteArray& manifest, const QByteArray& signatureBase64,
                             const QByteArray& publicKey);

// Validates structure only; call verifyManifestSignature first.
std::optional<UpdateManifest> parseManifest(const QByteArray& json, QString* error);

}  // namespace chobi
