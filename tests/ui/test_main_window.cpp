// SPDX-License-Identifier: GPL-3.0-or-later
// Drives the real MainWindow (offscreen) with a fake engine: keyboard shortcuts, Esc order,
// drag-and-drop with awkward paths, external subtitle discovery, error display, and the
// "resume where stopped" / "remember volume" settings.
#include <QtTest>

#include <QCheckBox>
#include <QFile>
#include <QMimeData>
#include <QTemporaryDir>
#include <QThreadPool>

#include "FakeEngine.h"
#include "app/MainWindow.h"
#include "app/ResumeService.h"
#include "app/Settings.h"
#include "app/SettingsDialog.h"
#include "core/ResumeStore.h"

using chobi::AppSettings;
using chobi::MainWindow;
using chobi::ResumeService;
using chobi::SettingsStore;

namespace {

void touch(const QString& path) {
  QFile f(path);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write("x");
}

void drop(QWidget* target, const QList<QUrl>& urls) {
  auto* mime = new QMimeData;
  mime->setUrls(urls);
  const QPointF pos(target->width() / 2.0, target->height() / 2.0);
  QDragEnterEvent enter(pos.toPoint(), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(target, &enter);
  QDropEvent dropEvent(pos, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(target, &dropEvent);
  delete mime;
}

}  // namespace

class MainWindowTest : public QObject {
  Q_OBJECT

  FakeEngine* engine = nullptr;
  MainWindow* window = nullptr;
  QTemporaryDir dir;
  std::shared_ptr<ResumeService> resume;
  std::shared_ptr<SettingsStore> store;

  void createWindow(const AppSettings& settings) {
    delete window;
    resume = std::make_shared<ResumeService>(dir.filePath("resume.tsv"));
    store = std::make_shared<SettingsStore>(dir.filePath("settings.ini"));
    window = new MainWindow(engine, new QWidget, {resume, store, settings, {"bn", "en"}});
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
  }

  // touch() writes 1 byte, so the resume key is "1:<name>"; pre-seed a position for it.
  void seedPosition(const QString& fileName, double seconds) {
    resume->record(chobi::core::ResumeStore::makeKey(fileName.toStdString(), 1), seconds, 0);
  }

  // Opens `path`, waits for the background probe and fakes a successful load.
  void openAndLoad(const QString& path, double duration = 100) {
    QSignalSpy probed(window, &MainWindow::probeFinished);
    window->openLocation(path);
    QVERIFY(probed.wait(5000));
    QVERIFY(!engine->opened.empty());
    engine->finishLoading(duration);
  }

 private slots:
  void init() {
    engine = new FakeEngine;
    QFile::remove(dir.filePath("resume.tsv"));
    QFile::remove(dir.filePath("settings.ini"));
    createWindow(AppSettings{});
  }

  void cleanup() {
    delete window;
    window = nullptr;
    delete engine;
  }

  void idleHintAndKeysWithoutFile() {
    QVERIFY(window->centerText().contains("Drop a video"));
    QTest::keyClick(window, Qt::Key_Right);
    QTest::keyClick(window, Qt::Key_Space);
    QVERIFY(engine->seeks.empty());
  }

  void seekAndVolumeKeys() {
    const QString video = dir.filePath("clip.mp4");
    touch(video);
    openAndLoad(video);
    engine->s.position = 50;
    QTest::keyClick(window, Qt::Key_Right);
    QCOMPARE(engine->seeks.back(), 55.0);
    emit engine->playbackRestarted();
    QTest::keyClick(window, Qt::Key_Left);
    QCOMPARE(engine->seeks.back(), 50.0);

    engine->s.volume = 97;
    QTest::keyClick(window, Qt::Key_Up);
    QCOMPARE(engine->s.volume, 100.0);
    QTest::keyClick(window, Qt::Key_Down);
    QCOMPARE(engine->s.volume, 95.0);
  }

  void spaceMuteFullscreen() {
    const QString video = dir.filePath("clip2.mkv");
    touch(video);
    openAndLoad(video);
    engine->s.paused = false;
    QTest::keyClick(window, Qt::Key_Space);
    QVERIFY(engine->s.paused);
    QTest::keyClick(window, Qt::Key_M);
    QVERIFY(engine->s.muted);
    QTest::keyClick(window, Qt::Key_F);
    QTRY_VERIFY(window->isFullscreen());
    QTest::keyClick(window, Qt::Key_F);
    QTRY_VERIFY(!window->isFullscreen());
  }

  void escapePausesLeavesFullscreenAndMinimizes() {
    const QString video = dir.filePath("clip3.mkv");
    touch(video);
    openAndLoad(video);
    engine->s.paused = false;
    window->setFullscreen(true);
    QTRY_VERIFY(window->isFullscreen());
    QTest::keyClick(window, Qt::Key_Escape);
    QVERIFY(engine->s.paused);
    QTRY_VERIFY(!window->isFullScreen());
    QTRY_VERIFY(window->windowState().testFlag(Qt::WindowMinimized));
    QVERIFY(window->isVisible());  // minimized, never closed
  }

  void ctrlQCloses() {
    QTest::keyClick(window, Qt::Key_Q, Qt::ControlModifier);
    QTRY_VERIFY(!window->isVisible());
  }

  void dropUnicodePathWithSpacesAndDiscoverSubtitles() {
    const QString base = QString::fromUtf8(u8"ছবি টেস্ট ✓ 100% Movie");
    const QString video = dir.filePath(base + ".mkv");
    touch(video);
    touch(dir.filePath(base + ".en.srt"));
    touch(dir.filePath(base + ".bn.srt"));
    touch(dir.filePath(base + " Part2.srt"));  // unrelated
    touch(dir.filePath("Other.srt"));          // unrelated

    QSignalSpy probed(window, &MainWindow::probeFinished);
    drop(window, {QUrl::fromLocalFile(video)});
    QVERIFY(probed.wait(5000));

    QCOMPARE(QString::fromStdString(engine->opened.back().location), video);
    const auto& subs = engine->opened.back().subtitles;
    QCOMPARE(subs.size(), std::size_t{2});
    QCOMPARE(QString::fromStdString(subs[0].path), dir.filePath(base + ".bn.srt"));  // preferred
    QVERIFY(subs[0].select);
    QCOMPARE(subs[0].language, std::string("bn"));
    QVERIFY(!subs[1].select);
  }

  void dropSwitchesCleanlyWhilePlaying() {
    const QString a = dir.filePath("a.mp4");
    const QString b = dir.filePath("b.mp4");
    touch(a);
    touch(b);
    openAndLoad(a);
    QSignalSpy probed(window, &MainWindow::probeFinished);
    drop(window, {QUrl::fromLocalFile(b)});
    QVERIFY(probed.wait(5000));
    QCOMPARE(QString::fromStdString(engine->opened.back().location), b);
    QCOMPARE(window->currentLocation(), b);
  }

  void cliArgumentFormsResolveToTheSameFile() {
    const QString video = dir.filePath("space name.mp4");
    touch(video);
    for (const QString& arg : {video, QUrl::fromLocalFile(video).toString()}) {
      QSignalSpy probed(window, &MainWindow::probeFinished);
      window->openLocation(arg);
      QVERIFY(probed.wait(5000));
      QCOMPARE(QString::fromStdString(engine->opened.back().location), video);
    }
  }

  void missingFileShowsError() {
    QSignalSpy probed(window, &MainWindow::probeFinished);
    window->openLocation(dir.filePath("does-not-exist.mkv"));
    QVERIFY(probed.wait(5000));
    QVERIFY(engine->opened.empty());
    QVERIFY(window->centerText().contains("Cannot play"));
    QVERIFY(window->centerText().contains("not found"));
  }

  void engineFailureShowsError() {
    const QString video = dir.filePath("broken.mkv");
    touch(video);
    QSignalSpy probed(window, &MainWindow::probeFinished);
    window->openLocation(video);
    QVERIFY(probed.wait(5000));
    emit engine->fileFailed("Unsupported or unrecognized file format.");
    QVERIFY(window->centerText().contains("Unsupported"));
  }

  // --- Settings: resume where stopped -------------------------------------------------------

  void resumeOnStartsFromSavedPosition() {
    const QString video = dir.filePath("movie.mkv");
    touch(video);  // 1 byte
    seedPosition("movie.mkv", 754);
    QSignalSpy probed(window, &MainWindow::probeFinished);
    window->openLocation(video);
    QVERIFY(probed.wait(5000));
    QCOMPARE(engine->opened.back().start, 754.0);
  }

  void resumeOffStartsFromBeginningAndSavesNothing() {
    AppSettings s;
    s.resumePlayback = false;
    createWindow(s);
    const QString video = dir.filePath("movie.mkv");
    touch(video);
    seedPosition("movie.mkv", 754);
    openAndLoad(video, 3600);
    QCOMPARE(engine->opened.back().start, 0.0);

    engine->s.position = 1800;
    window->close();  // would save 1800 if resume were on
    QThreadPool::globalInstance()->waitForDone();
    QCOMPARE(resume->lookup(chobi::core::ResumeStore::makeKey("movie.mkv", 1)).value_or(-1), 754.0);
  }

  void positionSavedWhenSwitchingAndRestoredLater() {
    const QString a = dir.filePath("a.mkv");
    const QString b = dir.filePath("b.mkv");
    touch(a);
    touch(b);
    openAndLoad(a, 3600);
    engine->s.position = 1234;
    openAndLoad(b, 3600);  // switching saves a's position
    QThreadPool::globalInstance()->waitForDone();
    openAndLoad(a, 3600);
    QCOMPARE(engine->opened.back().start, 1234.0);
  }

  void forgetSavedPositions() {
    seedPosition("movie.mkv", 754);
    resume->clear();
    QVERIFY(!resume->lookup(chobi::core::ResumeStore::makeKey("movie.mkv", 1)));
  }

  // --- Settings: remember volume ------------------------------------------------------------

  void volumeRememberedAcrossSessions() {
    const QString video = dir.filePath("v.mkv");
    touch(video);
    openAndLoad(video);
    engine->setVolume(86);
    window->close();  // the user clicks X
    QThreadPool::globalInstance()->waitForDone();

    const AppSettings next = store->load();
    QCOMPARE(next.volume, 86.0);
    QCOMPARE(next.startupVolume(), 86.0);  // the next session's engine starts at 86
  }

  void volumeNotRememberedWhenOff() {
    AppSettings s;
    s.rememberVolume = false;
    createWindow(s);
    engine->setVolume(40);
    window->close();
    QThreadPool::globalInstance()->waitForDone();
    QCOMPARE(store->load().startupVolume(), 100.0);
  }

  void settingsPersistAndClamp() {
    AppSettings s;
    s.resumePlayback = false;
    s.rememberVolume = true;
    s.volume = 140;  // out of range
    store->save(s);
    const AppSettings loaded = store->load();
    QVERIFY(!loaded.resumePlayback);
    QVERIFY(loaded.rememberVolume);
    QCOMPARE(loaded.volume, 100.0);
    QCOMPARE(SettingsStore(dir.filePath("missing.ini")).load().startupVolume(), 100.0);
  }

  void settingsDialogEditsBothOptions() {
    chobi::SettingsDialog dialog(AppSettings{});
    auto* resumeBox = dialog.findChild<QCheckBox*>("resumePlayback");
    auto* volumeBox = dialog.findChild<QCheckBox*>("rememberVolume");
    QVERIFY(resumeBox && volumeBox);
    QVERIFY(resumeBox->isChecked() && volumeBox->isChecked());  // both on by default
    resumeBox->setChecked(false);
    const AppSettings out = dialog.settings(AppSettings{});
    QVERIFY(!out.resumePlayback);
    QVERIFY(out.rememberVolume);

    window->applySettings(out);
    QThreadPool::globalInstance()->waitForDone();
    QVERIFY(!store->load().resumePlayback);
  }
};

QTEST_MAIN(MainWindowTest)
#include "test_main_window.moc"
