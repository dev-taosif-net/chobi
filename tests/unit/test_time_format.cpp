// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include <cmath>

#include "core/TimeFormat.h"

using chobi::core::formatPosition;
using chobi::core::formatRemaining;
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

  void remaining() {
    const std::string minus = "\u2212";
    QCOMPARE(formatRemaining(0, 100), minus + "1:40");
    QCOMPARE(formatRemaining(754.6, 5532.4, true), minus + "1:19:38");
    QCOMPARE(formatRemaining(30, 100, true), minus + "0:01:10");
    QCOMPARE(formatRemaining(100, 100), minus + "0:00");
    QCOMPARE(formatRemaining(150, 100), minus + "0:00");  // past the end: never negative
    QCOMPARE(formatRemaining(NAN, 100), minus + "1:40");
  }

  void playedPlusRemainingIsTheTotal() {
    // Whole seconds on both sides, so the two labels never disagree with the total by one.
    for (double position : {0.0, 0.4, 12.3, 59.99, 99.5}) {
      const std::string left = formatRemaining(position, 100.7).substr(3);  // drop the minus
      const int played = static_cast<int>(position);
      const int rest = std::stoi(left.substr(0, left.find(':'))) * 60 +
                       std::stoi(left.substr(left.find(':') + 1));
      QCOMPARE(played + rest, 100);
    }
  }

  void position() {
    QCOMPARE(formatPosition(105, 600), std::string("1:45 / 10:00"));
    QCOMPARE(formatPosition(105, 7200), std::string("0:01:45 / 2:00:00"));  // same width as total
    QCOMPARE(formatPosition(105, 0), std::string("1:45"));                  // unknown duration
    QCOMPARE(formatPosition(105, NAN), std::string("1:45"));
  }

  void invalidInputsAreZero() {
    QCOMPARE(formatTime(-3), std::string("0:00"));
    QCOMPARE(formatTime(NAN), std::string("0:00"));
    QCOMPARE(formatTime(INFINITY), std::string("0:00"));
  }
};

QTEST_APPLESS_MAIN(TimeFormatTest)
#include "test_time_format.moc"
