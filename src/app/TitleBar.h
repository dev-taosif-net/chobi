// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class QLabel;

namespace chobi {

// Top strip for fullscreen, where the window's own title bar is gone: the video's title and the
// window buttons. MainWindow shows and hides it together with the control bar.
class TitleBar final : public QWidget {
  Q_OBJECT

 public:
  explicit TitleBar(QWidget* parent = nullptr);

  void setTitle(const QString& title);

 signals:
  void minimizeClicked();
  void exitFullscreenClicked();
  void closeClicked();

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private:
  void updateTitleText();

  QLabel* titleLabel_;
  QString title_;
};

}  // namespace chobi
