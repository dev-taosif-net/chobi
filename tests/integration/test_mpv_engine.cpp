// SPDX-License-Identifier: GPL-3.0-or-later
// Real libmpv (vo=null, ao=null) against the generated fixture corpus: format coverage,
// audio-track switching, exact seeking, external subtitles, broken files, rapid switching.
#include <QtTest>

#include <clocale>
#include <cmath>
#include <memory>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>

#include "engine/MpvEngine.h"

using chobi::MpvEngine;
using chobi::PlayerEngine;
using chobi::core::SubtitleFile;
using chobi::core::Track;
using chobi::core::TrackType;

namespace {

QString fixture(const QString& name) { return QDir(QStringLiteral(CHOBI_FIXTURE_DIR)).filePath(name); }

const QString kUnicodeBase = QString::fromUtf8(u8"ছবি টেস্ট ✓ Movie");

std::vector<const Track*> tracksOf(const std::vector<Track>& all, TrackType type) {
  std::vector<const Track*> out;
  for (const Track& t : all) {
    if (t.type == type) out.push_back(&t);
  }
  return out;
}

const Track* selectedOf(const std::vector<Track>& all, TrackType type) {
  for (const Track* t : tracksOf(all, type)) {
    if (t->selected) return t;
  }
  return nullptr;
}

#define REQUIRE_FIXTURE(name)                                                   \
  do {                                                                          \
    if (!QFile::exists(fixture(name))) QSKIP(qPrintable("fixture missing: " + QString(name))); \
  } while (0)

}  // namespace

class MpvEngineTest : public QObject {
  Q_OBJECT

  std::unique_ptr<MpvEngine> engine;

  // Opens and waits for file-loaded (true) or file-failed (false).
  bool load(const QString& path, double start = 0, std::vector<SubtitleFile> subs = {},
            QString* error = nullptr) {
    QSignalSpy loaded(engine.get(), &PlayerEngine::fileLoaded);
    QSignalSpy failed(engine.get(), &PlayerEngine::fileFailed);
    engine->open(path.toStdString(), start, std::move(subs));
    QElapsedTimer t;
    t.start();
    while (loaded.isEmpty() && failed.isEmpty() && t.elapsed() < 15000) QTest::qWait(5);
    if (!failed.isEmpty() && error) *error = failed.first().first().toString();
    return !loaded.isEmpty();
  }

 private slots:
  void initTestCase() { std::setlocale(LC_NUMERIC, "C"); }

  void init() {
    MpvEngine::Options o;
    o.headless = true;
    o.preferredLanguages = {QStringLiteral("bn"), QStringLiteral("en")};
    o.verboseLog = qEnvironmentVariableIntValue("CHOBI_MPV_LOG") == 1;
    engine = std::make_unique<MpvEngine>(o);
    QVERIFY2(engine->isValid(), qPrintable(engine->initError()));
  }

  void cleanup() { engine.reset(); }

