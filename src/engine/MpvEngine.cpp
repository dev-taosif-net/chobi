// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/MpvEngine.h"

#include <mpv/client.h>

#if MPV_CLIENT_API_VERSION < MPV_MAKE_VERSION(2, 3)
#error "Chobi needs libmpv from mpv 0.38 or newer (client API 2.3)"
#endif

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include <QByteArray>
#include <QMetaObject>

namespace chobi {
namespace {

enum PropertyId : std::uint64_t {
  kTimePos = 1,
  kDuration,
  kPause,
  kVolume,
  kMute,
  kSpeed,
  kTrackList,
  kSubVisibility,
  kEofReached,
  kPausedForCache,
  kMediaTitle,
  kHwdecCurrent,
};

enum ReplyId : std::uint64_t { kReplyNone = 0, kReplySubAdd = 100 };
constexpr std::uint64_t kReplyLoadBase = std::uint64_t{1} << 32;  // + open() serial

const char* hwdecOption(bool headless) {
  if (headless) return "no";
#if defined(Q_OS_WIN)
  // The OpenGL render API has no zero-copy D3D11 interop without ANGLE, so copy back.
  // A D3D11 render-API surface (mpv >= 0.42) will lift this; see docs/adr/0001.
  return "d3d11va-copy,auto-safe";
#else
  // mpv 0.41 prefers Vulkan hwdec, which the OpenGL render API cannot display.
  return "vaapi,auto-safe";
#endif
}

QString describeLoadError(int error, const QString& detail) {
  QString message;
  switch (error) {
    case MPV_ERROR_UNKNOWN_FORMAT:
      message = QStringLiteral("Unsupported or unrecognized file format.");
      break;
    case MPV_ERROR_LOADING_FAILED:
      message = QStringLiteral("The file could not be opened. It may be missing, unreadable, "
                               "incomplete or corrupt.");
      break;
    case MPV_ERROR_NOTHING_TO_PLAY:
      message = QStringLiteral("No playable audio or video stream was found. The codec may not "
                               "be supported by this build.");
      break;
    case MPV_ERROR_VO_INIT_FAILED:
      message = QStringLiteral("Video output could not be initialized.");
      break;
    case MPV_ERROR_AO_INIT_FAILED:
      message = QStringLiteral("Audio output could not be initialized.");
      break;
    default:
      message = QString::fromUtf8(mpv_error_string(error));
      break;
  }
  if (!detail.isEmpty()) message += QStringLiteral("\n(") + detail + QLatin1Char(')');
  return message;
}

QString nodeString(const mpv_node& n) {
  return n.format == MPV_FORMAT_STRING ? QString::fromUtf8(n.u.string) : QString();
}

std::vector<core::Track> parseTrackList(const mpv_node& root) {
  std::vector<core::Track> out;
  if (root.format != MPV_FORMAT_NODE_ARRAY) return out;
  for (int i = 0; i < root.u.list->num; ++i) {
    const mpv_node& item = root.u.list->values[i];
    if (item.format != MPV_FORMAT_NODE_MAP) continue;
    core::Track t;
    bool typeKnown = false;
    for (int j = 0; j < item.u.list->num; ++j) {
      const QByteArray key(item.u.list->keys[j]);
      const mpv_node& v = item.u.list->values[j];
      const bool flag = v.format == MPV_FORMAT_FLAG && v.u.flag != 0;
      if (key == "id" && v.format == MPV_FORMAT_INT64) {
        t.id = v.u.int64;
      } else if (key == "type") {
        const QString type = nodeString(v);
        typeKnown = true;
        if (type == QLatin1String("audio")) {
          t.type = core::TrackType::Audio;
        } else if (type == QLatin1String("sub")) {
          t.type = core::TrackType::Subtitle;
        } else if (type == QLatin1String("video")) {
          t.type = core::TrackType::Video;
        } else {
          typeKnown = false;
        }
      } else if (key == "title") {
        t.title = nodeString(v).toStdString();
      } else if (key == "lang") {
        t.language = nodeString(v).toStdString();
      } else if (key == "codec") {
        t.codec = nodeString(v).toStdString();
      } else if (key == "demux-channel-count" && v.format == MPV_FORMAT_INT64) {
        t.channels = static_cast<int>(v.u.int64);
      } else if (key == "external-filename") {
        t.externalFile = nodeString(v).toStdString();
      } else if (key == "default") {
        t.isDefault = flag;
      } else if (key == "forced") {
        t.forced = flag;
      } else if (key == "external") {
        t.external = flag;
      } else if (key == "selected") {
        t.selected = flag;
      } else if (key == "hearing-impaired") {
        t.hearingImpaired = flag;
      }
    }
    if (typeKnown && t.id > 0) out.push_back(std::move(t));
  }
  return out;
}

}  // namespace

MpvEngine::MpvEngine(const Options& options, QObject* parent)
    : PlayerEngine(parent), verboseLog_(options.verboseLog) {
  mpv_ = mpv_create();
  if (!mpv_) {
    initError_ = QStringLiteral("mpv_create() failed (is LC_NUMERIC set to \"C\"?)");
    return;
  }

  auto set = [this](const char* name, const QByteArray& value) {
    const int rc = mpv_set_option_string(mpv_, name, value.constData());
    if (rc < 0 && verboseLog_) {
      std::fprintf(stderr, "[chobi] option %s=%s: %s\n", name, value.constData(),
                   mpv_error_string(rc));
    }
  };

  set("config", "no");
  set("vo", options.headless ? "null" : "libmpv");
  if (options.headless) set("ao", "null");
  set("hwdec", hwdecOption(options.headless));
  set("idle", "yes");
  set("keep-open", "yes");
  set("volume-max", "100");
  // We discover subtitles ourselves (strict matching, off the UI thread) and never scan for
  // external audio or cover art.
  set("sub-auto", "no");
  set("audio-file-auto", "no");
  set("cover-art-auto", "no");
  set("sub-codepage", "auto");
  // No input handling, scripts, OSC or youtube-dl helper processes inside the engine.
  set("input-default-bindings", "no");
  set("input-vo-keyboard", "no");
  set("osc", "no");
  set("load-scripts", "no");
  set("ytdl", "no");
  set("terminal", "no");
  // Bounded demuxer cache for local files and streams.
  set("demuxer-max-bytes", "64MiB");
  set("demuxer-max-back-bytes", "16MiB");
  set("audio-client-name", "Chobi");
  // The GUI thread renders with BLOCK_FOR_TARGET_TIME=0 so it never sleeps inside mpv;
  // mpv's docs pair that with a zero timing offset to keep A/V sync exact.
  if (!options.headless) set("video-timing-offset", "0");
  if (!options.preferredLanguages.isEmpty()) {
    set("slang", options.preferredLanguages.join(QLatin1Char(',')).toUtf8());
  }
  if (!options.fontsDir.isEmpty()) set("sub-fonts-dir", options.fontsDir.toUtf8());

  // Must happen before the render context exists: not render-thread safe.
  mpv_request_log_messages(mpv_, options.verboseLog ? "v" : "warn");

  mpv_observe_property(mpv_, kTimePos, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv_, kDuration, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv_, kPause, "pause", MPV_FORMAT_FLAG);
  mpv_observe_property(mpv_, kVolume, "volume", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv_, kMute, "mute", MPV_FORMAT_FLAG);
  mpv_observe_property(mpv_, kSpeed, "speed", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv_, kTrackList, "track-list", MPV_FORMAT_NODE);
  mpv_observe_property(mpv_, kSubVisibility, "sub-visibility", MPV_FORMAT_FLAG);
  mpv_observe_property(mpv_, kEofReached, "eof-reached", MPV_FORMAT_FLAG);
  mpv_observe_property(mpv_, kPausedForCache, "paused-for-cache", MPV_FORMAT_FLAG);
  mpv_observe_property(mpv_, kMediaTitle, "media-title", MPV_FORMAT_STRING);
  mpv_observe_property(mpv_, kHwdecCurrent, "hwdec-current", MPV_FORMAT_STRING);

  mpv_set_wakeup_callback(
      mpv_,
      [](void* ctx) {
        // Called from an mpv thread: coalesce into one queued call on the engine's thread.
        auto* self = static_cast<MpvEngine*>(ctx);
        if (!self->wakeupPending_.exchange(true)) {
          QMetaObject::invokeMethod(self, [self] { self->processEvents(); },
                                    Qt::QueuedConnection);
        }
      },
      this);

  if (const int rc = mpv_initialize(mpv_); rc < 0) {
    initError_ = QStringLiteral("mpv_initialize() failed: ") + QString::fromUtf8(mpv_error_string(rc));
    mpv_set_wakeup_callback(mpv_, nullptr, nullptr);
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
  }
}

MpvEngine::~MpvEngine() {
  if (!mpv_) return;
  mpv_set_wakeup_callback(mpv_, nullptr, nullptr);
  mpv_terminate_destroy(mpv_);
  mpv_ = nullptr;
}

void MpvEngine::processEvents() {
  wakeupPending_.store(false);
  while (mpv_) {
    const mpv_event* event = mpv_wait_event(mpv_, 0);
    if (event->event_id == MPV_EVENT_NONE) break;
    handleEvent(*event);
  }
}

void MpvEngine::handleEvent(const mpv_event& event) {
  switch (event.event_id) {
    case MPV_EVENT_PROPERTY_CHANGE:
      handleProperty(event.reply_userdata, *static_cast<const mpv_event_property*>(event.data));
      break;
    case MPV_EVENT_START_FILE:
      currentEntry_ = static_cast<const mpv_event_start_file*>(event.data)->playlist_entry_id;
      // Entries older than the one starting now were replaced before they ever played.
      pendingByEntry_.erase(pendingByEntry_.begin(), pendingByEntry_.lower_bound(currentEntry_));
      state_.loaded = false;
      lastLogError_.clear();
      emit fileStarted();
      break;
    case MPV_EVENT_FILE_LOADED:
      state_.loaded = true;
      attachSubtitles(currentEntry_);
      emit fileLoaded();
      break;
    case MPV_EVENT_PLAYBACK_RESTART:
      emit playbackRestarted();
      break;
    case MPV_EVENT_END_FILE: {
      const auto* end = static_cast<const mpv_event_end_file*>(event.data);
      state_.loaded = false;
      if (end->reason == MPV_END_FILE_REASON_ERROR) {
        emit fileFailed(describeLoadError(end->error, lastLogError_));
      }
      break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
      const auto* msg = static_cast<const mpv_event_log_message*>(event.data);
      if (verboseLog_) std::fprintf(stderr, "[mpv/%s] %s: %s", msg->prefix, msg->level, msg->text);
      const QString text = QString::fromUtf8(msg->text).trimmed();
      if (msg->log_level <= MPV_LOG_LEVEL_ERROR && !text.isEmpty()) lastLogError_ = text;
      if (text.startsWith(QLatin1String("Failed to initialize a decoder")) ||
          text.startsWith(QLatin1String("Could not open codec"))) {
        emit warningRaised(QStringLiteral("A stream uses a codec this build cannot decode: ") +
                           text);
      }
      break;
    }
    case MPV_EVENT_COMMAND_REPLY:
      if (event.reply_userdata >= kReplyLoadBase) {
        onLoadReply(event.reply_userdata - kReplyLoadBase, event.error,
                    static_cast<const mpv_event_command*>(event.data)->result);
      } else if (event.error < 0 && event.reply_userdata == kReplySubAdd) {
        emit warningRaised(QStringLiteral("Could not load the subtitle file."));
      }
      break;
    default:
      break;
  }
}

void MpvEngine::handleProperty(std::uint64_t id, const mpv_event_property& p) {
  const bool hasDouble = p.format == MPV_FORMAT_DOUBLE;
  const double d = hasDouble ? *static_cast<const double*>(p.data) : 0.0;
  const bool flag = p.format == MPV_FORMAT_FLAG && *static_cast<const int*>(p.data) != 0;
  const QString str = p.format == MPV_FORMAT_STRING
                          ? QString::fromUtf8(*static_cast<char* const*>(p.data))
                          : QString();

  switch (id) {
    case kTimePos:
      if (d != state_.position) {  // unavailable (no file) reads as 0
        state_.position = d;
        emit positionChanged(d);
      }
      break;
    case kDuration:
      if (d != state_.duration) {
        state_.duration = d;
        emit durationChanged(d);
      }
      break;
    case kPause:
      if (flag != state_.paused) {
        state_.paused = flag;
        emit pausedChanged(flag);
      }
      break;
    case kVolume:
      if (hasDouble && d != state_.volume) {
        state_.volume = d;
        emit volumeChanged(d);
      }
      break;
    case kMute:
      if (flag != state_.muted) {
        state_.muted = flag;
        emit mutedChanged(flag);
      }
      break;
    case kSpeed:
      if (hasDouble && d != state_.speed) {
        state_.speed = d;
        emit speedChanged(d);
      }
      break;
    case kTrackList:
      tracks_ = p.format == MPV_FORMAT_NODE
                    ? parseTrackList(*static_cast<const mpv_node*>(p.data))
                    : std::vector<core::Track>();
      emit tracksChanged();
      break;
    case kSubVisibility:
      if (flag != state_.subtitlesVisible) {
        state_.subtitlesVisible = flag;
        emit subtitlesVisibleChanged(flag);
      }
      break;
    case kEofReached:
      if (flag != state_.eofReached) {
        state_.eofReached = flag;
        emit eofReachedChanged(flag);
      }
      break;
    case kPausedForCache:
      if (flag != state_.buffering) {
        state_.buffering = flag;
        emit bufferingChanged(flag);
      }
      break;
    case kMediaTitle:
      state_.mediaTitle = str.toStdString();
      emit mediaTitleChanged(str);
      break;
    case kHwdecCurrent: {
      const QString hw = str == QLatin1String("no") ? QString() : str;
      state_.hwdec = hw.toStdString();
      emit hwdecChanged(hw);
      break;
    }
    default:
      break;
  }
}

void MpvEngine::command(std::initializer_list<const char*> args, std::uint64_t replyId) {
  if (!mpv_) return;
  std::vector<const char*> argv(args);
  argv.push_back(nullptr);
  mpv_command_async(mpv_, replyId, argv.data());
}

void MpvEngine::setFlag(const char* name, bool value) {
  if (!mpv_) return;
  int v = value ? 1 : 0;
  mpv_set_property_async(mpv_, kReplyNone, name, MPV_FORMAT_FLAG, &v);
}

void MpvEngine::setDouble(const char* name, double value) {
  if (!mpv_) return;
  mpv_set_property_async(mpv_, kReplyNone, name, MPV_FORMAT_DOUBLE, &value);
}

void MpvEngine::setString(const char* name, const char* value) {
  if (!mpv_) return;
  mpv_set_property_async(mpv_, kReplyNone, name, MPV_FORMAT_STRING, &value);
}

void MpvEngine::open(const std::string& pathOrUrl, double startSeconds,
                     std::vector<core::SubtitleFile> subtitles) {
  if (!mpv_) return;
  const std::uint64_t serial = ++openSerial_;
  // Older opens that mpv hasn't acknowledged yet are superseded by this one.
  pendingBySerial_.clear();
  if (!subtitles.empty()) pendingBySerial_[serial] = std::move(subtitles);

  // Named-argument form: no escaping of the path or the per-file options is needed.
  const QByteArray start = QByteArray::number(startSeconds, 'f', 3);
  const bool withStart = startSeconds > 0 && std::isfinite(startSeconds);

  char* optionKeys[] = {const_cast<char*>("start")};
  mpv_node optionValues[1];
  optionValues[0].format = MPV_FORMAT_STRING;
  optionValues[0].u.string = const_cast<char*>(start.constData());
  mpv_node_list optionList{withStart ? 1 : 0, optionValues, optionKeys};

  char* keys[] = {const_cast<char*>("name"), const_cast<char*>("url"), const_cast<char*>("flags"),
                  const_cast<char*>("index"), const_cast<char*>("options")};
  mpv_node values[5];
  values[0].format = MPV_FORMAT_STRING;
  values[0].u.string = const_cast<char*>("loadfile");
  values[1].format = MPV_FORMAT_STRING;
  values[1].u.string = const_cast<char*>(pathOrUrl.c_str());
  values[2].format = MPV_FORMAT_STRING;
  values[2].u.string = const_cast<char*>("replace");
  values[3].format = MPV_FORMAT_INT64;
  values[3].u.int64 = -1;
  values[4].format = MPV_FORMAT_NODE_MAP;
  values[4].u.list = &optionList;
  mpv_node_list args{5, values, keys};
  mpv_node cmd;
  cmd.format = MPV_FORMAT_NODE_MAP;
  cmd.u.list = &args;
  mpv_command_node_async(mpv_, kReplyLoadBase + serial, &cmd);  // copies everything

  if (state_.eofReached) {
    state_.eofReached = false;
    emit eofReachedChanged(false);
  }
}

void MpvEngine::onLoadReply(std::uint64_t serial, int error, const mpv_node& result) {
  auto it = pendingBySerial_.find(serial);
  if (it == pendingBySerial_.end()) return;
  std::vector<core::SubtitleFile> subs = std::move(it->second);
  pendingBySerial_.erase(it);
  if (error < 0 || result.format != MPV_FORMAT_NODE_MAP) return;

  for (int i = 0; i < result.u.list->num; ++i) {
    if (std::strcmp(result.u.list->keys[i], "playlist_entry_id") != 0) continue;
    const mpv_node& v = result.u.list->values[i];
    if (v.format != MPV_FORMAT_INT64) return;
    pendingByEntry_[v.u.int64] = std::move(subs);
    // Normally the reply precedes start-file; handle the reverse order too.
    if (v.u.int64 == currentEntry_ && state_.loaded) attachSubtitles(currentEntry_);
    return;
  }
}

void MpvEngine::attachSubtitles(std::int64_t entryId) {
  auto it = pendingByEntry_.find(entryId);
  if (it == pendingByEntry_.end()) return;
  for (const core::SubtitleFile& s : it->second) {
    addSubtitleFile(s.path, s.select, s.title, s.language);
  }
  pendingByEntry_.erase(it);
}

void MpvEngine::stop() { command({"stop"}); }

void MpvEngine::setPaused(bool paused) {
  if (paused != state_.paused) {
    state_.paused = paused;
    emit pausedChanged(paused);
  }
  setFlag("pause", paused);
}

void MpvEngine::seekAbsolute(double seconds, bool exact) {
  const QByteArray target = QByteArray::number(seconds, 'f', 3);
  command({"seek", target.constData(), exact ? "absolute+exact" : "absolute+keyframes"});
  state_.position = seconds;
  emit positionChanged(seconds);
}

void MpvEngine::setVolume(double volume) {
  if (volume != state_.volume) {
    state_.volume = volume;
    emit volumeChanged(volume);
  }
  setDouble("volume", volume);
}

void MpvEngine::setMuted(bool muted) {
  if (muted != state_.muted) {
    state_.muted = muted;
    emit mutedChanged(muted);
  }
  setFlag("mute", muted);
}

void MpvEngine::setSpeed(double speed) { setDouble("speed", speed); }

void MpvEngine::selectAudioTrack(std::int64_t id) {
  const QByteArray value = QByteArray::number(static_cast<qlonglong>(id));
  setString("aid", value.constData());
  for (core::Track& t : tracks_) {
    if (t.type == core::TrackType::Audio) t.selected = t.id == id;
  }
  emit tracksChanged();
}

void MpvEngine::selectSubtitleTrack(std::int64_t id) {
  const QByteArray value = id > 0 ? QByteArray::number(static_cast<qlonglong>(id)) : "no";
  setString("sid", value.constData());
  for (core::Track& t : tracks_) {
    if (t.type == core::TrackType::Subtitle) t.selected = t.id == id;
  }
  emit tracksChanged();
}

void MpvEngine::setSubtitlesVisible(bool visible) {
  if (visible != state_.subtitlesVisible) {
    state_.subtitlesVisible = visible;
    emit subtitlesVisibleChanged(visible);
  }
  setFlag("sub-visibility", visible);
}

void MpvEngine::addSubtitleFile(const std::string& path, bool select, const std::string& title,
                                const std::string& language) {
  const char* flags = select ? "select" : "auto";
  if (language.empty()) {
    command({"sub-add", path.c_str(), flags, title.c_str()}, kReplySubAdd);
  } else {
    command({"sub-add", path.c_str(), flags, title.c_str(), language.c_str()}, kReplySubAdd);
  }
}

}  // namespace chobi
