// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

class QWidget;

namespace chobi {

// Where video pixels go. Today: OpenGL render API (Linux, Windows). Planned: a D3D11
// render-API surface on Windows once mpv 0.42 ships, and a native-window (wid) fallback.
// The main window only ever sees a QWidget, so swapping backends is a construction detail.
class IVideoSurface {
 public:
  virtual ~IVideoSurface() = default;
  virtual QWidget* widget() = 0;
  virtual QString backendName() const = 0;
};

}  // namespace chobi
