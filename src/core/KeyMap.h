// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace chobi::core {

enum class Key {
  Unknown,
  Left,
  Right,
  Up,
  Down,
  Space,
  Enter,
  Escape,
  Backspace,
  A,
  F,
  M,
  O,
  Q,
  S,
};

struct KeyModifiers {
  bool ctrl = false;
  bool shift = false;
  bool alt = false;
  bool meta = false;
};

enum class Action {
  None,
  SeekForward,
  SeekBackward,
  VolumeUp,
  VolumeDown,
  TogglePause,
  PauseAndMinimize,
  ToggleFullscreen,
  ToggleMute,
  ToggleSubtitles,
  CycleAudioTrack,
  OpenFile,
  Quit,
};

// The shortcut table from the product spec. Unlisted modifier combinations map to None so
// that e.g. Shift+F or Alt+Left never trigger an action by accident.
Action actionForKey(Key key, KeyModifiers modifiers);

const char* actionName(Action action);

}  // namespace chobi::core
