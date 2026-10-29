// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/UpdateBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace chobi {
namespace {

QPushButton* makeButton(QWidget* parent) {
  auto* b = new QPushButton(parent);
  b->setFocusPolicy(Qt::NoFocus);
  b->setAutoDefault(false);
  return b;
}

}  // namespace

UpdateBar::UpdateBar(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_StyledBackground);
  setObjectName(QStringLiteral("UpdateBar"));
  setStyleSheet(QStringLiteral(
      "#UpdateBar { background: rgba(14, 116, 144, 235); }"
      "#UpdateBar QLabel { color: #ffffff; }"));

  label_ = new QLabel(this);
  primary_ = makeButton(this);
  notes_ = makeButton(this);
  notes_->setText(tr("What's new"));
  secondary_ = makeButton(this);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 6, 12, 6);
  layout->addWidget(label_, 1);
  layout->addWidget(notes_);
  layout->addWidget(primary_);
  layout->addWidget(secondary_);

  connect(primary_, &QPushButton::clicked, this, [this] {
    switch (mode_) {
      case Mode::Available:
      case Mode::Error: emit installClicked(); break;
      case Mode::Installed: emit restartClicked(); break;
      case Mode::Downloading: break;
    }
  });
  connect(notes_, &QPushButton::clicked, this, &UpdateBar::notesClicked);
  connect(secondary_, &QPushButton::clicked, this, [this] {
    if (mode_ == Mode::Downloading) emit cancelClicked();
    hide();
  });
  hide();
}

void UpdateBar::setMode(Mode mode) {
  mode_ = mode;
  primary_->setVisible(mode != Mode::Downloading);
  notes_->setVisible(mode == Mode::Available);
  secondary_->setText(mode == Mode::Downloading ? tr("Cancel") : tr("Later"));
  show();
  raise();
}

void UpdateBar::showAvailable(const QString& version, bool installInApp) {
  label_->setText(tr("Chobi %1 is available.").arg(version));
  primary_->setText(installInApp ? tr("Update now") : tr("Download"));
  setMode(Mode::Available);
}

void UpdateBar::showProgress(qint64 received, qint64 total) {
  const int percent = total > 0 ? static_cast<int>(received * 100 / total) : 0;
  label_->setText(tr("Downloading update… %1%").arg(percent));
  if (mode_ != Mode::Downloading) setMode(Mode::Downloading);
}

void UpdateBar::showInstalled(const QString& version) {
  label_->setText(tr("Chobi %1 is installed. Restart to use it; your video continues where "
                     "you left off.").arg(version));
  primary_->setText(tr("Restart now"));
  setMode(Mode::Installed);
}

void UpdateBar::showError(const QString& message) {
  label_->setText(tr("Update failed: %1").arg(message));
  primary_->setText(tr("Try again"));
  setMode(Mode::Error);
}

QString UpdateBar::text() const { return label_->text(); }

}  // namespace chobi
