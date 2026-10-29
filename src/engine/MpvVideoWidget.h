// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>

#include <QOpenGLWidget>

#include "engine/IVideoSurface.h"

struct mpv_render_context;

namespace chobi {

class MpvEngine;

// Renders libmpv output with the OpenGL render API. Redraws only when mpv reports a new
// frame (no timers), so a paused or idle player costs nothing.
class MpvVideoWidget final : public QOpenGLWidget, public IVideoSurface {
  Q_OBJECT

 public:
  explicit MpvVideoWidget(MpvEngine* engine, QWidget* parent = nullptr);
  ~MpvVideoWidget() override;

  QWidget* widget() override { return this; }
  QString backendName() const override { return QStringLiteral("opengl"); }

 signals:
  void framePainted();

 protected:
  void initializeGL() override;
  void paintGL() override;

 private:
  void onMpvUpdate();
  void destroyRenderContext();

  MpvEngine* engine_;
  mpv_render_context* render_ = nullptr;
  std::atomic<bool> updatePending_{false};
};

}  // namespace chobi
