// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/ResumeStore.h"

using namespace chobi::core;

namespace {

constexpr std::int64_t kDay = 24 * 60 * 60;
constexpr std::int64_t kT0 = 1'800'000'000;  // an arbitrary "now" (2027)

}  // namespace

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
    s.record(key, "/v/Movie.mkv", 600, 3600, kT0);
    QCOMPARE(s.lookup(key, kT0).value_or(-1), 600.0);
    QCOMPARE(s.entries().front().path, std::string("/v/Movie.mkv"));
    s.record(key, "/v/Movie.mkv", 3590, 3600, kT0);  // watched to the end
    QVERIFY(!s.lookup(key, kT0));
  }

  void expiresAfterSevenDays() {
    QCOMPARE(ResumeStore::kRetentionSeconds, 7 * kDay);
    ResumeStore s;
    s.record("k", "/v/a.mkv", 600, 0, kT0);
    QVERIFY(s.lookup("k", kT0 + 6 * kDay));       // day 6: still there
    QVERIFY(s.lookup("k", kT0 + 7 * kDay));       // exactly 7 days: still there
    QVERIFY(!s.lookup("k", kT0 + 7 * kDay + 1));  // a second later: gone
    QVERIFY(!s.pruneExpired(kT0 + 7 * kDay));
    QVERIFY(s.pruneExpired(kT0 + 7 * kDay + 1));
    QCOMPARE(s.size(), std::size_t{0});
  }

  void watchingAgainRestartsTheSevenDays() {
    ResumeStore s;
    s.record("k", "/v/a.mkv", 600, 0, kT0);
    s.record("k", "/v/a.mkv", 900, 0, kT0 + 5 * kDay);  // watched again on day 5
    QCOMPARE(s.lookup("k", kT0 + 11 * kDay).value_or(-1), 900.0);  // 6 days after that
    QVERIFY(!s.lookup("k", kT0 + 13 * kDay));
  }

  void roundTripWithAwkwardNames() {
    ResumeStore s;
    const std::string a = ResumeStore::makeKey(u8"ছবি ✓ tab\tnew\nline\\.mkv", 42);
    const std::string pathA = u8"/media/ছবি/ছবি ✓ tab\tnew\nline\\.mkv";
    const std::string b = ResumeStore::makeKey("plain.mp4", 7);
    s.record(a, pathA, 61.27, 0, kT0);
    s.record(b, "C:/Videos/plain.mp4", 95.5, 1000, kT0 + 10);
    ResumeStore t;
    t.parse(s.serialize(), kT0 + 20);
    QCOMPARE(t.size(), std::size_t{2});
    QCOMPARE(t.lookup(a, kT0 + 20).value_or(-1), 61.3);  // stored with one decimal
    QCOMPARE(t.entries()[0].path, pathA);
    QCOMPARE(t.entries()[0].updatedAt, kT0);
    QCOMPARE(t.entries()[1].updatedAt, kT0 + 10);
    QCOMPARE(t.lookup(b, kT0 + 20).value_or(-1), 95.5);
  }

  void migratesVersion1Files() {
    ResumeStore s;
    s.parse("chobi-resume 1\n120.5\t10:old.mkv\n", kT0);
    QCOMPARE(s.lookup("10:old.mkv", kT0).value_or(-1), 120.5);
    QCOMPARE(s.entries().front().updatedAt, kT0);  // full 7 days from the upgrade
    QVERIFY(s.entries().front().path.empty());     // unknown path: only expiry applies
    QVERIFY(s.serialize().rfind("chobi-resume 2\n", 0) == 0);
  }

  void capacityEvictsLeastRecent() {
    ResumeStore s(3);
    for (int i = 0; i < 5; ++i) s.record("k" + std::to_string(i), "", 100 + i, 0, kT0);
    QCOMPARE(s.size(), std::size_t{3});
    QVERIFY(!s.lookup("k0", kT0));
    QVERIFY(!s.lookup("k1", kT0));
    QVERIFY(s.lookup("k4", kT0));
    s.record("k2", "", 200, 0, kT0);  // refresh k2, then add k5 -> k3 goes
    s.record("k5", "", 300, 0, kT0);
    QVERIFY(!s.lookup("k3", kT0));
    QVERIFY(s.lookup("k2", kT0));
  }

  void ignoresGarbage() {
    ResumeStore s;
    s.parse("not a resume file\n12.0\tkey\n", kT0);
    QCOMPARE(s.size(), std::size_t{0});
    s.parse("chobi-resume 2\nabc\t1\tkey\tp\n\n12.5\n\t\n12.5\tx\tkey\tp\n12.5\t" +
                std::to_string(kT0) + "\tok\t/p\r\n",
            kT0);
    QCOMPARE(s.size(), std::size_t{1});
    QCOMPARE(s.lookup("ok", kT0).value_or(-1), 12.5);
  }
};

QTEST_APPLESS_MAIN(ResumeStoreTest)
#include "test_resume_store.moc"
