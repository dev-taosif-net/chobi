// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/ControlsBar.h"

#include <algorithm>
#include <cmath>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QToolButton>

#include "app/SeekSlider.h"
#include "core/TimeFormat.h"

namespace chobi {
namespace {

QToolButton* makeButton(QWidget* parent, const QString& tip) {
  auto* b = new QToolButton(parent);
  b->setFocusPolicy(Qt::NoFocus);
  b->setAutoRaise(true);
  b->setToolTip(tip);
  return b;
}

QString speedText(double speed) {
  return QString::number(speed, 'g', 3) + QChar(0x00D7);  // "1.25×"
}

}  // namespace

ControlsBar::ControlsBar(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_StyledBackground);
  setObjectName(QStringLiteral("ControlsBar"));
  setStyleSheet(QStringLiteral(
      "#ControlsBar { background: rgba(16, 16, 16, 210); }"
      "#ControlsBar QLabel, #ControlsBar QToolButton { color: #f2f2f2; }"));

  playButton_ = makeButton(this, tr("Play/Pause (Space)"));
  positionLabel_ = new QLabel(this);
  seekSlider_ = new SeekSlider(this);
  durationLabel_ = new QLabel(this);
  muteButton_ = makeButton(this, tr("Mute (M)"));
  volumeSlider_ = new QSlider(Qt::Horizontal, this);
  volumeSlider_->setFocusPolicy(Qt::NoFocus);
  volumeSlider_->setRange(0, 100);
  volumeSlider_->setFixedWidth(90);
  audioButton_ = makeButton(this, tr("Audio track (A)"));
  audioButton_->setText(tr("Audio"));
  audioButton_->setPopupMode(QToolButton::InstantPopup);
  subtitleButton_ = makeButton(this, tr("Subtitles (S)"));
  subtitleButton_->setText(tr("Subtitles"));
  subtitleButton_->setPopupMode(QToolButton::InstantPopup);
  speedButton_ = makeButton(this, tr("Playback speed"));
  speedButton_->setPopupMode(QToolButton::InstantPopup);
  fullscreenButton_ = makeButton(this, tr("Fullscreen (F)"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(6);
  layout->addWidget(playButton_);
  layout->addWidget(positionLabel_);
  layout->addWidget(seekSlider_, 1);
  layout->addWidget(durationLabel_);
  layout->addSpacing(6);
  layout->addWidget(muteButton_);
  layout->addWidget(volumeSlider_);
  layout->addWidget(audioButton_);
  layout->addWidget(subtitleButton_);
  layout->addWidget(speedButton_);
  layout->addWidget(fullscreenButton_);

  connect(playButton_, &QToolButton::clicked, this, &ControlsBar::playPauseClicked);
  connect(muteButton_, &QToolButton::clicked, this, &ControlsBar::muteClicked);
  connect(fullscreenButton_, &QToolButton::clicked, this, &ControlsBar::fullscreenClicked);
  connect(seekSlider_, &SeekSlider::seekRequested, this, &ControlsBar::seekRequested);
  connect(volumeSlider_, &QSlider::valueChanged, this,
          [this](int v) { emit volumeRequested(v); });

  setPaused(true);
  setMuted(false);
  setVolume(100);
  setSpeed(1);
  setFullscreen(false);
  setDuration(0);
}

void ControlsBar::setMenus(QMenu* audio, QMenu* subtitles, QMenu* speed) {
  audioButton_->setMenu(audio);
  subtitleButton_->setMenu(subtitles);
  speedButton_->setMenu(speed);
}

void ControlsBar::setPaused(bool paused) {
  playButton_->setIcon(style()->standardIcon(paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
}

void ControlsBar::setPosition(double seconds) {
  position_ = seconds;
  seekSlider_->setPosition(seconds);
  updateTimeLabels();
}

void ControlsBar::setDuration(double seconds) {
  duration_ = seconds;
  seekSlider_->setDuration(seconds);
  shownSecond_ = -1;
  durationLabel_->setText(QString::fromStdString(core::formatTime(seconds)));
  updateTimeLabels();
}

void ControlsBar::updateTimeLabels() {
  const long long second = static_cast<long long>(std::floor(std::max(0.0, position_)));
  if (second == shownSecond_) return;  // text changes at most once per second
  shownSecond_ = second;
  positionLabel_->setText(QString::fromStdString(core::formatTime(position_, duration_ >= 3600)));
}

void ControlsBar::setVolume(double volume) {
  const QSignalBlocker block(volumeSlider_);
  volumeSlider_->setValue(static_cast<int>(std::lround(volume)));
}

void ControlsBar::setMuted(bool muted) {
  muteButton_->setIcon(
      style()->standardIcon(muted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
}

void ControlsBar::setSpeed(double speed) { speedButton_->setText(speedText(speed)); }

void ControlsBar::setFullscreen(bool fullscreen) {
  fullscreenButton_->setIcon(style()->standardIcon(
      fullscreen ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton));
}

bool ControlsBar::isInteracting() const {
  return seekSlider_->isSliderDown() || volumeSlider_->isSliderDown();
}

}  // namespace chobi
