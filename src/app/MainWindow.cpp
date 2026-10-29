// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/MainWindow.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QThreadPool>
#include <QWheelEvent>

#include "app/ControlsBar.h"
#include "app/MediaPath.h"
#include "app/ResumeService.h"
#include "core/Language.h"
#include "core/MediaFormats.h"
#include "core/SubtitleFinder.h"
#include "core/TimeFormat.h"
#include "core/TrackLabel.h"
#include "engine/PlayerEngine.h"

namespace chobi {
namespace {

constexpr int kControlsHideMs = 2500;
constexpr int kOsdMs = 1500;
constexpr int kLoadingIndicatorDelayMs = 300;
constexpr double kSpeeds[] = {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};

core::Key toCoreKey(int key) {
  switch (key) {
    case Qt::Key_Left: return core::Key::Left;
    case Qt::Key_Right: return core::Key::Right;
    case Qt::Key_Up: return core::Key::Up;
    case Qt::Key_Down: return core::Key::Down;
    case Qt::Key_Space: return core::Key::Space;
    case Qt::Key_Escape: return core::Key::Escape;
    case Qt::Key_A: return core::Key::A;
    case Qt::Key_F: return core::Key::F;
    case Qt::Key_M: return core::Key::M;
    case Qt::Key_O: return core::Key::O;
    case Qt::Key_Q: return core::Key::Q;
    case Qt::Key_S: return core::Key::S;
    default: return core::Key::Unknown;
  }
}

core::KeyModifiers toCoreModifiers(Qt::KeyboardModifiers m) {
  // KeypadModifier is ignored so numpad arrows behave like the arrow keys.
  return {m.testFlag(Qt::ControlModifier), m.testFlag(Qt::ShiftModifier),
          m.testFlag(Qt::AltModifier), m.testFlag(Qt::MetaModifier)};
}

bool repeatable(core::Action a) {
  return a == core::Action::SeekForward || a == core::Action::SeekBackward ||
         a == core::Action::VolumeUp || a == core::Action::VolumeDown;
}

QString extensionFilter(const std::vector<std::string_view>& exts) {
  QStringList patterns;
  for (std::string_view e : exts) {
    patterns << QStringLiteral("*.") + QString::fromLatin1(e.data(), static_cast<int>(e.size()));
  }
  return patterns.join(QLatin1Char(' '));
}

QString idleHint() {
  return MainWindow::tr("Drop a video here, or press O to open a file.");
}

}  // namespace

MainWindow::MainWindow(PlayerEngine* engine, QWidget* videoSurface,
                       std::shared_ptr<ResumeService> resume,
                       std::vector<std::string> preferredLanguages, QWidget* parent)
    : QWidget(parent),
      engine_(engine),
      video_(videoSurface),
      resume_(std::move(resume)),
      preferredLanguages_(std::move(preferredLanguages)),
      controller_(*engine, *this) {
  setWindowTitle(QStringLiteral("Chobi"));
  setAcceptDrops(true);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  setMinimumSize(360, 220);
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, Qt::black);
  setPalette(pal);

  video_->setParent(this);
  video_->setFocusPolicy(Qt::NoFocus);
  video_->setMouseTracking(true);

  controls_ = new ControlsBar(this);

  osd_ = new QLabel(this);
  osd_->setAttribute(Qt::WA_TransparentForMouseEvents);
  osd_->setStyleSheet(QStringLiteral(
      "QLabel { background: rgba(0,0,0,170); color: #fff; padding: 6px 10px; border-radius: 4px; }"));
  osd_->hide();

  center_ = new QLabel(this);
  center_->setAttribute(Qt::WA_TransparentForMouseEvents);
  center_->setAlignment(Qt::AlignCenter);
  center_->setWordWrap(true);
  center_->setStyleSheet(QStringLiteral("QLabel { color: #d8d8d8; background: transparent; }"));
  QFont centerFont = center_->font();
  centerFont.setPointSizeF(centerFont.pointSizeF() * 1.2);
  center_->setFont(centerFont);

  buildMenus();
  controls_->setMenus(audioMenu_, subtitleMenu_, speedMenu_);

