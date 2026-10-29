// SPDX-License-Identifier: GPL-3.0-or-later
// Updater end to end over file:// URLs with a throwaway signing key: signature checks, version
// comparison, download verification and the AppImage self-replace; plus the update bar.
#include <QtTest>

#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "FakeEngine.h"
#include "app/MainWindow.h"
#include "app/UpdateManifest.h"
#include "app/Updater.h"

extern "C" {
#include "monocypher-ed25519.h"
}

using chobi::Updater;

namespace {

struct TestKey {
  uint8_t secret[64];
  uint8_t pub[32];
  TestKey() {
    uint8_t seed[32];
    for (int i = 0; i < 32; ++i) seed[i] = static_cast<uint8_t>(i * 7 + 1);
    crypto_ed25519_key_pair(secret, pub, seed);
  }
  QByteArray publicKey() const { return QByteArray(reinterpret_cast<const char*>(pub), 32); }
  QByteArray sign(const QByteArray& message) const {
    uint8_t sig[64];
    crypto_ed25519_sign(sig, secret, reinterpret_cast<const uint8_t*>(message.constData()),
                        static_cast<size_t>(message.size()));
    return QByteArray(reinterpret_cast<const char*>(sig), 64).toBase64();
  }
};

void writeFile(const QString& path, const QByteArray& data) {
  QFile f(path);
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write(data);
}

QByteArray readFile(const QString& path) {
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

}  // namespace

class UpdaterTest : public QObject {
  Q_OBJECT

  QTemporaryDir dir;
  TestKey key;
  QByteArray newBuild = "#!/bin/sh\necho 'Chobi 9.9.9'\n";

  // Publishes a release into `dir`: the asset, update.json and (optionally) its signature.
  QUrl publish(const QString& version, const QByteArray& asset, bool sign = true,
               QByteArray overrideSha = {}) {
    const QString assetPath = dir.filePath("Chobi-" + version + "-x86_64.AppImage");
    writeFile(assetPath, asset);
    QJsonObject a{{"url", QUrl::fromLocalFile(assetPath).toString()},
                  {"sha256", QString::fromLatin1(overrideSha.isEmpty()
                                 ? QCryptographicHash::hash(asset, QCryptographicHash::Sha256).toHex()
                                 : overrideSha)},
                  {"size", static_cast<double>(asset.size())}};
    QJsonObject root{{"schema", 1},
                     {"version", version},
                     {"notes_url", "https://example.invalid/notes"},
                     {"assets", QJsonObject{{"linux-x86_64-appimage", a}}}};
    const QByteArray json = QJsonDocument(root).toJson();
    const QString manifest = dir.filePath("update.json");
    writeFile(manifest, json);
    if (sign) {
      writeFile(manifest + ".sig", key.sign(json));
    } else {
      QFile::remove(manifest + ".sig");
    }
    return QUrl::fromLocalFile(manifest);
  }

  Updater::Config config(const QUrl& manifest) {
    Updater::Config c;
    c.kind = Updater::InstallKind::AppImage;
    c.manifestUrl = manifest;
    c.publicKey = key.publicKey();
    c.currentVersion = "0.2.0";
    c.assetKey = "linux-x86_64-appimage";
    c.appImagePath = dir.filePath("Chobi.AppImage");
    return c;
  }

 private slots:
  void init() { writeFile(dir.filePath("Chobi.AppImage"), "old build"); }

  void signatureVerification() {
    const QByteArray msg = "{\"schema\":1}";
    QVERIFY(chobi::verifyManifestSignature(msg, key.sign(msg), key.publicKey()));
    QVERIFY(!chobi::verifyManifestSignature(msg + " ", key.sign(msg), key.publicKey()));
    QVERIFY(!chobi::verifyManifestSignature(msg, "bm90IGEgc2lnbmF0dXJl", key.publicKey()));
    QVERIFY(!chobi::verifyManifestSignature(msg, key.sign(msg), QByteArray(32, '\0')));
    QVERIFY(!chobi::verifyManifestSignature(msg, key.sign(msg), QByteArray()));
  }