  void playsH264AndReportsDuration() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(std::fabs(engine->state().duration - 12.0) < 0.2, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().position > 0.5, 5000);  // actually playing
    QVERIFY(!engine->state().paused);
  }

  void formatCoverage_data() {
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("codec");
    QTest::newRow("H.264/MP4") << "h264_aac.mp4" << "h264";
    QTest::newRow("HEVC/MKV") << "hevc.mkv" << "hevc";
    QTest::newRow("HEVC 10-bit") << "hevc10.mkv" << "hevc";
    QTest::newRow("VP9/WebM") << "vp9.webm" << "vp9";
    QTest::newRow("AV1/MKV") << "av1.mkv" << "av1";
    QTest::newRow("MPEG-2/TS") << "mpeg2.ts" << "mpeg2video";
    QTest::newRow("MPEG-2/VOB") << "mpeg2.vob" << "mpeg2video";
    QTest::newRow("MPEG-4/AVI") << "mpeg4.avi" << "mpeg4";
    QTest::newRow("WMV2/ASF") << "wmv2.wmv" << "wmv2";
    QTest::newRow("FLV1/FLV") << "flv1.flv" << "flv1";
    QTest::newRow("Theora/OGV") << "theora.ogv" << "theora";
    QTest::newRow("ProRes/MOV") << "prores.mov" << "prores";
    QTest::newRow("H.263/3GP") << "h263.3gp" << "h263";
    QTest::newRow("MJPEG/AVI") << "mjpeg.avi" << "mjpeg";
    QTest::newRow("FFV1/NUT") << "ffv1.nut" << "ffv1";
    QTest::newRow("DNxHD/MXF") << "dnxhd.mxf" << "dnxhd";
  }

  void formatCoverage() {
    QFETCH(QString, file);
    QFETCH(QString, codec);
    REQUIRE_FIXTURE(file);
    QString error;
    QVERIFY2(load(fixture(file), 0, {}, &error), qPrintable(error));
    QTRY_VERIFY_WITH_TIMEOUT(selectedOf(engine->tracks(), TrackType::Video) != nullptr, 5000);
    QCOMPARE(QString::fromStdString(selectedOf(engine->tracks(), TrackType::Video)->codec), codec);
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().position > 0.2, 8000);  // decodes, not just demuxes
  }

  void listsMultipleAudioTracksWithMetadata() {
    REQUIRE_FIXTURE("multi_audio.mkv");
    QVERIFY(load(fixture("multi_audio.mkv")));
    QTRY_COMPARE_WITH_TIMEOUT(tracksOf(engine->tracks(), TrackType::Audio).size(), std::size_t{3},
                              5000);
    const auto audio = tracksOf(engine->tracks(), TrackType::Audio);
    QCOMPARE(audio[0]->language, std::string("eng"));
    QCOMPARE(audio[0]->codec, std::string("aac"));
    QCOMPARE(audio[0]->channels, 2);
    QCOMPARE(audio[1]->language, std::string("ben"));
    QCOMPARE(audio[1]->codec, std::string("ac3"));
    QCOMPARE(audio[1]->channels, 6);
    QCOMPARE(audio[1]->title, std::string("Bangla dub"));
    QCOMPARE(audio[2]->codec, std::string("opus"));
    QCOMPARE(audio[2]->channels, 1);
    QVERIFY(audio[2]->language.empty() || audio[2]->language == "und");
  }

  void switchesAudioTrackWithoutRestarting() {
    REQUIRE_FIXTURE("multi_audio.mkv");
    QVERIFY(load(fixture("multi_audio.mkv")));
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().position > 1.0, 8000);
    QSignalSpy restarted(engine.get(), &PlayerEngine::fileStarted);
    const double before = engine->state().position;
    const auto audio = tracksOf(engine->tracks(), TrackType::Audio);
    const std::int64_t target = audio[1]->id;

    QElapsedTimer timer;
    timer.start();
    engine->selectAudioTrack(target);
    // Wait for mpv's own track-list (not our optimistic copy) to confirm.
    QTRY_VERIFY_WITH_TIMEOUT(
        [&] {
          const Track* sel = selectedOf(engine->tracks(), TrackType::Audio);
          return sel && sel->id == target && engine->state().position > before + 0.2;
        }(),
        5000);
    qInfo("audio switch confirmed after %lld ms", static_cast<long long>(timer.elapsed()));
    QVERIFY(restarted.isEmpty());                    // same file, no reload
    QVERIFY(engine->state().position >= before);     // position kept
  }

  void exactSeekLandsOnTarget() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4")));
    engine->setPaused(true);
    for (double target : {5.0, 7.37, 2.0}) {
      QSignalSpy restarted(engine.get(), &PlayerEngine::playbackRestarted);
      QElapsedTimer timer;
      timer.start();
      engine->seekAbsolute(target, true);
      QVERIFY(restarted.wait(5000));
      qInfo("seek to %.2f: frame ready after %lld ms", target,
            static_cast<long long>(timer.elapsed()));
      QTRY_VERIFY_WITH_TIMEOUT(std::fabs(engine->state().position - target) < 0.05, 2000);
    }
  }

  void seekToEndKeepsFileOpen() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4")));
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().duration > 0, 5000);
    engine->seekAbsolute(engine->state().duration, true);
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().eofReached, 5000);
    QVERIFY(engine->state().loaded);  // keep-open: still loaded at the end
  }

  void startOptionResumes() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4"), 6.0));
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().position >= 5.9, 5000);
    QVERIFY(engine->state().position < 8.0);
  }

  void attachesExternalSubtitlesUnicodePath() {
    const QString video = kUnicodeBase + ".mp4";
    REQUIRE_FIXTURE(video);
    std::vector<SubtitleFile> subs = {
        {fixture(kUnicodeBase + ".bn.srt").toStdString(), "bn.srt", "bn", true},
        {fixture(kUnicodeBase + ".en.srt").toStdString(), "en.srt", "en", false},
        {fixture(kUnicodeBase + ".ass").toStdString(), "ass", "", false},
    };
    QVERIFY(load(fixture(video), 0, subs));
    QTRY_COMPARE_WITH_TIMEOUT(tracksOf(engine->tracks(), TrackType::Subtitle).size(),
                              std::size_t{3}, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(selectedOf(engine->tracks(), TrackType::Subtitle) != nullptr, 5000);
    const Track* sel = selectedOf(engine->tracks(), TrackType::Subtitle);
    QCOMPARE(sel->language, std::string("bn"));
    QVERIFY(sel->external);
    QVERIFY(QString::fromStdString(sel->externalFile).endsWith(".bn.srt"));
  }

  void subtitlesNeverLeakIntoTheNextFile() {
    // Open A with subtitles and immediately replace it with B (no subtitles): mpv never
    // starts A, and A's subtitles must not be attached to B.
    const QString a = kUnicodeBase + ".mp4";
    REQUIRE_FIXTURE(a);
    REQUIRE_FIXTURE("h264_aac.mp4");
    QSignalSpy loaded(engine.get(), &PlayerEngine::fileLoaded);
    engine->open(fixture(a).toStdString(), 0,
                 {{fixture(kUnicodeBase + ".bn.srt").toStdString(), "bn", "bn", true}});
    engine->open(fixture("h264_aac.mp4").toStdString(), 0, {});
    QVERIFY(loaded.wait(10000));
    QTest::qWait(300);
    QVERIFY(tracksOf(engine->tracks(), TrackType::Subtitle).empty());
  }

  void manualSubtitleWithLegacyEncoding() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4")));
    engine->addSubtitleFile(fixture("cp1252.srt").toStdString(), true, "cp1252.srt", "");
    QTRY_VERIFY_WITH_TIMEOUT(selectedOf(engine->tracks(), TrackType::Subtitle) != nullptr, 5000);
  }

  void brokenInputsFailGracefully_data() {
    QTest::addColumn<QString>("file");
    QTest::newRow("random bytes") << "garbage.mkv";
    QTest::newRow("empty file") << "empty.mp4";
    QTest::newRow("mp4 without moov") << "truncated.mp4";
    QTest::newRow("missing file") << "no-such-file.mkv";
  }

  void brokenInputsFailGracefully() {
    QFETCH(QString, file);
    QString error;
    QVERIFY(!load(fixture(file), 0, {}, &error));
    QVERIFY2(!error.isEmpty(), "expected a user-facing error message");
    qInfo("%s -> %s", qPrintable(file), qPrintable(error.section('\n', 0, 0)));
    // The engine must still be usable afterwards.
    REQUIRE_FIXTURE("h264_aac.mp4");
    QVERIFY(load(fixture("h264_aac.mp4")));
  }

  void partialDownloadPlays() {
    REQUIRE_FIXTURE("truncated.mkv");
    QVERIFY(load(fixture("truncated.mkv")));
    QTRY_VERIFY_WITH_TIMEOUT(engine->state().position > 0.5, 8000);
  }

  void repeatedFileSwitchingIsStable() {
    REQUIRE_FIXTURE("h264_aac.mp4");
    REQUIRE_FIXTURE("multi_audio.mkv");
    auto switchFiles = [this](int count) {
      for (int i = 0; i < count; ++i) {
        if (!load(fixture(i % 2 ? "multi_audio.mkv" : "h264_aac.mp4"))) return false;
      }
      return true;
    };
    QVERIFY(switchFiles(10));  // warm-up: caches, decoder pools
    const long before = residentKb();
    QVERIFY(switchFiles(100));
    const long after = residentKb();
    if (before > 0) {
      qInfo("RSS across 100 file switches: %ld kB -> %ld kB (%+ld kB)", before, after,
            after - before);
    }
  }

 private:
  static long residentKb() {  // Linux only; other platforms report -1
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly)) return -1;
    for (const QByteArray& line : status.readAll().split('\n')) {
      if (line.startsWith("VmRSS:")) return line.mid(6).trimmed().split(' ').first().toLong();
    }
    return -1;
  }
};

QTEST_GUILESS_MAIN(MpvEngineTest)
#include "test_mpv_engine.moc"