  hideTimer_.setSingleShot(true);
  hideTimer_.setInterval(kControlsHideMs);
  connect(&hideTimer_, &QTimer::timeout, this, &MainWindow::maybeHideControls);
  osdTimer_.setSingleShot(true);
  osdTimer_.setInterval(kOsdMs);
  connect(&osdTimer_, &QTimer::timeout, osd_, &QLabel::hide);
  loadingTimer_.setSingleShot(true);
  loadingTimer_.setInterval(kLoadingIndicatorDelayMs);
  connect(&loadingTimer_, &QTimer::timeout, this, [this] {
    if (!engine_->state().loaded && lastError_.isEmpty()) setCenterText(tr("Loading…"));
  });

  connect(controls_, &ControlsBar::playPauseClicked, this, [this] { controller_.togglePause(); });
  connect(controls_, &ControlsBar::seekRequested, this, [this](double s, bool exact) {
    if (exact) {
      controller_.seekTo(s);
    } else if (engine_->state().loaded) {
      engine_->seekAbsolute(s, false);
    }
  });
  connect(controls_, &ControlsBar::volumeRequested, this,
          [this](double v) { engine_->setVolume(core::clampVolume(v)); });
  connect(controls_, &ControlsBar::muteClicked, this, [this] { controller_.toggleMute(); });
  connect(controls_, &ControlsBar::fullscreenClicked, this,
          [this] { controller_.toggleFullscreen(); });

  connectEngine();
  setCenterText(idleHint());
  resize(960, 540);
}

MainWindow::~MainWindow() {
  // Probe and resume jobs capture `this`/shared state; let them finish first.
  QThreadPool::globalInstance()->waitForDone();
}

void MainWindow::connectEngine() {
  connect(engine_, &PlayerEngine::positionChanged, controls_, &ControlsBar::setPosition);
  connect(engine_, &PlayerEngine::durationChanged, controls_, &ControlsBar::setDuration);
  connect(engine_, &PlayerEngine::volumeChanged, controls_, &ControlsBar::setVolume);
  connect(engine_, &PlayerEngine::mutedChanged, controls_, &ControlsBar::setMuted);
  connect(engine_, &PlayerEngine::speedChanged, controls_, &ControlsBar::setSpeed);
  connect(engine_, &PlayerEngine::pausedChanged, this, [this](bool paused) {
    controls_->setPaused(paused);
    if (paused) {
      showControls();
      hideTimer_.stop();
    } else {
      hideTimer_.start();
    }
  });
  connect(engine_, &PlayerEngine::tracksChanged, this, [this] { controller_.onTracksChanged(); });
  connect(engine_, &PlayerEngine::playbackRestarted, this, [this] { controller_.onSeekFinished(); });
  connect(engine_, &PlayerEngine::fileStarted, this, [this] { controller_.onFileChanged(); });
  connect(engine_, &PlayerEngine::fileLoaded, this, &MainWindow::onFileLoaded);
  connect(engine_, &PlayerEngine::fileFailed, this, &MainWindow::onFileFailed);
  connect(engine_, &PlayerEngine::mediaTitleChanged, this, &MainWindow::updateTitle);
  connect(engine_, &PlayerEngine::bufferingChanged, this, [this](bool buffering) {
    if (buffering) {
      setCenterText(tr("Buffering…"));
    } else if (lastError_.isEmpty() && engine_->state().loaded) {
      setCenterText(QString());
    }
  });
  connect(engine_, &PlayerEngine::warningRaised, this,
          [this](const QString& text) { showMessage(text.toStdString()); });
}

void MainWindow::buildMenus() {
  audioMenu_ = new QMenu(tr("Audio"), this);
  subtitleMenu_ = new QMenu(tr("Subtitles"), this);
  speedMenu_ = new QMenu(tr("Speed"), this);
  connect(audioMenu_, &QMenu::aboutToShow, this, &MainWindow::rebuildAudioMenu);
  connect(subtitleMenu_, &QMenu::aboutToShow, this, &MainWindow::rebuildSubtitleMenu);
  connect(speedMenu_, &QMenu::aboutToShow, this, &MainWindow::rebuildSpeedMenu);
  for (QMenu* m : {audioMenu_, subtitleMenu_, speedMenu_}) {
    connect(m, &QMenu::aboutToHide, this, [this] { hideTimer_.start(); });
  }
}