  void manifestValidation() {
    QString error;
    QVERIFY(!chobi::parseManifest("not json", &error));
    QVERIFY(!chobi::parseManifest(R"({"schema":2,"version":"1.0.0","assets":{}})", &error));
    QVERIFY(!chobi::parseManifest(R"({"schema":1,"version":"one","assets":{}})", &error));
    QVERIFY(!chobi::parseManifest(
        R"({"schema":1,"version":"1.0.0","assets":{"x":{"url":"http://insecure/x","sha256":"00","size":1}}})",
        &error));
    QVERIFY(chobi::parseManifest(R"({"schema":1,"version":"1.0.0","assets":{}})", &error));
  }

  void newerVersionIsOffered() {
    Updater u(config(publish("0.3.0", newBuild)));
    QSignalSpy available(&u, &Updater::updateAvailable);
    u.check(true);
    QVERIFY(available.wait(5000));
    QCOMPARE(available.first().at(0).toString(), QString("0.3.0"));
    QCOMPARE(u.availableVersion(), QString("0.3.0"));
  }

  void sameOrOlderVersionIsUpToDate() {
    for (const char* v : {"0.2.0", "0.1.5"}) {
      Updater u(config(publish(v, newBuild)));
      QSignalSpy upToDate(&u, &Updater::upToDate);
      u.check(false);
      QVERIFY2(upToDate.wait(5000), v);
    }
  }

  void unsignedOrForgedManifestIsRejected() {
    {
      Updater u(config(publish("0.3.0", newBuild, /*sign=*/false)));
      QSignalSpy failed(&u, &Updater::checkFailed);
      u.check(true);
      QVERIFY(failed.wait(5000));
    }
    {
      const QUrl url = publish("0.3.0", newBuild);
      writeFile(url.toLocalFile(), readFile(url.toLocalFile()).replace("0.3.0", "6.6.6"));
      Updater u(config(url));
      QSignalSpy failed(&u, &Updater::checkFailed);
      QSignalSpy available(&u, &Updater::updateAvailable);
      u.check(true);
      QVERIFY(failed.wait(5000));
      QVERIFY(failed.first().at(0).toString().contains("signed"));
      QVERIFY(available.isEmpty());
    }
  }

  void appImageIsReplacedAfterVerifiedDownload() {
#if !defined(Q_OS_LINUX)
    QSKIP("AppImage self-replacement is Linux-only");
#endif
    Updater u(config(publish("0.3.0", newBuild)));
    QSignalSpy available(&u, &Updater::updateAvailable);
    QSignalSpy installed(&u, &Updater::installed);
    u.check(false);
    QVERIFY(available.wait(5000));
    u.install();
    QVERIFY(installed.wait(5000));
    const QString app = dir.filePath("Chobi.AppImage");
    QCOMPARE(installed.first().at(1).toString(), app);
    QCOMPARE(readFile(app), newBuild);
    QVERIFY(QFileInfo(app).isExecutable());
    QVERIFY(!QFile::exists(dir.filePath(".Chobi.AppImage.update")));  // no leftovers
  }

  void corruptDownloadLeavesTheOldBuildAlone() {
    Updater u(config(publish("0.3.0", newBuild, true, QByteArray(64, 'a'))));
    QSignalSpy available(&u, &Updater::updateAvailable);
    QSignalSpy failed(&u, &Updater::installFailed);
    u.check(false);
    QVERIFY(available.wait(5000));
    u.install();
    QVERIFY(failed.wait(5000));
    QCOMPARE(readFile(dir.filePath("Chobi.AppImage")), QByteArray("old build"));
    QVERIFY(!QFile::exists(dir.filePath(".Chobi.AppImage.update")));
  }

  void defaultConfigNeedsTheReleaseKey() {
    const Updater::Config c = Updater::defaultConfig();
    if (chobi::updatePublicKey().size() == 32) {
      QVERIFY(c.kind != Updater::InstallKind::Disabled);
    } else {
      QCOMPARE(c.kind, Updater::InstallKind::Disabled);  // dev build without a key
    }
    QCOMPARE(c.currentVersion, QStringLiteral(CHOBI_VERSION));
  }

  void updateBarAppearsInTheWindow() {
    FakeEngine engine;
    Updater u(config(publish("0.3.0", newBuild)));
    chobi::MainWindow window(&engine, new QWidget, {nullptr, nullptr, {}, {}, &u});
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QSignalSpy available(&u, &Updater::updateAvailable);
    u.check(true);
    QVERIFY(available.wait(5000));
    QTRY_VERIFY(window.updateBarText().contains("0.3.0"));
  }
};

QTEST_MAIN(UpdaterTest)
#include "test_updater.moc"
