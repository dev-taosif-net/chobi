// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/KeyMap.h"

namespace chobi::core {

Action actionForKey(Key key, KeyModifiers m) {
  const bool none = !m.ctrl && !m.shift && !m.alt && !m.meta;
  const bool ctrlOnly = m.ctrl && !m.shift && !m.alt && !m.meta;

  if (ctrlOnly) {
    switch (key) {
      case Key::O: return Action::OpenFile;
      case Key::Q: return Action::Quit;
      default: return Action::None;
    }
  }
  if (!none) return Action::None;

  switch (key) {
    case Key::Right: return Action::SeekForward;
    case Key::Left: return Action::SeekBackward;
    case Key::Up: return Action::VolumeUp;
    case Key::Down: return Action::VolumeDown;
    case Key::Space: return Action::TogglePause;
    case Key::Escape: return Action::PauseAndMinimize;
    case Key::F: return Action::ToggleFullscreen;
    case Key::M: return Action::ToggleMute;
    case Key::S: return Action::ToggleSubtitles;
    case Key::A: return Action::CycleAudioTrack;
    case Key::O: return Action::OpenFile;
    case Key::Q:
    case Key::Unknown: return Action::None;
  }
  return Action::None;
}

const char* actionName(Action action) {
  switch (action) {
    case Action::None: return "None";
    case Action::SeekForward: return "SeekForward";
    case Action::SeekBackward: return "SeekBackward";
    case Action::VolumeUp: return "VolumeUp";
    case Action::VolumeDown: return "VolumeDown";
    case Action::TogglePause: return "TogglePause";
    case Action::PauseAndMinimize: return "PauseAndMinimize";
    case Action::ToggleFullscreen: return "ToggleFullscreen";
    case Action::ToggleMute: return "ToggleMute";
    case Action::ToggleSubtitles: return "ToggleSubtitles";
    case Action::CycleAudioTrack: return "CycleAudioTrack";
    case Action::OpenFile: return "OpenFile";
    case Action::Quit: return "Quit";
  }
  return "?";
}

}  // namespace chobi::core
