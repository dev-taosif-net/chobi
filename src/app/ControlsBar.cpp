// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/ControlsBar.h"

#include <algorithm>
#include <cmath>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>

#include "app/Icons.h"
#include "app/SeekSlider.h"
#include "core/TimeFormat.h"

namespace chobi {
namespace {

QIcon icon(Glyph glyph) { return glyphIcon(glyph, QColor(0xf2, 0xf2, 0xf2)); }  // the text colour

QToolButton* makeButton(QWidget* parent, const QString& tip) {
  auto* b = new QToolButton(parent);
  b->setFocusPolicy(Qt::NoFocus);
  b->setAutoRaise(true);
  b->setToolTip(tip);
  return b;
}

// The text with every digit as '0'. Digits share one width in practically every UI font, so
// this measures the widest a time label gets for a given duration.
int reservedWidth(const QLabel* label, QString text) {
  for (QChar& c : text) {
    if (c.isDigit()) c = QLatin1Char('0');
  }
  return label->fontMetrics().horizontalAdvance(text);
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
      "#ControlsBar QLabel, #ControlsBar QToolButton { color: #f2f2f2; }"
      "#ControlsBar QToolButton::menu-indicator { image: none; width: 0; }"));

  playButton_ = makeButton(this, tr("Play/Pause (Space)"));
  positionLabel_ = new QLabel(this);
  positionLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  positionLabel_->setToolTip(tr("Time played"));
  seekSlider_ = new SeekSlider(this);
  remainingLabel_ = new QLabel(this);
  remainingLabel_->setToolTip(tr("Time left / total length"));
  muteButton_ = makeButton(this, tr("Mute (M)"));
  volumeSlider_ = new QSlider(Qt::Horizontal, this);
  volumeSlider_->setFocusPolicy(Qt::NoFocus);
  volumeSlider_->setRange(0, 100);
  volumeSlider_->setFixedWidth(90);
  audioButton_ = makeButton(this, tr("Audio track (A)"));
  audioButton_->setIcon(icon(Glyph::AudioTrack));
  audioButton_->setAccessibleName(tr("Audio track"));
  audioButton_->setPopupMode(QToolButton::InstantPopup);
  subtitleButton_ = makeButton(this, tr("Subtitles (S)"));
  subtitleButton_->setIcon(icon(Glyph::Subtitles));
  subtitleButton_->setAccessibleName(tr("Subtitles"));
  subtitleButton_->setPopupMode(QToolButton::InstantPopup);
  speedButton_ = makeButton(this, tr("Playback speed"));
  speedButton_->setPopupMode(QToolButton::InstantPopup);
  fullscreenButton_ = makeButton(this, tr("Fullscreen (F)"));
  settingsButton_ = makeButton(this, tr("Settings"));
  settingsButton_->setIcon(icon(Glyph::Settings));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(6);
  layout->addWidget(playButton_);
  layout->addWidget(positionLabel_);
  layout->addWidget(seekSlider_, 1);
  layout->addWidget(remainingLabel_);
  layout->addSpacing(6);
  layout->addWidget(muteButton_);
  layout->addWidget(volumeSlider_);
  layout->addWidget(audioButton_);
  layout->addWidget(subtitleButton_);
  layout->addWidget(speedButton_);
  layout->addWidget(fullscreenButton_);
  layout->addWidget(settingsButton_);

  connect(playButton_, &QToolButton::clicked, this, &ControlsBar::playPauseClicked);
  connect(muteButton_, &QToolButton::clicked, this, &ControlsBar::muteClicked);
  connect(fullscreenButton_, &QToolButton::clicked, this, &ControlsBar::fullscreenClicked);
  connect(settingsButton_, &QToolButton::clicked, this, &ControlsBar::settingsClicked);
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
  playButton_->setIcon(icon(paused ? Glyph::Play : Glyph::Pause));
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
  updateTimeLabels();
  // Reserve the widest text up front (the times at the start or end of the video), so the
  // seek bar doesn't shift when e.g. 10:00 becomes 9:59.
  const QString widest = QString::fromStdString(core::formatTime(seconds, seconds >= 3600));
  positionLabel_->setMinimumWidth(reservedWidth(positionLabel_, widest));
  remainingLabel_->setMinimumWidth(reservedWidth(remainingLabel_, remainingText(0)));
}

void ControlsBar::updateTimeLabels() {
  const long long second = static_cast<long long>(std::floor(std::max(0.0, position_)));
  if (second == shownSecond_) return;  // text changes at most once per second
  shownSecond_ = second;
  positionLabel_->setText(QString::fromStdString(core::formatTime(position_, duration_ >= 3600)));
  remainingLabel_->setText(remainingText(position_));
}

QString ControlsBar::remainingText(double position) const {
  const std::string total = core::formatTime(duration_);
  if (duration_ <= 0) return QString::fromStdString(total);  // unknown length, e.g. a live stream
  return QString::fromStdString(core::formatRemaining(position, duration_, duration_ >= 3600) +
                                " / " + total);
}

void ControlsBar::setVolume(double volume) {
  const QSignalBlocker block(volumeSlider_);
  volumeSlider_->setValue(static_cast<int>(std::lround(volume)));
}

void ControlsBar::setMuted(bool muted) {
  muteButton_->setIcon(icon(muted ? Glyph::Muted : Glyph::Volume));
}

void ControlsBar::setSpeed(double speed) { speedButton_->setText(speedText(speed)); }

void ControlsBar::setFullscreen(bool fullscreen) {
  fullscreenButton_->setIcon(icon(fullscreen ? Glyph::ExitFullscreen : Glyph::Fullscreen));
}

bool ControlsBar::isInteracting() const {
  return seekSlider_->isSliderDown() || volumeSlider_->isSliderDown();
}

}  // namespace chobi
