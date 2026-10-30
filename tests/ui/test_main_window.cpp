// SPDX-License-Identifier: GPL-3.0-or-later
// Drives the real MainWindow (offscreen) with a fake engine: keyboard shortcuts, Backspace order,
// drag-and-drop with awkward paths, external subtitle discovery, error display, and the
// "resume where stopped" / "remember volume" settings.
#include <QtTest>

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLabel>
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
  static std::string keyFor(const QString& fileName) {
    return chobi::core::ResumeStore::makeKey(fileName.toStdString(), 1);
  }
  void seedPosition(const QString& fileName, double seconds) {
    resume->record(keyFor(fileName), dir.filePath(fileName), seconds, 0);
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
    QCOMPARE(window->osdText(), QStringLiteral("0:50 / 1:40"));
    // The seek OSD follows where playback really is: keyframe seeks land past the target.
    engine->s.position = 57.4;
    emit engine->positionChanged(57.4);
    QCOMPARE(window->osdText(), QStringLiteral("0:57 / 1:40"));

    engine->s.volume = 97;
    QTest::keyClick(window, Qt::Key_Up);
    QCOMPARE(engine->s.volume, 100.0);
    emit engine->positionChanged(60);  // any other message stops following the position
    QVERIFY(window->osdText().startsWith(QStringLiteral("Volume")));
    QTest::keyClick(window, Qt::Key_Down);
    QCOMPARE(engine->s.volume, 95.0);
  }

  void heldArrowRepeatsLikeMpv() {
    // Stock mpv repeats a held key after 200 ms, 40 times a second, whatever the desktop says
    // (GNOME: after 500 ms, 33 a second). The desktop's own repeats are ignored for seeking.
    const QString video = dir.filePath("held.mp4");
    touch(video);
    openAndLoad(video, 10000);
    engine->s.position = 1000;
    QTest::keyPress(window, Qt::Key_Right);
    QCOMPARE(engine->seeks.size(), std::size_t{1});
    QKeyEvent desktopRepeat(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, QString(), true);
    QApplication::sendEvent(window, &desktopRepeat);
    QTest::qWait(150);
    QCOMPARE(engine->seeks.size(), std::size_t{1});  // still inside the 200 ms delay
    QTest::qWait(250);
    QTest::keyRelease(window, Qt::Key_Right);
    const std::size_t held = engine->seeks.size();
    QVERIFY2(held >= 5, qPrintable(QString::number(held)));
    QTest::qWait(100);
    QCOMPARE(engine->seeks.size(), held);  // stops on release

    QTest::keyPress(window, Qt::Key_Left);
    QFocusEvent focusOut(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(window, &focusOut);  // the release goes to another window
    QTest::qWait(300);
    QCOMPARE(engine->seeks.size(), held + 1);
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

  void backspacePausesLeavesFullscreenAndMinimizes() {
    const QString video = dir.filePath("clip3.mkv");
    touch(video);
    openAndLoad(video);
    engine->s.paused = false;
    window->setFullscreen(true);
    QTRY_VERIFY(window->isFullscreen());
    QTest::keyClick(window, Qt::Key_Backspace);
    QVERIFY(engine->s.paused);
    QTRY_VERIFY(!window->isFullScreen());
    QTRY_VERIFY(window->windowState().testFlag(Qt::WindowMinimized));
    QVERIFY(window->isVisible());  // minimized, never closed
  }

  void ctrlQCloses() {
    QTest::keyClick(window, Qt::Key_Q, Qt::ControlModifier);
    QTRY_VERIFY(!window->isVisible());
  }

  void escapeClosesAndSavesThePosition() {
    const QString video = dir.filePath("clip4.mkv");
    touch(video);
    openAndLoad(video, 600);
    engine->s.position = 120;
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!window->isVisible());
    QThreadPool::globalInstance()->waitForDone();
    QCOMPARE(resume->lookup(keyFor("clip4.mkv")).value_or(-1), 120.0);
  }

  void anotherLaunchPlaysHereAndComesForward() {
    const QString first = dir.filePath("first.mp4");
    const QString second = dir.filePath("second.mp4");
    touch(first);
    touch(second);
    openAndLoad(first);
    window->showMinimized();
    QTRY_VERIFY(window->isMinimized());

    QSignalSpy probed(window, &MainWindow::probeFinished);
    window->openFromAnotherLaunch(second, {});
    QVERIFY(probed.wait(5000));
    QCOMPARE(window->currentLocation(), second);
    QCOMPARE(QString::fromStdString(engine->opened.back().location), second);
    QTRY_VERIFY(!window->isMinimized());

    const std::size_t opens = engine->opened.size();
    window->openFromAnotherLaunch(QString(), {});  // plain relaunch: nothing new to play
    QCOMPARE(engine->opened.size(), opens);
    QCOMPARE(window->currentLocation(), second);
  }

  void controlBarShowsTimeLeft() {
    const QString video = dir.filePath("clip5.mkv");
    touch(video);
    openAndLoad(video, 100);
    emit engine->positionChanged(30.4);
    const QString minus = QString(QChar(0x2212));
    QStringList texts;
    for (const QLabel* label : window->findChildren<QLabel*>()) texts << label->text();
    QVERIFY2(texts.contains(QStringLiteral("0:30")), qPrintable(texts.join(" | ")));
    QVERIFY2(texts.contains(minus + QStringLiteral("1:10 / 1:40")), qPrintable(texts.join(" | ")));
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
    QCOMPARE(resume->lookup(keyFor("movie.mkv")).value_or(-1), 754.0);
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
    touch(dir.filePath("movie.mkv"));
    seedPosition("movie.mkv", 754);
    QVERIFY(resume->lookup(keyFor("movie.mkv")));
    resume->clear();
    QVERIFY(!resume->lookup(keyFor("movie.mkv")));
  }

  void positionsExpireAfterSevenDays() {
    auto now = std::make_shared<std::int64_t>(1'800'000'000);
    const QString file = dir.filePath("week.tsv");
    ResumeService service(file, [now] { return *now; });
    const QString video = dir.filePath("week.mkv");
    touch(video);
    service.record(keyFor("week.mkv"), video, 600, 0);

    *now += 6 * 24 * 3600;  // six days later: still resumes
    QCOMPARE(service.lookup(keyFor("week.mkv")).value_or(-1), 600.0);
    *now += 2 * 24 * 3600;  // eight days after the last watch: gone, also from disk
    QVERIFY(!service.lookup(keyFor("week.mkv")));
    QFile f(file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QVERIFY(!f.readAll().contains("week.mkv"));
  }

  void deletedVideoIsForgotten() {
    const QString kept = dir.filePath("kept.mkv");
    const QString gone = dir.filePath("gone.mkv");
    touch(kept);
    touch(gone);
    seedPosition("kept.mkv", 300);
    seedPosition("gone.mkv", 400);
    QVERIFY(QFile::remove(gone));  // the user deletes the video
    QCOMPARE(resume->pruneDeletedVideos(), 1);
    QVERIFY(!resume->lookup(keyFor("gone.mkv")));
    QCOMPARE(resume->lookup(keyFor("kept.mkv")).value_or(-1), 300.0);
  }

  void replacedVideoIsForgotten() {
    const QString video = dir.filePath("replaced.mkv");
    touch(video);
    seedPosition("replaced.mkv", 300);
    QFile f(video);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("a different, longer file");  // same name, different size
    f.close();
    QCOMPARE(resume->pruneDeletedVideos(), 1);
    QVERIFY(!resume->lookup(keyFor("replaced.mkv")));
  }

  void unpluggedDriveIsKept() {
    QDir(dir.path()).mkdir("usb");
    const QString video = dir.filePath("usb/film.mkv");
    touch(video);
    resume->record(keyFor("film.mkv"), video, 500, 0);
    QVERIFY(QDir(dir.path()).rename("usb", "usb-unplugged"));  // whole folder vanishes
    QCOMPARE(resume->pruneDeletedVideos(), 0);  // can't tell deleted from unplugged: keep
    QCOMPARE(resume->lookup(keyFor("film.mkv")).value_or(-1), 500.0);
  }

  void videoDeletedWhilePlayingIsNotSaved() {
    const QString video = dir.filePath("playing.mkv");
    touch(video);
    seedPosition("playing.mkv", 100);
    openAndLoad(video, 3600);
    engine->s.position = 900;
    QVERIFY(QFile::remove(video));
    window->close();
    QThreadPool::globalInstance()->waitForDone();
    QVERIFY(!resume->lookup(keyFor("playing.mkv")));
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
