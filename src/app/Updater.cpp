// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/Updater.h"

#include <cstdio>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>

#include "core/Version.h"

namespace chobi {
namespace {

// Enable with QT_LOGGING_RULES="chobi.update=true" when diagnosing update problems.
Q_LOGGING_CATEGORY(lcUpdate, "chobi.update", QtWarningMsg)

constexpr int kTimeoutMs = 20000;
constexpr char kDefaultManifestUrl[] =
    "https://github.com/dev-taosif-net/chobi/releases/latest/download/update.json";
constexpr char kReleasesPage[] = "https://github.com/dev-taosif-net/chobi/releases/latest";

QString platformAssetKey() {
#if defined(Q_OS_WIN)
  return QStringLiteral("windows-x64-setup");
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64)
  return QStringLiteral("linux-x86_64-appimage");
#else
  return QString();
#endif
}

}  // namespace

Updater::Config Updater::defaultConfig() {
  Config c;
  c.currentVersion = QStringLiteral(CHOBI_VERSION);
  c.publicKey = updatePublicKey();
  c.assetKey = platformAssetKey();
  c.releasesPage = QUrl(QString::fromLatin1(kReleasesPage));
  // Forks and the end-to-end test point this elsewhere; a manifest still has to carry a
  // valid signature from the built-in key, so this is not a way to inject updates.
  const QString override = qEnvironmentVariable("CHOBI_UPDATE_MANIFEST_URL");
  c.manifestUrl = QUrl(override.isEmpty() ? QString::fromLatin1(kDefaultManifestUrl) : override);

#if defined(CHOBI_DISABLE_UPDATER)
  c.kind = InstallKind::Disabled;  // distribution packages update through their own channel
  return c;
#endif
  if (c.publicKey.size() != 32) {
    c.kind = InstallKind::Disabled;  // this build has no release key (local/dev build)
    return c;
  }

  c.kind = InstallKind::Manual;
#if defined(Q_OS_LINUX)
  const QString appImage = qEnvironmentVariable("APPIMAGE");
  if (!appImage.isEmpty() && QFileInfo(appImage).isFile() &&
      QFileInfo(QFileInfo(appImage).absolutePath()).isWritable()) {
    c.kind = InstallKind::AppImage;
    c.appImagePath = QFileInfo(appImage).absoluteFilePath();
  }
#elif defined(Q_OS_WIN)
  // The installer drops this marker; the portable zip doesn't have it.
  if (QFileInfo::exists(QCoreApplication::applicationDirPath() +
                        QStringLiteral("/chobi-install.ini"))) {
    c.kind = InstallKind::WindowsInstaller;
    c.downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  }
#endif
  return c;
}

Updater::Updater(Config config, QObject* parent) : QObject(parent), config_(std::move(config)) {}

Updater::~Updater() { cancel(); }

bool Updater::canInstallInApp() const {
  return config_.kind == InstallKind::AppImage || config_.kind == InstallKind::WindowsInstaller;
}

QNetworkAccessManager* Updater::network() {
  if (!network_) network_ = new QNetworkAccessManager(this);
  return network_;
}

QNetworkReply* Updater::get(const QUrl& url) {
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("Chobi/") + config_.currentVersion);
  request.setTransferTimeout(kTimeoutMs);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);  // GitHub redirects to its CDN
  return network()->get(request);
}

void Updater::check(bool userInitiated) {
  if (!enabled() || manifestReply_ || signatureReply_) return;
  userInitiated_ = userInitiated;
  qCInfo(lcUpdate) << "checking" << config_.manifestUrl.toString() << "current"
                   << config_.currentVersion;
  manifestData_.clear();
  signatureData_.clear();
  checkError_.clear();
  manifestReply_ = get(config_.manifestUrl);
  signatureReply_ = get(QUrl(config_.manifestUrl.toString() + QStringLiteral(".sig")));
  for (QNetworkReply* reply : {manifestReply_.data(), signatureReply_.data()}) {
    connect(reply, &QNetworkReply::finished, this, &Updater::onCheckReplyFinished);
  }
}

void Updater::onCheckReplyFinished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply) return;
  if (reply->error() != QNetworkReply::NoError && checkError_.isEmpty()) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // HTTP/2 has no reason phrase, so errorString() alone can end in "server replied: ".
    checkError_ = status > 0 ? tr("HTTP %1").arg(status) : reply->errorString();
  }
  (reply == manifestReply_ ? manifestData_ : signatureData_) = reply->readAll();
  reply->deleteLater();
  if (reply == manifestReply_) manifestReply_.clear();
  if (reply == signatureReply_) signatureReply_.clear();
  if (manifestReply_ || signatureReply_) return;  // wait for the other one

  if (!checkError_.isEmpty()) {
    qCInfo(lcUpdate) << "check failed:" << checkError_;
    emit checkFailed(checkError_, userInitiated_);
    return;
  }
  if (!verifyManifestSignature(manifestData_, signatureData_, config_.publicKey)) {
    qCWarning(lcUpdate) << "manifest signature does not verify; ignoring it";
    emit checkFailed(tr("the update information is not signed by the Chobi release key"),
                     userInitiated_);
    return;
  }
  QString error;
  std::optional<UpdateManifest> manifest = parseManifest(manifestData_, &error);
  if (!manifest) {
    emit checkFailed(error, userInitiated_);
    return;
  }
  if (!core::isNewerVersion(manifest->version.toStdString(), config_.currentVersion.toStdString())) {
    latest_.reset();
    qCInfo(lcUpdate) << "up to date; latest release is" << manifest->version;
    emit upToDate(userInitiated_);
    return;
  }
  latest_ = std::move(manifest);
  qCInfo(lcUpdate) << "update available:" << latest_->version;
  emit updateAvailable(latest_->version, userInitiated_);
}

