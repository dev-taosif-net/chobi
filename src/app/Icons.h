// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QIcon>

namespace chobi {

enum class Glyph {
  Play,
  Pause,
  Volume,
  Muted,
  AudioTrack,
  Subtitles,
  Fullscreen,
  ExitFullscreen,
  Settings,
};

// The control bar's icons, drawn with QPainter at whatever size and pixel ratio they are shown
// at. One consistent set that stays sharp on any display and readable on the dark bar whatever
// the desktop's icon theme, with no image files or SVG plugin. Disabled icons are drawn at
// reduced opacity.
QIcon glyphIcon(Glyph glyph, const QColor& color);

}  // namespace chobi
