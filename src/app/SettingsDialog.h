// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

#include "app/Settings.h"

class QCheckBox;

namespace chobi {

class SettingsDialog final : public QDialog {
  Q_OBJECT

 public:
  // `updatesAvailable`: whether this build can update itself (shows the Updates section).
  explicit SettingsDialog(const AppSettings& current, bool updatesAvailable = false,
                          QWidget* parent = nullptr);

  // `base` with the user's choices applied (fields the dialog doesn't edit are kept).
  AppSettings settings(AppSettings base) const;

 signals:
  void forgetPositionsRequested();
  void checkNowRequested();

 private:
  QCheckBox* resume_;
  QCheckBox* rememberVolume_;
  QCheckBox* checkForUpdates_;
};

}  // namespace chobi
