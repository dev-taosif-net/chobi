// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/TitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include "app/Icons.h"

namespace chobi {
namespace {

QToolButton* makeButton(QWidget* parent, const QString& tip, Glyph glyph) {
  auto* b = new QToolButton(parent);
  b->setFocusPolicy(Qt::NoFocus);
  b->setAutoRaise(true);
  b->setToolTip(tip);
  b->setAccessibleName(tip);
  b->setIcon(glyphIcon(glyph, QColor(0xf2, 0xf2, 0xf2)));
  return b;
}

}  // namespace

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_StyledBackground);
  setObjectName(QStringLiteral("TitleBar"));
  // Same look as the control bar, with the usual red hover on Close.
  setStyleSheet(QStringLiteral(
      "#TitleBar { background: rgba(16, 16, 16, 210); }"
      "#TitleBar QLabel { color: #f2f2f2; }"
      "#TitleBar QToolButton { background: transparent; border: none; border-radius: 4px;"
      " padding: 3px; }"
      "#TitleBar QToolButton:hover { background: rgba(255, 255, 255, 40); }"
      "#TitleBar QToolButton:pressed { background: rgba(255, 255, 255, 70); }"
      "#TitleBar QToolButton#close:hover { background: #c42b1c; }"
      "#TitleBar QToolButton#close:pressed { background: #a52617; }"));

  titleLabel_ = new QLabel(this);
  // Long titles are elided instead of widening the bar.
  titleLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  auto* minimize = makeButton(this, tr("Minimize"), Glyph::Minimize);
  auto* exitFullscreen = makeButton(this, tr("Exit fullscreen (F)"), Glyph::ExitFullscreen);
  auto* close = makeButton(this, tr("Close"), Glyph::Close);
  close->setObjectName(QStringLiteral("close"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 4, 8, 4);
  layout->setSpacing(6);
  layout->addWidget(titleLabel_, 1);
  layout->addWidget(minimize);
  layout->addWidget(exitFullscreen);
  layout->addWidget(close);

  connect(minimize, &QToolButton::clicked, this, &TitleBar::minimizeClicked);
  connect(exitFullscreen, &QToolButton::clicked, this, &TitleBar::exitFullscreenClicked);
  connect(close, &QToolButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitle(const QString& title) {
  title_ = title;
  titleLabel_->setToolTip(title);
  updateTitleText();
}

void TitleBar::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  updateTitleText();  // the layout has already given the label its new width
}

void TitleBar::updateTitleText() {
  titleLabel_->setText(
      titleLabel_->fontMetrics().elidedText(title_, Qt::ElideRight, titleLabel_->width()));
}

}  // namespace chobi
