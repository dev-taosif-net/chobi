// SPDX-License-Identifier: GPL-3.0-or-later
// Drives the real MainWindow (offscreen) with a fake engine: keyboard shortcuts, Esc order,
// drag-and-drop with awkward paths, external subtitle discovery and error display.
#include <QtTest>

#include <QFile>
#include <QMimeData>
#include <QTemporaryDir>

#include "FakeEngine.h"
#include "app/MainWindow.h"

using chobi::MainWindow;

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
    window = new MainWindow(engine, new QWidget, nullptr, {"bn", "en"});
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
  }

  void cleanup() {
    delete window;
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
};

QTEST_MAIN(MainWindowTest)
#include "test_main_window.moc"
