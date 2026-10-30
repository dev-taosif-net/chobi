// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include <memory>

#include "app/SeekSlider.h"

using chobi::SeekSlider;

class TestSeekSlider : public QObject {
  Q_OBJECT

  std::unique_ptr<SeekSlider> slider;

  void mouse(QEvent::Type type, QPoint p, Qt::MouseButton button, Qt::MouseButtons buttons) {
    QMouseEvent e(type, p, slider->mapToGlobal(p), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(slider.get(), &e);
  }

 private slots:
  void init() {
    slider = std::make_unique<SeekSlider>();
    slider->resize(600, 24);
    slider->setDuration(6000);
    slider->setPosition(100);
    slider->show();
    QVERIFY(QTest::qWaitForWindowExposed(slider.get()));
  }

  void clickSeeksExactlyOnce() {
    QSignalSpy spy(slider.get(), &SeekSlider::seekRequested);
    const QPoint p(400, 12);
    mouse(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
    mouse(QEvent::MouseMove, p + QPoint(2, 0), Qt::NoButton, Qt::LeftButton);  // hand jitter
    mouse(QEvent::MouseButtonRelease, p + QPoint(2, 0), Qt::LeftButton, Qt::NoButton);
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.at(0).at(1).toBool());  // exact
  }

  void dragScrubsByKeyframeLikeMpv() {
    QSignalSpy spy(slider.get(), &SeekSlider::seekRequested);
    mouse(QEvent::MouseButtonPress, QPoint(300, 12), Qt::LeftButton, Qt::LeftButton);
    for (int x = 310; x <= 400; x += 10) {
      mouse(QEvent::MouseMove, QPoint(x, 12), Qt::NoButton, Qt::LeftButton);
    }
    mouse(QEvent::MouseButtonRelease, QPoint(400, 12), Qt::LeftButton, Qt::NoButton);
    QVERIFY(spy.count() >= 3);
    QVERIFY(spy.first().at(1).toBool());  // press: exact
    QVERIFY(!spy.at(1).at(1).toBool());   // moves: keyframe
    QVERIFY(!spy.last().at(1).toBool());  // release adds nothing: the last keyframe seek stands...
    QCOMPARE(spy.last().at(0).toDouble(), slider->value() / 1000.0);  // ...at the drop point
  }
};

QTEST_MAIN(TestSeekSlider)
#include "test_seek_slider.moc"
