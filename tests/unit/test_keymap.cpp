// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>

#include "core/KeyMap.h"

using namespace chobi::core;

Q_DECLARE_METATYPE(chobi::core::Key)
Q_DECLARE_METATYPE(chobi::core::Action)

class KeyMapTest : public QObject {
  Q_OBJECT

 private slots:
  void specTable_data() {
    QTest::addColumn<Key>("key");
    QTest::addColumn<bool>("ctrl");
    QTest::addColumn<Action>("expected");
    QTest::newRow("Right") << Key::Right << false << Action::SeekForward;
    QTest::newRow("Left") << Key::Left << false << Action::SeekBackward;
    QTest::newRow("Up") << Key::Up << false << Action::VolumeUp;
    QTest::newRow("Down") << Key::Down << false << Action::VolumeDown;
    QTest::newRow("Space") << Key::Space << false << Action::TogglePause;
    QTest::newRow("Esc") << Key::Escape << false << Action::Quit;
    QTest::newRow("Backspace") << Key::Backspace << false << Action::PauseAndMinimize;
    QTest::newRow("F") << Key::F << false << Action::ToggleFullscreen;
    QTest::newRow("Enter") << Key::Enter << false << Action::ToggleFullscreen;
    QTest::newRow("M") << Key::M << false << Action::ToggleMute;
    QTest::newRow("S") << Key::S << false << Action::ToggleSubtitles;
    QTest::newRow("A") << Key::A << false << Action::CycleAudioTrack;
    QTest::newRow("O") << Key::O << false << Action::OpenFile;
    QTest::newRow("Ctrl+O") << Key::O << true << Action::OpenFile;
    QTest::newRow("Ctrl+Q") << Key::Q << true << Action::Quit;
  }

  void specTable() {
    QFETCH(Key, key);
    QFETCH(bool, ctrl);
    QFETCH(Action, expected);
    KeyModifiers m;
    m.ctrl = ctrl;
    QCOMPARE(actionForKey(key, m), expected);
  }

  void backspaceNeverQuits() {
    for (bool ctrl : {false, true}) {
      for (bool shift : {false, true}) {
        KeyModifiers m{ctrl, shift, false, false};
        QVERIFY(actionForKey(Key::Backspace, m) != Action::Quit);
      }
    }
  }

  void plainQDoesNothing() { QCOMPARE(actionForKey(Key::Q, {}), Action::None); }

  void unlistedModifiersDoNothing() {
    QCOMPARE(actionForKey(Key::F, {false, true, false, false}), Action::None);   // Shift+F
    QCOMPARE(actionForKey(Key::Left, {false, false, true, false}), Action::None);  // Alt+Left
    QCOMPARE(actionForKey(Key::Right, {true, false, false, false}), Action::None);  // Ctrl+Right
    QCOMPARE(actionForKey(Key::O, {true, true, false, false}), Action::None);  // Ctrl+Shift+O
    QCOMPARE(actionForKey(Key::Unknown, {}), Action::None);
  }
};

QTEST_APPLESS_MAIN(KeyMapTest)
#include "test_keymap.moc"
