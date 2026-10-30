// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <clocale>
#include <cstdio>
#include <memory>
#include <string_view>

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QThreadPool>
#include <QTimer>

#include "app/Bench.h"
#include "app/MainWindow.h"
#include "app/MediaPath.h"
#include "app/ResumeService.h"
#include "app/Settings.h"
#include "app/SingleInstance.h"
#include "app/Updater.h"
#include "core/Language.h"
#include "engine/MpvEngine.h"
#include "engine/MpvVideoWidget.h"

namespace {

// Subtitle language preference: CHOBI_SUBTITLE_LANGS="bn,en" overrides the system UI
// languages; English is always the final fallback.
std::vector<std::string> preferredLanguages() {
  QStringList sources;
  const QString env = qEnvironmentVariable("CHOBI_SUBTITLE_LANGS");
  sources = env.isEmpty() ? QLocale::system().uiLanguages() : env.split(QLatin1Char(','));
  sources << QStringLiteral("en");

  std::vector<std::string> out;
  for (const QString& s : sources) {
    const std::string code = chobi::core::normalizeLanguage(s.trimmed().toStdString());
    if (!code.empty() && std::find(out.begin(), out.end(), code) == out.end()) out.push_back(code);
  }
  return out;
}

QString bundledFontsDir() {
  const QString appDir = QCoreApplication::applicationDirPath();
  for (const QString& candidate :
       {appDir + QStringLiteral("/fonts"), appDir + QStringLiteral("/../share/chobi/fonts")}) {
    if (QDir(candidate).exists()) return QDir::cleanPath(candidate);
  }
  return {};
}

void printUsage() {
  std::printf("Usage: chobi [--version] [--new-window] [FILE | URL]\n\n"
              "Chobi " CHOBI_VERSION " - a lightweight video player.\n"
              "A video opened while Chobi is running plays in that window; --new-window opens\n"
              "a separate one.\n"
              "Environment: CHOBI_SUBTITLE_LANGS=bn,en  CHOBI_MPV_LOG=1  CHOBI_BENCH=1\n");
}

}  // namespace

int main(int argc, char* argv[]) {
  chobi::bench::start();
  // Set by the launcher (file manager, desktop) on Wayland. Read it before Qt can use it.
  const QByteArray activationToken = qgetenv("XDG_ACTIVATION_TOKEN");

  // Answer --version/--help without starting a GUI (also used to smoke-test packages).
  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    if (a == "--") break;
    if (a == "--version" || a == "-v") {
      std::printf("Chobi %s\n", CHOBI_VERSION);
      return 0;
    }
    if (a == "--help" || a == "-h") {
      printUsage();
      return 0;
    }
  }

#if defined(Q_OS_LINUX)
  // VA-API zero-copy needs EGL. Wayland always uses EGL; make X11 do the same.
  if (qEnvironmentVariableIsEmpty("QT_XCB_GL_INTEGRATION")) {
    qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
  }
