// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

#include "app/MediaProbe.h"
#include "app/Settings.h"
#include "core/PlaybackController.h"

class QLabel;
class QMenu;

namespace chobi {

class ControlsBar;
class InstanceServer;
class PlayerEngine;
class ResumeService;
class SettingsStore;
class TitleBar;
class UpdateBar;
class Updater;

class MainWindow final : public QWidget, public core::IWindowController {
  Q_OBJECT

 public:
  struct Services {
    std::shared_ptr<ResumeService> resume;        // null: no position memory
    std::shared_ptr<SettingsStore> settingsStore;  // null: settings aren't persisted
    AppSettings settings;                          // as loaded at startup
    std::vector<std::string> preferredLanguages;   // subtitle preference, ISO 639-1
    Updater* updater = nullptr;                    // null or disabled: no update UI
    InstanceServer* instance = nullptr;            // null: other launches open their own window
  };

  // `videoSurface` becomes a child of the window.
  MainWindow(PlayerEngine* engine, QWidget* videoSurface, Services services,
             QWidget* parent = nullptr);
  ~MainWindow() override;

  const AppSettings& settings() const { return settings_; }
  void applySettings(const AppSettings& settings);  // takes effect now and is saved
  void openSettings();

  // Path, file:// URI or network URL. Safe to call while something else is playing.
  void openLocation(const QString& location);

  // A later launch (double-click in the file manager, "Open With") handed over its video:
  // play it here (empty: nothing new to play) and bring this window to the front.
  void openFromAnotherLaunch(const QString& location, const QByteArray& activationToken);

  // core::IWindowController
  bool isFullscreen() const override;
  void setFullscreen(bool fullscreen) override;
  void minimize() override;
  void requestOpenFile() override;
  void requestQuit() override;
  void showMessage(const std::string& text) override;
  void showPosition() override;

  // Introspection for tests.
  QString currentLocation() const { return currentLocation_; }
  QString centerText() const;
  QString osdText() const;  // "" when hidden
  bool controlsVisible() const;
  bool titleBarVisible() const;
  QString updateBarText() const;  // "" when hidden

 signals:
  void probeFinished();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
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
  void saveSession();  // resume position + remembered volume, written off the UI thread
  void persistSettings();
  void showControls();
  void maybeHideControls();
  void setTitleBarVisible(bool visible);
  void setCenterText(const QString& text);
  void setOsdText(const QString& text);
  void updateTitle();
  void layoutOverlays();
  int osdTop() const;
  void connectUpdater();
  void checkForUpdates(bool userInitiated);
  void restartIntoUpdate(const QString& executable);
  void quitForInstaller();
  void repeatSeek();
  void stopSeekRepeat();

  PlayerEngine* engine_;
  QWidget* video_;
  std::shared_ptr<ResumeService> resume_;
  std::shared_ptr<SettingsStore> settingsStore_;
  AppSettings settings_;
  std::vector<std::string> preferredLanguages_;
  core::PlaybackController controller_;
  Updater* updater_;
  InstanceServer* instance_;
  UpdateBar* updateBar_ = nullptr;

  ControlsBar* controls_;
  TitleBar* titleBar_;  // fullscreen only
  QLabel* osd_;
  QLabel* center_;
  QMenu* audioMenu_;
  QMenu* subtitleMenu_;
  QMenu* speedMenu_;
  QTimer hideTimer_;
  QTimer osdTimer_;
  bool osdFollowsPosition_ = false;
  QTimer loadingTimer_;
  // Held ←/→ repeat on stock mpv's timing, not the desktop's (GNOME: 500 ms, then 33/s).
  QTimer seekRepeatTimer_;
  QElapsedTimer seekRepeatClock_;
  core::Action seekRepeatAction_ = core::Action::None;
  int seekRepeatKey_ = 0;
  qint64 seekRepeatsDone_ = 0;

  quint64 generation_ = 0;
  QString currentLocation_;
  QString displayName_;
  std::string resumeKey_;
  QString resumePath_;
  std::vector<ExternalSubtitle> pendingSubtitles_;
  QString lastError_;
  QString lastDirectory_;
  bool wasMaximized_ = false;
  bool sessionSaved_ = false;
  QString restartPath_;  // updated AppImage waiting for "Restart now"
  bool firstFramePending_ = false;
  int wheelAccumulator_ = 0;
  bool dragPending_ = false;  // left button went down on the picture; may become a window move
  QPoint dragStart_;          // global position of that press
};

}  // namespace chobi