void Updater::install() {
  if (!latest_ || downloadReply_) return;
  if (!canInstallInApp() || !latest_->assets.contains(config_.assetKey)) {
    QDesktopServices::openUrl(latest_->notesUrl.isValid() ? latest_->notesUrl
                                                          : config_.releasesPage);
    return;
  }
  startDownload();
}

void Updater::startDownload() {
  const UpdateAsset asset = latest_->assets.value(config_.assetKey);
  QString target;
  if (config_.kind == InstallKind::AppImage) {
    // Same directory as the AppImage, so the final rename is atomic.
    const QFileInfo app(config_.appImagePath);
    target = app.absolutePath() + QStringLiteral("/.") + app.fileName() + QStringLiteral(".update");
  } else {
    QDir().mkpath(config_.downloadDir);
    target = config_.downloadDir + QStringLiteral("/Chobi-") + latest_->version +
             QStringLiteral("-setup.exe");
  }

  delete downloadFile_;
  downloadFile_ = new QFile(target);
  if (!downloadFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    abortDownload(tr("cannot write %1").arg(target));
    return;
  }
  downloadHash_.reset();
  downloadedBytes_ = 0;

  qCInfo(lcUpdate) << "downloading" << asset.url.toString() << "to" << target;
  downloadReply_ = get(asset.url);
  connect(downloadReply_, &QNetworkReply::readyRead, this, [this] {
    const QByteArray chunk = downloadReply_->readAll();
    downloadedBytes_ += chunk.size();
    downloadHash_.addData(chunk);
    downloadFile_->write(chunk);
  });
  connect(downloadReply_, &QNetworkReply::downloadProgress, this,
          [this, asset](qint64 received, qint64 total) {
            emit downloadProgress(received, total > 0 ? total : asset.size);
          });
  connect(downloadReply_, &QNetworkReply::finished, this, &Updater::onDownloadFinished);
}

void Updater::onDownloadFinished() {
  QNetworkReply* reply = downloadReply_;
  downloadReply_.clear();
  if (!reply || !downloadFile_) return;
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError) {
    abortDownload(reply->errorString());
    return;
  }
  const QByteArray rest = reply->readAll();
  downloadedBytes_ += rest.size();
  downloadHash_.addData(rest);
  downloadFile_->write(rest);
  downloadFile_->close();

  const UpdateAsset asset = latest_->assets.value(config_.assetKey);
  if (downloadedBytes_ != asset.size || downloadHash_.result().toHex() != asset.sha256) {
    abortDownload(tr("the download is corrupt (size or checksum mismatch)"));
    return;
  }
  const QString path = downloadFile_->fileName();
  delete downloadFile_;
  downloadFile_ = nullptr;
  finishInstall(path);
}

void Updater::finishInstall(const QString& downloadedPath) {
  if (config_.kind == InstallKind::AppImage) {
    QFile::setPermissions(downloadedPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                              QFile::ReadGroup | QFile::ExeGroup |
                                              QFile::ReadOther | QFile::ExeOther);
    // POSIX rename replaces the old file atomically; the running process keeps its copy.
    const QByteArray from = QFile::encodeName(downloadedPath);
    const QByteArray to = QFile::encodeName(config_.appImagePath);
    if (std::rename(from.constData(), to.constData()) != 0) {
      QFile::remove(downloadedPath);
      emit installFailed(tr("cannot replace %1").arg(config_.appImagePath));
      return;
    }
    emit installed(latest_->version, config_.appImagePath);
    return;
  }

  // Windows: per-user Inno Setup installer, no UAC prompt. It closes and relaunches Chobi.
  const QStringList args = {QStringLiteral("/SILENT"), QStringLiteral("/SUPPRESSMSGBOXES"),
                            QStringLiteral("/NORESTART"), QStringLiteral("/CLOSEAPPLICATIONS")};
  if (!QProcess::startDetached(downloadedPath, args)) {
    emit installFailed(tr("cannot start the installer"));
    return;
  }
  emit installerStarted();
}

void Updater::abortDownload(const QString& message) {
  qCWarning(lcUpdate) << "update failed:" << message;
  if (downloadFile_) {
    downloadFile_->close();
    downloadFile_->remove();
    delete downloadFile_;
    downloadFile_ = nullptr;
  }
  emit installFailed(message);
}

void Updater::cancel() {
  for (QNetworkReply* reply : {manifestReply_.data(), signatureReply_.data()}) {
    if (reply) {
      reply->disconnect(this);
      reply->abort();
      reply->deleteLater();
    }
  }
  manifestReply_.clear();
  signatureReply_.clear();
  if (QNetworkReply* reply = downloadReply_) {
    downloadReply_.clear();
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
    if (downloadFile_) {
      downloadFile_->close();
      downloadFile_->remove();
      delete downloadFile_;
      downloadFile_ = nullptr;
    }
  }
}

}  // namespace chobi
