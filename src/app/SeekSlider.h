// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSlider>

namespace chobi {

// Progress bar in milliseconds. Click anywhere to jump, drag for keyframe-fast scrubbing,
// release for an exact seek. Repaints only when the handle moves by at least one pixel.
class SeekSlider final : public QSlider {
  Q_OBJECT

 public:
  explicit SeekSlider(QWidget* parent = nullptr);

  void setDuration(double seconds);
  void setPosition(double seconds);

 signals:
  void seekRequested(double seconds, bool exact);

 protected:
  void mousePressEvent(QMouseEvent* event) override;

 private:
  int pixelFor(int value) const;
  int valueAt(int x) const;
};

}  // namespace chobi
