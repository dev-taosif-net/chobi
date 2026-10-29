// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace chobi {

SettingsDialog::SettingsDialog(const AppSettings& current, QWidget* parent) : QDialog(parent) {
  setWindowTitle(tr("Settings"));
  setObjectName(QStringLiteral("SettingsDialog"));

  resume_ = new QCheckBox(tr("Resume videos from where you stopped"), this);
  resume_->setObjectName(QStringLiteral("resumePlayback"));
  resume_->setChecked(current.resumePlayback);

  auto* forget = new QPushButton(tr("Forget saved positions"), this);
  forget->setObjectName(QStringLiteral("forgetPositions"));
  forget->setAutoDefault(false);
  connect(forget, &QPushButton::clicked, this, [this, forget] {
    emit forgetPositionsRequested();
    forget->setEnabled(false);
    forget->setText(tr("Saved positions forgotten"));
  });

  rememberVolume_ = new QCheckBox(tr("Remember volume level for the next video"), this);
  rememberVolume_->setObjectName(QStringLiteral("rememberVolume"));
  rememberVolume_->setChecked(current.rememberVolume);

  auto* hint = new QLabel(
      tr("A position is saved after 30 seconds of watching. It is forgotten when the video is "
         "watched to the end, when it hasn't been watched for 7 days, or when the video file "
         "is deleted."),
      this);
  hint->setWordWrap(true);
  hint->setEnabled(false);  // dimmed helper text

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(resume_);
  layout->addWidget(hint);
  layout->addWidget(forget, 0, Qt::AlignLeft);
  layout->addSpacing(8);
  layout->addWidget(rememberVolume_);
  layout->addStretch();
  layout->addWidget(buttons);
  setMinimumWidth(380);
}

AppSettings SettingsDialog::settings(AppSettings base) const {
  base.resumePlayback = resume_->isChecked();
  base.rememberVolume = rememberVolume_->isChecked();
  return base;
}

}  // namespace chobi
