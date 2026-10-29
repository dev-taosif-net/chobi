// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include <cmath>

#include "core/TimeFormat.h"

using chobi::core::formatTime;

class TimeFormatTest : public QObject {
  Q_OBJECT

 private slots:
  void formats() {
    QCOMPARE(formatTime(0), std::string("0:00"));
    QCOMPARE(formatTime(5.9), std::string("0:05"));
    QCOMPARE(formatTime(65), std::string("1:05"));
    QCOMPARE(formatTime(3599), std::string("59:59"));
    QCOMPARE(formatTime(3600), std::string("1:00:00"));
    QCOMPARE(formatTime(3725), std::string("1:02:05"));
    QCOMPARE(formatTime(65, true), std::string("0:01:05"));
    QCOMPARE(formatTime(100 * 3600), std::string("100:00:00"));
  }

  void invalidInputsAreZero() {
    QCOMPARE(formatTime(-3), std::string("0:00"));
    QCOMPARE(formatTime(NAN), std::string("0:00"));
    QCOMPARE(formatTime(INFINITY), std::string("0:00"));
  }
};

QTEST_APPLESS_MAIN(TimeFormatTest)
#include "test_time_format.moc"
