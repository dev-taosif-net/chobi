// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QTimer>
#include <QWidget>

#include "app/MediaProbe.h"
#include "core/PlaybackController.h"

class QLabel;
class QMenu;

namespace chobi {

class ControlsBar;
class PlayerEngine;
class ResumeService;

class MainWindow final : public QWidget, public core::IWindowController {
  Q_OBJECT

 public:
  // `videoSurface` becomes a child of the window. `resume` may be null (no position memory).
  MainWindow(PlayerEngine* engine, QWidget* videoSurface, std::shared_ptr<ResumeService> resume,
             std::vector<std::string> preferredLanguages, QWidget* parent = nullptr);
  ~MainWindow() override;

  // Path, file:// URI or network URL. Safe to call while something else is playing.
  void openLocation(const QString& location);

  // core::IWindowController
  bool isFullscreen() const override;
  void setFullscreen(bool fullscreen) override;
  void minimize() override;
  void requestOpenFile() override;
  void requestQuit() override;
  void showMessage(const std::string& text) override;

  // Introspection for tests.
  QString currentLocation() const { return currentLocation_; }
  QString centerText() const;
  bool controlsVisible() const;

 signals:
  void probeFinished();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

 private:
  void buildMenus();
  void connectEngine();
  void rebuildAudioMenu();
  void rebuildSubtitleMenu();
  void rebuildSpeedMenu();
  void onProbeFinished(quint64 generation, const ProbeResult& result);
  void onFileLoaded();
  void onFileFailed(const QString& message);
  void openFileDialog();
  void openSubtitleDialog();
  void loadSubtitleFile(const QString& path);
  void saveResumePosition();
  void showControls();
  void maybeHideControls();
  void setCenterText(const QString& text);
  void updateTitle();
  void layoutOverlays();

  PlayerEngine* engine_;
  QWidget* video_;
  std::shared_ptr<ResumeService> resume_;
  std::vector<std::string> preferredLanguages_;
  core::PlaybackController controller_;

  ControlsBar* controls_;
  QLabel* osd_;
  QLabel* center_;
  QMenu* audioMenu_;
  QMenu* subtitleMenu_;
  QMenu* speedMenu_;
  QTimer hideTimer_;
  QTimer osdTimer_;
  QTimer loadingTimer_;

  quint64 generation_ = 0;
  QString currentLocation_;
  QString displayName_;
  std::string resumeKey_;
  std::vector<ExternalSubtitle> pendingSubtitles_;
  QString lastError_;
  QString lastDirectory_;
  bool wasMaximized_ = false;
  bool firstFramePending_ = false;
  int wheelAccumulator_ = 0;
};

}  // namespace chobi
