// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/Version.h"

using namespace chobi::core;

class VersionTest : public QObject {
  Q_OBJECT

 private slots:
  void parses() {
    auto v = parseVersion("v1.12.3");
    QVERIFY(v);
    QCOMPARE(v->major, 1);
    QCOMPARE(v->minor, 12);
    QCOMPARE(v->patch, 3);
    v = parseVersion("0.3.0-beta.2+build.7");
    QVERIFY(v);
    QCOMPARE(v->prerelease, std::string("beta.2"));
  }

  void rejectsJunk() {
    for (const char* bad : {"", "1", "1.2", "1.2.3.4", "1.02.3", "a.b.c", "1.2.3-", "v", "-1.2.3"}) {
      QVERIFY2(!parseVersion(bad), bad);
    }
  }

  void ordering() {
    QVERIFY(isNewerVersion("0.2.0", "0.1.0"));
    QVERIFY(isNewerVersion("0.10.0", "0.9.9"));  // numeric, not string, comparison
    QVERIFY(isNewerVersion("1.0.0", "0.99.99"));
    QVERIFY(isNewerVersion("0.2.1", "0.2.0"));
    QVERIFY(!isNewerVersion("0.2.0", "0.2.0"));
    QVERIFY(!isNewerVersion("0.1.9", "0.2.0"));  // never "update" to an older release
    QVERIFY(isNewerVersion("0.3.0", "0.3.0-beta.1"));
    QVERIFY(!isNewerVersion("0.3.0-beta.1", "0.3.0"));
    QVERIFY(isNewerVersion("0.3.0-beta.10", "0.3.0-beta.2"));
    QVERIFY(isNewerVersion("0.3.0-rc.1", "0.3.0-beta.9"));
    QVERIFY(!isNewerVersion("garbage", "0.1.0"));
    QVERIFY(!isNewerVersion("0.2.0", "garbage"));
  }
};

QTEST_APPLESS_MAIN(VersionTest)
#include "test_version.moc"
