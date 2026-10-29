// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/TrackLabel.h"

using namespace chobi::core;

class TrackLabelTest : public QObject {
  Q_OBJECT

 private slots:
  void audioLabels() {
    Track t;
    t.type = TrackType::Audio;
    t.language = "ben";
    t.codec = "ac3";
    t.channels = 6;
    QCOMPARE(trackLabel(t), std::string("Bangla · AC-3 · 5.1"));

    t.language = "eng";
    t.title = "Commentary";
    t.codec = "aac";
    t.channels = 2;
    QCOMPARE(trackLabel(t), std::string("English — Commentary · AAC · stereo"));
  }

  void missingMetadata() {
    Track t;
    t.type = TrackType::Audio;
    QCOMPARE(trackLabel(t), std::string("Unknown language"));
    t.language = "und";
    QCOMPARE(trackLabel(t), std::string("Unknown language"));
    t.codec = "pcm_s16le";
    t.channels = 3;
    QCOMPARE(trackLabel(t), std::string("Unknown language · PCM · 3 ch"));
  }

  void subtitleFlags() {
    Track t;
    t.type = TrackType::Subtitle;
    t.language = "bn";
    t.codec = "subrip";
    t.external = true;
    t.forced = true;
    t.channels = 2;  // ignored for subtitles
    QCOMPARE(trackLabel(t), std::string("Bangla · SRT · forced · external"));
  }

  void helpers() {
    QCOMPARE(prettyCodecName("eac3"), std::string("E-AC-3"));
    QCOMPARE(prettyCodecName("weirdcodec"), std::string("WEIRDCODEC"));
    QCOMPARE(channelLayoutName(1), std::string("mono"));
    QCOMPARE(channelLayoutName(8), std::string("7.1"));
    QCOMPARE(channelLayoutName(0), std::string());
  }
};

QTEST_APPLESS_MAIN(TrackLabelTest)
#include "test_track_label.moc"
