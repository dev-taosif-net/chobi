// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace chobi {

// Thin strip at the top of the window for update notices. Its buttons never take keyboard
// focus, so the player's shortcuts keep working while it is shown.
class UpdateBar final : public QWidget {
  Q_OBJECT

 public:
  explicit UpdateBar(QWidget* parent = nullptr);

  void showAvailable(const QString& version, bool installInApp);
  void showProgress(qint64 received, qint64 total);
  void showInstalled(const QString& version);
  void showError(const QString& message);

  QString text() const;

 signals:
  void installClicked();
  void notesClicked();
  void restartClicked();
  void cancelClicked();

 private:
  enum class Mode { Available, Downloading, Installed, Error };
  void setMode(Mode mode);

  Mode mode_ = Mode::Available;
  QLabel* label_;
  QPushButton* primary_;    // Update now / Restart now / Try again
  QPushButton* notes_;      // What's new
  QPushButton* secondary_;  // Later / Cancel
};

}  // namespace chobi
