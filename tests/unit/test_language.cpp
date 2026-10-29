// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/Language.h"

using namespace chobi::core;

class LanguageTest : public QObject {
  Q_OBJECT

 private slots:
  void parsesIso6391() {
    LanguageTag t;
    QVERIFY(parseLanguageTag("bn", &t));
    QCOMPARE(t.primary, std::string("bn"));
    QCOMPARE(t.full, std::string("bn"));
    QVERIFY(parseLanguageTag("EN", &t));
    QCOMPARE(t.primary, std::string("en"));
  }

  void parsesIso6392AndNames() {
    QCOMPARE(normalizeLanguage("eng"), std::string("en"));
    QCOMPARE(normalizeLanguage("ben"), std::string("bn"));
    QCOMPARE(normalizeLanguage("fre"), std::string("fr"));
    QCOMPARE(normalizeLanguage("Bangla"), std::string("bn"));
    QCOMPARE(normalizeLanguage("Bengali"), std::string("bn"));
    QCOMPARE(normalizeLanguage("ENGLISH"), std::string("en"));
  }

  void parsesRegionAndScript() {
    LanguageTag t;
    QVERIFY(parseLanguageTag("en-us", &t));
    QCOMPARE(t.full, std::string("en-US"));
    QVERIFY(parseLanguageTag("pt_BR", &t));
    QCOMPARE(t.full, std::string("pt-BR"));
    QVERIFY(parseLanguageTag("zh-hans-cn", &t));
    QCOMPARE(t.full, std::string("zh-Hans-CN"));
    QVERIFY(parseLanguageTag("es-419", &t));
    QCOMPARE(t.full, std::string("es-419"));
  }

  void rejectsNonLanguages() {
    for (const char* bad : {"", "und", "xx", "hd", "1080p", "x264", "part2", "english-us", "en-",
                            "en-USA1", "e", "toolongtoken"}) {
      QVERIFY2(!parseLanguageTag(bad, nullptr), bad);
    }
  }

  void displayNames() {
    QCOMPARE(languageDisplayName("bn"), std::string("Bangla"));
    QCOMPARE(languageDisplayName("eng"), std::string("English"));
    QCOMPARE(languageDisplayName("en-US"), std::string("English"));
    QCOMPARE(languageDisplayName("qaa"), std::string("QAA"));
  }
};

QTEST_APPLESS_MAIN(LanguageTest)
#include "test_language.moc"
