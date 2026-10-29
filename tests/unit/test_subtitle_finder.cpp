// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/SubtitleFinder.h"

using namespace chobi::core;

namespace {

std::vector<std::string> names(const std::vector<SubtitleCandidate>& found) {
  std::vector<std::string> out;
  for (const auto& c : found) out.push_back(c.fileName);
  return out;
}

QStringList q(const std::vector<std::string>& v) {
  QStringList out;
  for (const auto& s : v) out << QString::fromStdString(s);
  return out;
}

SubtitleMatchOptions prefs(std::vector<std::string> langs) {
  SubtitleMatchOptions o;
  o.preferredLanguages = std::move(langs);
  return o;
}

}  // namespace

class SubtitleFinderTest : public QObject {
  Q_OBJECT

 private slots:
  void specExamples() {
    const std::vector<std::string> dir = {"Movie.mkv", "Movie.srt", "Movie.en.srt",
                                          "Movie.en-US.srt", "Movie.ass", "Movie.bn.srt"};
    const auto found = findSubtitles("Movie.mkv", dir, prefs({"en"}));
    QCOMPARE(q(names(found)), (QStringList{"Movie.en.srt", "Movie.en-US.srt", "Movie.ass",
                                           "Movie.srt", "Movie.bn.srt"}));
    QCOMPARE(found[0].language, std::string("en"));
    QCOMPARE(found[1].languageTag, std::string("en-US"));
  }

  void preferredLanguageWinsAndOrderIsStable() {
    const std::vector<std::string> dir = {"Movie.srt", "Movie.en.srt", "Movie.bn.srt"};
    QCOMPARE(q(names(findSubtitles("Movie.mkv", dir, prefs({"bn", "en"})))),
             (QStringList{"Movie.bn.srt", "Movie.en.srt", "Movie.srt"}));
    // Directory order must not matter.
    const std::vector<std::string> shuffled = {"Movie.bn.srt", "Movie.srt", "Movie.en.srt"};
    QCOMPARE(q(names(findSubtitles("Movie.mkv", shuffled, prefs({"bn", "en"})))),
             (QStringList{"Movie.bn.srt", "Movie.en.srt", "Movie.srt"}));
  }

  void untaggedBeforeOtherLanguages() {
    const std::vector<std::string> dir = {"Movie.fr.srt", "Movie.srt", "Movie.de.srt"};
    QCOMPARE(q(names(findSubtitles("Movie.mkv", dir, prefs({"en"})))),
             (QStringList{"Movie.srt", "Movie.de.srt", "Movie.fr.srt"}));
  }

  void rejectsUnrelatedFiles() {
    const std::vector<std::string> dir = {
        "Movie2.srt",       "Movie Part2.srt", "Movie.Part2.srt", "Other.srt",
        "Movie.1080p.srt",  "Movie.mkv.txt",   "Movie.sub.txt",   "movie.en.srt.bak",
        "Movie.en.fr.srt",  ".srt",            "Movie..srt",      "Movie.en.srt.mkv"};
    QVERIFY(findSubtitles("Movie.mkv", dir, prefs({"en"})).empty());
  }

  void acceptsFlagsAndNames() {
    const std::vector<std::string> dir = {"Movie.en.forced.srt", "Movie.English.srt",
                                          "Movie.sdh.en.srt", "Movie.mkv.srt"};
    const auto found = findSubtitles("Movie.mkv", dir, prefs({"en"}));
    QCOMPARE(q(names(found)), (QStringList{"Movie.English.srt", "Movie.sdh.en.srt",
                                           "Movie.en.forced.srt", "Movie.mkv.srt"}));
    QVERIFY(found[2].forced);
    QVERIFY(found[1].hearingImpaired);
  }

  void formatPriority() {
    const std::vector<std::string> dir = {"Movie.vtt", "Movie.srt", "Movie.ssa", "Movie.ass"};
    QCOMPARE(q(names(findSubtitles("Movie.mkv", dir, {}))),
             (QStringList{"Movie.ass", "Movie.ssa", "Movie.srt", "Movie.vtt"}));
  }

  void caseFoldingRanksBelowExactCase() {
    const std::vector<std::string> dir = {"movie.EN.SRT", "Movie.en.srt"};
    const auto found = findSubtitles("Movie.mkv", dir, prefs({"en"}));
    QCOMPARE(q(names(found)), (QStringList{"Movie.en.srt", "movie.EN.SRT"}));
    QVERIFY(found[0].exactCase);
    QVERIFY(!found[1].exactCase);
  }

  void unicodeAndSpaces() {
    const std::string video = u8"ছবি টেস্ট ✓ Movie.mkv";
    const std::vector<std::string> dir = {u8"ছবি টেস্ট ✓ Movie.bn.srt",
                                          u8"ছবি টেস্ট ✓ Movie.srt", u8"ছবি টেস্ট.srt"};
    QCOMPARE(q(names(findSubtitles(video, dir, prefs({"bn"})))),
             (QStringList{QString::fromUtf8(u8"ছবি টেস্ট ✓ Movie.bn.srt"),
                          QString::fromUtf8(u8"ছবি টেস্ট ✓ Movie.srt")}));
  }

  void dotsInVideoName() {
    const std::vector<std::string> dir = {"The.Movie.2020.1080p.en.srt",
                                          "The.Movie.2020.1080p.srt", "The.Movie.2020.srt"};
    QCOMPARE(q(names(findSubtitles("The.Movie.2020.1080p.mkv", dir, prefs({"en"})))),
             (QStringList{"The.Movie.2020.1080p.en.srt", "The.Movie.2020.1080p.srt"}));
  }

  void stemHelper() {
    QCOMPARE(std::string(fileStem("a.b.mkv")), std::string("a.b"));
    QCOMPARE(std::string(fileStem("noext")), std::string("noext"));
    QCOMPARE(std::string(fileStem(".hidden")), std::string(".hidden"));
  }
};

QTEST_APPLESS_MAIN(SubtitleFinderTest)
#include "test_subtitle_finder.moc"
