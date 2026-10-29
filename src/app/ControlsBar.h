// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class QLabel;
class QMenu;
class QSlider;
class QToolButton;

namespace chobi {

class SeekSlider;

// Bottom control strip. Self-contained (signals in, setters out) so it can be hosted as an
// overlay today or in its own window if a native-window video surface is ever used.
class ControlsBar final : public QWidget {
  Q_OBJECT

 public:
  explicit ControlsBar(QWidget* parent = nullptr);

  void setMenus(QMenu* audio, QMenu* subtitles, QMenu* speed);
  void setPaused(bool paused);
  void setPosition(double seconds);
  void setDuration(double seconds);
  void setVolume(double volume);
  void setMuted(bool muted);
  void setSpeed(double speed);
  void setFullscreen(bool fullscreen);
  bool isInteracting() const;

 signals:
  void playPauseClicked();
  void seekRequested(double seconds, bool exact);
  void volumeRequested(double volume);
  void muteClicked();
  void fullscreenClicked();

 private:
  void updateTimeLabels();

  QToolButton* playButton_;
  QLabel* positionLabel_;
  SeekSlider* seekSlider_;
  QLabel* durationLabel_;
  QToolButton* muteButton_;
  QSlider* volumeSlider_;
  QToolButton* audioButton_;
  QToolButton* subtitleButton_;
  QToolButton* speedButton_;
  QToolButton* fullscreenButton_;
  double position_ = 0;
  double duration_ = 0;
  long long shownSecond_ = -1;
};

}  // namespace chobi
