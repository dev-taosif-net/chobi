// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/SeekSlider.h"

#include <algorithm>
#include <cmath>

#include <QApplication>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionSlider>

namespace chobi {

SeekSlider::SeekSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {
  setFocusPolicy(Qt::NoFocus);
  setRange(0, 0);
  setTracking(false);
  connect(this, &QSlider::sliderMoved, this, [this](int v) {
    // A click already seeked on press; hand jitter during it is not a drag.
    const int moved = std::abs(pixelFor(v) - pixelFor(pressValue_));
    if (!dragging_ && moved < QApplication::startDragDistance()) return;
    dragging_ = true;
    emit seekRequested(v / 1000.0, false);
  });
  // Like mpv's OSC, a drag ends where its last keyframe seek landed: no seek on release. An exact
  // seek there froze the picture again while mpv decoded up to the drop point; after a plain
  // click it restarted the exact seek the press had just started.
  connect(this, &QSlider::sliderReleased, this, [this] { dragging_ = false; });
}

void SeekSlider::setDuration(double seconds) {
  const double ms = std::isfinite(seconds) && seconds > 0 ? seconds * 1000.0 : 0.0;
  const QSignalBlocker block(this);
  setRange(0, static_cast<int>(std::min(ms, 2.0e9)));
  setEnabled(maximum() > 0);
}

void SeekSlider::setPosition(double seconds) {
  if (isSliderDown() || maximum() <= 0) return;
  const int v = qBound(0, static_cast<int>(seconds * 1000.0), maximum());
  if (pixelFor(v) == pixelFor(value())) return;  // same pixel: skip the repaint
  const QSignalBlocker block(this);
  setValue(v);
}

int SeekSlider::pixelFor(int v) const {
  return QStyle::sliderPositionFromValue(minimum(), maximum(), v, width());
}

int SeekSlider::valueAt(int x) const {
  QStyleOptionSlider opt;
  initStyleOption(&opt);
  const QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
  const QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
  const int span = qMax(1, groove.width() - handle.width());
  const int pos = x - groove.x() - handle.width() / 2;
  return QStyle::sliderValueFromPosition(minimum(), maximum(), qBound(0, pos, span), span);
}

void SeekSlider::mousePressEvent(QMouseEvent* event) {
  dragging_ = false;
  pressValue_ = value();
  if (event->button() == Qt::LeftButton && maximum() > 0) {
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    const QRect handle =
        style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    if (!handle.contains(event->position().toPoint())) {
      const int v = valueAt(qRound(event->position().x()));
      {
        const QSignalBlocker block(this);
        setValue(v);
      }
      pressValue_ = v;
      emit seekRequested(v / 1000.0, true);
    }
  }
  QSlider::mousePressEvent(event);  // lets a drag continue from the new handle position
}

}  // namespace chobi
