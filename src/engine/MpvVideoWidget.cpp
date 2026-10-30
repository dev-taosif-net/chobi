// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/MpvVideoWidget.h"

#include <mpv/render_gl.h>

#include <vector>

#include <QGuiApplication>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QWindow>
#include <QtGui/qguiapplication_platform.h>

#include "engine/MpvEngine.h"

namespace chobi {
namespace {

void* getProcAddress(void*, const char* name) {
  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  return ctx ? reinterpret_cast<void*>(ctx->getProcAddress(name)) : nullptr;
}

}  // namespace

MpvVideoWidget::MpvVideoWidget(MpvEngine* engine, QWidget* parent)
    : QOpenGLWidget(parent), engine_(engine) {
  setFocusPolicy(Qt::NoFocus);
  setMouseTracking(true);
  connect(this, &QOpenGLWidget::frameSwapped, this, [this] {
    if (render_) mpv_render_context_report_swap(render_);
  });
}

MpvVideoWidget::~MpvVideoWidget() { destroyRenderContext(); }

void MpvVideoWidget::initializeGL() {
  if (render_ || !engine_ || !engine_->handle()) return;

  mpv_opengl_init_params glInit{&getProcAddress, nullptr};
  int advancedControl = 1;
  std::vector<mpv_render_param> params = {
      {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
      {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advancedControl},
  };
  // VA-API interop needs the native display the GL context lives on.
#if QT_CONFIG(wayland)
  if (auto* wl = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
    params.push_back({MPV_RENDER_PARAM_WL_DISPLAY, wl->display()});
  }
#endif
#if QT_CONFIG(xcb)
  if (auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
    params.push_back({MPV_RENDER_PARAM_X11_DISPLAY, x11->display()});
  }
#endif
  params.push_back({MPV_RENDER_PARAM_INVALID, nullptr});

  if (mpv_render_context_create(&render_, engine_->handle(), params.data()) < 0) {
    render_ = nullptr;
    qWarning("chobi: mpv_render_context_create failed; video output disabled");
    return;
  }

  mpv_render_context_set_update_callback(
      render_,
      [](void* ctx) {
        // mpv thread: never render here, just schedule one update on the GUI thread.
        auto* self = static_cast<MpvVideoWidget*>(ctx);
        if (!self->updatePending_.exchange(true)) {
          QMetaObject::invokeMethod(self, [self] { self->onMpvUpdate(); }, Qt::QueuedConnection);
        }
      },
      this);

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this,
          &MpvVideoWidget::destroyRenderContext, Qt::DirectConnection);
}

void MpvVideoWidget::onMpvUpdate() {
  updatePending_.store(false);
  if (!render_) return;

  makeCurrent();
  const std::uint64_t flags = mpv_render_context_update(render_);
  if (flags & MPV_RENDER_UPDATE_FRAME) {
    const QWindow* handle = window()->windowHandle();
    if (isVisible() && !window()->isMinimized() && handle && handle->isExposed()) {
      update();
    } else {
      // Qt paints nothing while the window is hidden, minimized or unexposed (Wayland: fully
      // covered, another workspace). Consume the frame and report it presented, or mpv's VO
      // waits 200 ms per frame for a swap that never comes: video drops to 5 fps and each
      // seek stalls for 200-400 ms.
      int skip = 1;
      mpv_render_param params[] = {{MPV_RENDER_PARAM_SKIP_RENDERING, &skip},
                                   {MPV_RENDER_PARAM_INVALID, nullptr}};
      mpv_render_context_render(render_, params);
      mpv_render_context_report_swap(render_);
    }
  }
  doneCurrent();
}

void MpvVideoWidget::paintGL() {
  if (!render_) return;
  const qreal dpr = devicePixelRatioF();
  mpv_opengl_fbo fbo{static_cast<int>(defaultFramebufferObject()), qRound(width() * dpr),
                     qRound(height() * dpr), 0};
  int flipY = 1;
  int blockForTargetTime = 0;  // never sleep on the GUI thread
  mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                               {MPV_RENDER_PARAM_FLIP_Y, &flipY},
                               {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &blockForTargetTime},
                               {MPV_RENDER_PARAM_INVALID, nullptr}};
  mpv_render_context_render(render_, params);
  emit framePainted();
}

void MpvVideoWidget::destroyRenderContext() {
  if (!render_) return;
  makeCurrent();
  mpv_render_context_set_update_callback(render_, nullptr, nullptr);
  mpv_render_context_free(render_);
  render_ = nullptr;
  doneCurrent();
}

}  // namespace chobi