#endif
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setDepthBufferSize(0);
  format.setStencilBufferSize(0);
  format.setSwapInterval(1);
  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);
  chobi::bench::mark("qapp-created");
  QApplication::setApplicationName(QStringLiteral("Chobi"));
  QApplication::setApplicationVersion(QStringLiteral(CHOBI_VERSION));
  QApplication::setDesktopFileName(QStringLiteral("io.github.dev_taosif_net.Chobi"));
  // libmpv refuses to start unless numbers are formatted the C way.
  std::setlocale(LC_NUMERIC, "C");

  QString location;
  bool newWindow = false;
  const QStringList args = QApplication::arguments();
  bool endOfOptions = false;
  for (int i = 1; i < args.size(); ++i) {
    const QString& a = args.at(i);
    if (!endOfOptions && a == QLatin1String("--")) {
      endOfOptions = true;
    } else if (!endOfOptions && a == QLatin1String("--new-window")) {
      newWindow = true;
    } else if (!endOfOptions && a.startsWith(QLatin1Char('-'))) {
      std::fprintf(stderr, "chobi: ignoring unknown option %s\n", qPrintable(a));
    } else if (location.isEmpty()) {
      location = a;  // one file per window; extra arguments are ignored
    }
  }

  // Chobi already running: it plays the video (relative paths resolved here, where they were
  // typed) and comes to the front, and this process ends before starting an engine. Benchmarks
  // always measure a fresh process.
  const bool singleInstance = !newWindow && !chobi::bench::enabled();
  chobi::InstanceServer instance(chobi::defaultInstanceName());
  if (singleInstance) {
    const std::optional<chobi::MediaSource> source = chobi::mediaSourceFromArgument(location);
    if (chobi::forwardToRunningInstance(chobi::defaultInstanceName(),
                                        {source ? source->location : location, activationToken})) {
      return 0;
    }
    instance.listen();  // requests that arrive before the window exists wait for exec()
  }

  const std::vector<std::string> languages = preferredLanguages();
  // One small INI read at startup; every later write happens on the thread pool.
  auto settingsStore = std::make_shared<chobi::SettingsStore>(
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
      QStringLiteral("/settings.ini"));
  const chobi::AppSettings settings = settingsStore->load();

  chobi::MpvEngine::Options options;
  options.initialVolume = settings.startupVolume();
  options.fontsDir = bundledFontsDir();
  options.verboseLog = qEnvironmentVariableIntValue("CHOBI_MPV_LOG") == 1;
  for (const std::string& l : languages) options.preferredLanguages << QString::fromStdString(l);

  auto engine = std::make_unique<chobi::MpvEngine>(options);
  if (!engine->isValid()) {
    QMessageBox::critical(nullptr, QStringLiteral("Chobi"),
                          QStringLiteral("The playback engine could not start.\n\n") +
                              engine->initError());
    return 1;
  }
  chobi::bench::mark("engine-ready");

  const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  auto resume = std::make_shared<chobi::ResumeService>(dataDir + QStringLiteral("/resume.tsv"));
  // Drop saved positions of videos deleted since the last run (expired ones go on load).
  QThreadPool::globalInstance()->start([resume] { resume->pruneDeletedVideos(); });

  chobi::Updater updater(chobi::Updater::defaultConfig());

  int rc = 0;
  {
    auto* video = new chobi::MpvVideoWidget(engine.get());
    chobi::MainWindow window(engine.get(), video,
                             {resume, settingsStore, settings, languages, &updater,
                              instance.isListening() ? &instance : nullptr});

    if (chobi::bench::enabled()) {
      // Startup timeline for bench/run_bench.py.
      auto armed = std::make_shared<bool>(false);
      QObject::connect(engine.get(), &chobi::PlayerEngine::fileLoaded, &window,
                       [] { chobi::bench::mark("file-loaded"); });
      QObject::connect(engine.get(), &chobi::PlayerEngine::playbackRestarted, &window,
                       [armed] { *armed = true; });
      QObject::connect(video, &chobi::MpvVideoWidget::framePainted, &window, [armed, &engine] {
        if (!*armed) return;
        *armed = false;
        chobi::bench::mark("first-frame-painted");
        std::fprintf(stderr, "[bench] hwdec %s volume %.0f\n",
                     engine->state().hwdec.empty() ? "no" : engine->state().hwdec.c_str(),
                     engine->state().volume);
        if (chobi::bench::quitAfterFirstFrame()) QTimer::singleShot(0, qApp, &QApplication::quit);
      });
      QObject::connect(engine.get(), &chobi::PlayerEngine::fileFailed, &window,
                       [](const QString& m) {
                         std::fprintf(stderr, "[bench] error %s\n", qPrintable(m));
                         if (chobi::bench::quitAfterFirstFrame()) QApplication::exit(2);
                       });
    }

    window.show();
    chobi::bench::mark("window-shown");
    if (!location.isEmpty()) window.openLocation(location);
    rc = QApplication::exec();
  }  // the window (and mpv's render context) must go before the engine
  engine.reset();
  return rc;
}
