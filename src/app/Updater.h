// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>

#include <QCryptographicHash>
#include <QNetworkReply>  // full type: QPointer<QNetworkReply> needs it on Qt < 6.9
#include <QObject>
#include <QPointer>
#include <QUrl>

#include "app/UpdateManifest.h"

class QFile;
class QNetworkAccessManager;

namespace chobi {

// Checks GitHub Releases for a newer version and installs it.
//
// check(): downloads update.json and update.json.sig (a few hundred bytes), verifies the
// Ed25519 signature with the key built into the app, and compares versions. Nothing about the
// user or their files is sent. install(): downloads the platform asset, checks its size and
// SHA-256 against the signed manifest, then
//   - AppImage (Linux): atomically replaces the running .AppImage file; restart to finish,
//   - Windows installer build: starts the per-user installer silently; the app then quits,
//   - anything else (distro package, portable zip, source build): opens the release page.
class Updater final : public QObject {
  Q_OBJECT

 public:
  enum class InstallKind { Disabled, AppImage, WindowsInstaller, Manual };

  struct Config {
    InstallKind kind = InstallKind::Disabled;
    QUrl manifestUrl;    // signature is fetched from manifestUrl + ".sig"
    QByteArray publicKey;
    QString currentVersion;
    QString assetKey;     // platform asset id in the manifest
    QString appImagePath;  // kind == AppImage: the file to replace
    QString downloadDir;   // kind == WindowsInstaller: where the installer is saved
    QUrl releasesPage;     // fallback for Manual
  };

  // Built from the compile-time key and the running environment ($APPIMAGE, installer marker).
  static Config defaultConfig();

  explicit Updater(Config config, QObject* parent = nullptr);
  ~Updater() override;

  bool enabled() const { return config_.kind != InstallKind::Disabled; }
  InstallKind installKind() const { return config_.kind; }
  bool canInstallInApp() const;
  QString currentVersion() const { return config_.currentVersion; }
  QString availableVersion() const { return latest_ ? latest_->version : QString(); }
  QUrl notesUrl() const {
    return latest_ && latest_->notesUrl.isValid() ? latest_->notesUrl : config_.releasesPage;
  }
  bool busy() const { return manifestReply_ || signatureReply_ || downloadReply_; }

  void check(bool userInitiated);
  void install();
  void cancel();

 signals:
  void updateAvailable(const QString& version, bool userInitiated);
  void upToDate(bool userInitiated);
  void checkFailed(const QString& message, bool userInitiated);
  void downloadProgress(qint64 received, qint64 total);
  void installed(const QString& version, const QString& relaunchPath);  // AppImage: restart
  void installerStarted();  // Windows: the installer runs now; quit the app
  void installFailed(const QString& message);

 private:
  QNetworkAccessManager* network();
  QNetworkReply* get(const QUrl& url);
  void onCheckReplyFinished();
  void startDownload();
  void onDownloadFinished();
  void finishInstall(const QString& downloadedPath);
  void abortDownload(const QString& message);

  Config config_;
  QNetworkAccessManager* network_ = nullptr;  // created on first use only
  QPointer<QNetworkReply> manifestReply_;
  QPointer<QNetworkReply> signatureReply_;
  QPointer<QNetworkReply> downloadReply_;
  QByteArray manifestData_;
  QByteArray signatureData_;
  QString checkError_;
  bool userInitiated_ = false;
  std::optional<UpdateManifest> latest_;
  QFile* downloadFile_ = nullptr;
  QCryptographicHash downloadHash_{QCryptographicHash::Sha256};
  qint64 downloadedBytes_ = 0;
};

}  // namespace chobi
