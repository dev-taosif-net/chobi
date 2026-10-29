// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/ResumeStore.h"

using namespace chobi::core;

class ResumeStoreTest : public QObject {
  Q_OBJECT

 private slots:
  void policy() {
    QVERIFY(!ResumeStore::shouldRemember(10, 3600));    // too early
    QVERIFY(ResumeStore::shouldRemember(600, 3600));
    QVERIFY(!ResumeStore::shouldRemember(3500, 3600));  // > 95%: finished
    QVERIFY(ResumeStore::shouldRemember(120, 0));       // unknown duration
  }

  void recordLookupForget() {
    ResumeStore s;
    const std::string key = ResumeStore::makeKey("Movie.mkv", 123456);
    s.record(key, 600, 3600);
    QCOMPARE(s.lookup(key).value_or(-1), 600.0);
    s.record(key, 3590, 3600);  // watched to the end
    QVERIFY(!s.lookup(key));
  }

  void roundTripWithAwkwardNames() {
    ResumeStore s;
    const std::string a = ResumeStore::makeKey(u8"ছবি ✓ tab\tnew\nline\\.mkv", 42);
    const std::string b = ResumeStore::makeKey("plain.mp4", 7);
    s.record(a, 61.27, 0);
    s.record(b, 95.5, 1000);
    ResumeStore t;
    t.parse(s.serialize());
    QCOMPARE(t.size(), std::size_t{2});
    QCOMPARE(t.lookup(a).value_or(-1), 61.3);  // stored with one decimal
    QCOMPARE(t.lookup(b).value_or(-1), 95.5);
  }

  void capacityEvictsLeastRecent() {
    ResumeStore s(3);
    for (int i = 0; i < 5; ++i) s.record("k" + std::to_string(i), 100 + i, 0);
    QCOMPARE(s.size(), std::size_t{3});
    QVERIFY(!s.lookup("k0"));
    QVERIFY(!s.lookup("k1"));
    QVERIFY(s.lookup("k4"));
    s.record("k2", 200, 0);  // refresh k2, then add k5 -> k3 goes
    s.record("k5", 300, 0);
    QVERIFY(!s.lookup("k3"));
    QVERIFY(s.lookup("k2"));
  }

  void ignoresGarbage() {
    ResumeStore s;
    s.parse("not a resume file\n12.0\tkey\n");
    QCOMPARE(s.size(), std::size_t{0});
    s.parse("chobi-resume 1\nabc\tkey\n\n12.5\n\t\n12.5\tok\r\n");
    QCOMPARE(s.size(), std::size_t{1});
    QCOMPARE(s.lookup("ok").value_or(-1), 12.5);
  }
};

QTEST_APPLESS_MAIN(ResumeStoreTest)
#include "test_resume_store.moc"