void MainWindow::rebuildAudioMenu() {
  audioMenu_->clear();
  auto* group = new QActionGroup(audioMenu_);
  int index = 0;
  for (const core::Track& t : engine_->tracks()) {
    if (t.type != core::TrackType::Audio) continue;
    const QString text = QStringLiteral("%1. %2").arg(++index).arg(
        QString::fromStdString(core::trackLabel(t)));
    QAction* a = audioMenu_->addAction(text);
    a->setCheckable(true);
    a->setChecked(t.selected);
    group->addAction(a);
    const std::int64_t id = t.id;
    connect(a, &QAction::triggered, this, [this, id] { engine_->selectAudioTrack(id); });
  }
  if (index == 0) audioMenu_->addAction(tr("No audio tracks"))->setEnabled(false);
}

void MainWindow::rebuildSubtitleMenu() {
  subtitleMenu_->clear();
  auto* group = new QActionGroup(subtitleMenu_);
  const bool visible = engine_->state().subtitlesVisible;
  struct Item {
    QString text;
    std::int64_t id;
    bool selected;
  };
  std::vector<Item> items;
  bool anySelected = false;
  for (const core::Track& t : engine_->tracks()) {
    if (t.type != core::TrackType::Subtitle) continue;
    anySelected = anySelected || t.selected;
    items.push_back({QStringLiteral("%1. %2").arg(items.size() + 1).arg(
                         QString::fromStdString(core::trackLabel(t))),
                     t.id, t.selected});
  }

  QAction* off = subtitleMenu_->addAction(tr("Off\tS"));
  off->setCheckable(true);
  off->setChecked(!anySelected || !visible);
  group->addAction(off);
  connect(off, &QAction::triggered, this, [this] { engine_->selectSubtitleTrack(0); });

  for (const Item& item : items) {
    QAction* a = subtitleMenu_->addAction(item.text);
    a->setCheckable(true);
    a->setChecked(item.selected && visible);
    group->addAction(a);
    const std::int64_t id = item.id;
    connect(a, &QAction::triggered, this, [this, id] {
      engine_->selectSubtitleTrack(id);
      engine_->setSubtitlesVisible(true);
    });
  }
  subtitleMenu_->addSeparator();
  QAction* load = subtitleMenu_->addAction(tr("Load subtitle file…"));
  load->setEnabled(engine_->state().loaded);
  connect(load, &QAction::triggered, this, &MainWindow::openSubtitleDialog);
}

void MainWindow::rebuildSpeedMenu() {
  speedMenu_->clear();
  auto* group = new QActionGroup(speedMenu_);
  for (double speed : kSpeeds) {
    QAction* a = speedMenu_->addAction(QString::number(speed, 'g', 3) + QChar(0x00D7));
    a->setCheckable(true);
    a->setChecked(qFuzzyCompare(speed, engine_->state().speed));
    group->addAction(a);
    connect(a, &QAction::triggered, this, [this, speed] { engine_->setSpeed(speed); });
  }
}

void MainWindow::openLocation(const QString& location) {
  const std::optional<MediaSource> source = mediaSourceFromArgument(location);
  if (!source) {
    showMessage(tr("Cannot open %1").arg(location).toStdString());
    return;
  }

  saveResumePosition();
  const quint64 generation = ++generation_;
  currentLocation_ = source->location;
  displayName_ = source->isLocalFile ? QFileInfo(source->location).fileName() : source->location;
  resumeKey_.clear();
  lastError_.clear();
  setCenterText(QString());
  loadingTimer_.start();
  updateTitle();

  if (!source->isLocalFile) {
    engine_->open(source->location.toStdString(), 0, {});
    emit probeFinished();
    return;
  }

  lastDirectory_ = QFileInfo(source->location).absolutePath();
  const QString path = source->location;
  const std::vector<std::string> prefs = preferredLanguages_;
  ResumeService* resume = resume_.get();
  QThreadPool::globalInstance()->start([this, generation, path, prefs, resume] {
    const ProbeResult result = probeMedia(path, prefs, resume);
    QMetaObject::invokeMethod(
        this, [this, generation, result] { onProbeFinished(generation, result); },
        Qt::QueuedConnection);
  });
}

