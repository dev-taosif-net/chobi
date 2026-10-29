// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

#include "app/Settings.h"

class QCheckBox;

namespace chobi {

class SettingsDialog final : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(const AppSettings& current, QWidget* parent = nullptr);

  // `base` with the user's choices applied (fields the dialog doesn't edit are kept).
  AppSettings settings(AppSettings base) const;

 signals:
  void forgetPositionsRequested();

 private:
  QCheckBox* resume_;
  QCheckBox* rememberVolume_;
};

}  // namespace chobi