void MainWindow::onProbeFinished(quint64 generation, const ProbeResult& result) {
  if (generation != generation_) return;  // the user already opened something else
  if (!result.exists) {
    onFileFailed(result.error);
    emit probeFinished();
    return;
  }

  resumeKey_ = result.resumeKey;
  std::vector<core::SubtitleFile> subtitles;
  for (std::size_t i = 0; i < result.subtitles.size(); ++i) {
    const ExternalSubtitle& s = result.subtitles[i];
    subtitles.push_back({s.path.toStdString(), s.info.fileName, s.info.languageTag, i == 0});
  }
  const double start = result.resumePosition.value_or(0.0);
  engine_->open(result.path.toStdString(), start, std::move(subtitles));
  if (start > 0) showMessage(tr("Resuming at %1").arg(QString::fromStdString(core::formatTime(start))).toStdString());
  emit probeFinished();
}

void MainWindow::onFileLoaded() {
  loadingTimer_.stop();
  lastError_.clear();
  setCenterText(QString());
  updateTitle();
  showControls();
}

void MainWindow::onFileFailed(const QString& message) {
  loadingTimer_.stop();
  lastError_ = message;
  resumeKey_.clear();
  setCenterText(tr("Cannot play “%1”").arg(displayName_) + QStringLiteral("\n\n") + message);
  showControls();
  updateTitle();
}

void MainWindow::openFileDialog() {
  const QString videos = extensionFilter(core::videoExtensions());
  const QString filter = tr("Videos (%1);;All files (*)").arg(videos);
  const QString path =
      QFileDialog::getOpenFileName(this, tr("Open video"), lastDirectory_, filter);
  if (!path.isEmpty()) openLocation(path);
}

void MainWindow::openSubtitleDialog() {
  const QString subs = extensionFilter(core::manualSubtitleExtensions());
  const QString path = QFileDialog::getOpenFileName(
      this, tr("Load subtitle file"), lastDirectory_,
      tr("Subtitles (%1);;All files (*)").arg(subs));
  if (!path.isEmpty()) loadSubtitleFile(path);
}

void MainWindow::loadSubtitleFile(const QString& path) {
  if (!engine_->state().loaded) return;
  const QString fileName = QFileInfo(path).fileName();
  // "Movie.bn.srt" -> "bn": the last dot-separated token before the extension, if any.
  const std::string stem(core::fileStem(fileName.toStdString()));
  const std::size_t dot = stem.rfind('.');
  core::LanguageTag tag;
  std::string language;
  if (dot != std::string::npos && core::parseLanguageTag(stem.substr(dot + 1), &tag)) {
    language = tag.full;
  }
  engine_->addSubtitleFile(path.toStdString(), true, fileName.toStdString(), language);
  engine_->setSubtitlesVisible(true);
  showMessage(tr("Subtitle: %1").arg(fileName).toStdString());
}

void MainWindow::saveResumePosition() {
  if (resumeKey_.empty() || !resume_) return;
  const core::PlaybackState& s = engine_->state();
  if (!s.loaded) return;
  std::shared_ptr<ResumeService> resume = resume_;
  const std::string key = resumeKey_;
  const double position = s.position;
  const double duration = s.duration;
  QThreadPool::globalInstance()->start(
      [resume, key, position, duration] { resume->record(key, position, duration); });
}

bool MainWindow::isFullscreen() const { return isFullScreen(); }

void MainWindow::setFullscreen(bool fullscreen) {
  if (fullscreen == isFullScreen()) return;
  if (fullscreen) {
    wasMaximized_ = isMaximized();
    showFullScreen();
  } else if (wasMaximized_) {
    showMaximized();
  } else {
    showNormal();
  }
}

void MainWindow::minimize() {
  saveResumePosition();
  showMinimized();
}

void MainWindow::requestOpenFile() { QTimer::singleShot(0, this, &MainWindow::openFileDialog); }

void MainWindow::requestQuit() { close(); }

void MainWindow::showMessage(const std::string& text) {
  osd_->setText(QString::fromStdString(text));
  osd_->adjustSize();
  osd_->move(16, 16);
  osd_->show();
  osd_->raise();
  osdTimer_.start();
}

QString MainWindow::centerText() const { return center_->isVisible() ? center_->text() : QString(); }

bool MainWindow::controlsVisible() const { return controls_->isVisible(); }

void MainWindow::setCenterText(const QString& text) {
  center_->setText(text);
  center_->setVisible(!text.isEmpty());
}

void MainWindow::updateTitle() {
  QString title = QString::fromStdString(engine_->state().mediaTitle);
  if (title.isEmpty()) title = displayName_;
  setWindowTitle(title.isEmpty() ? QStringLiteral("Chobi") : title + QStringLiteral(" — Chobi"));
}

void MainWindow::showControls() {
  controls_->show();
  controls_->raise();
  video_->unsetCursor();
  unsetCursor();
  if (!engine_->state().paused && engine_->state().loaded) hideTimer_.start();
}

void MainWindow::maybeHideControls() {
  const core::PlaybackState& s = engine_->state();
  if (!s.loaded || s.paused) return;
  if (controls_->underMouse() || controls_->isInteracting() || QApplication::activePopupWidget()) {
    hideTimer_.start();
    return;
  }
  controls_->hide();
  video_->setCursor(Qt::BlankCursor);
  setCursor(Qt::BlankCursor);
}

void MainWindow::layoutOverlays() {
  video_->setGeometry(rect());
  const int barHeight = controls_->sizeHint().height();
  controls_->setGeometry(0, height() - barHeight, width(), barHeight);
  center_->setGeometry(rect().adjusted(32, 32, -32, -32 - barHeight));
  osd_->move(16, 16);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  const core::Action action =
      core::actionForKey(toCoreKey(event->key()), toCoreModifiers(event->modifiers()));
  if (action == core::Action::None) {
    QWidget::keyPressEvent(event);
    return;
  }
  event->accept();
  if (event->isAutoRepeat() && !repeatable(action)) return;  // holding Space must not flicker
  controller_.trigger(action);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
  const QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty()) return;
  const std::optional<MediaSource> source = mediaSourceFromUrl(urls.first());
  if (!source) {
    showMessage(tr("Unsupported drop").toStdString());
    return;
  }
  event->acceptProposedAction();
  const QString ext = lowerSuffix(source->location);
  if (source->isLocalFile && engine_->state().loaded &&
      core::isManualSubtitleExtension(ext.toStdString())) {
    loadSubtitleFile(source->location);
    return;
  }
  openLocation(source->location);  // already decoded from the URL
  activateWindow();
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
  showControls();
  QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) controller_.toggleFullscreen();
}

void MainWindow::wheelEvent(QWheelEvent* event) {
  wheelAccumulator_ += event->angleDelta().y();
  while (wheelAccumulator_ >= 120) {
    controller_.trigger(core::Action::VolumeUp);
    wheelAccumulator_ -= 120;
  }
  while (wheelAccumulator_ <= -120) {
    controller_.trigger(core::Action::VolumeDown);
    wheelAccumulator_ += 120;
  }
  event->accept();
}

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);
  connect(menu.addAction(tr("Open…\tCtrl+O")), &QAction::triggered, this,
          &MainWindow::openFileDialog);
  QAction* loadSub = menu.addAction(tr("Load subtitle file…"));
  loadSub->setEnabled(engine_->state().loaded);
  connect(loadSub, &QAction::triggered, this, &MainWindow::openSubtitleDialog);
  menu.addSeparator();
  menu.addMenu(audioMenu_);
  menu.addMenu(subtitleMenu_);
  menu.addMenu(speedMenu_);
  menu.addSeparator();
  connect(menu.addAction(isFullScreen() ? tr("Exit fullscreen\tF") : tr("Fullscreen\tF")),
          &QAction::triggered, this, [this] { controller_.toggleFullscreen(); });
  connect(menu.addAction(tr("Quit\tCtrl+Q")), &QAction::triggered, this, &MainWindow::close);
  menu.exec(event->globalPos());
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  layoutOverlays();
}

void MainWindow::changeEvent(QEvent* event) {
  if (event->type() == QEvent::WindowStateChange) controls_->setFullscreen(isFullScreen());
  QWidget::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  saveResumePosition();
  QThreadPool::globalInstance()->waitForDone();
  event->accept();
}

}  // namespace chobi
