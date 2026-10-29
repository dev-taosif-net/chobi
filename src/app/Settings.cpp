// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/Settings.h"

#include <QMutexLocker>
#include <QSettings>

#include "core/PlaybackController.h"

namespace chobi {
namespace {

constexpr char kResume[] = "playback/resume";
constexpr char kRememberVolume[] = "audio/rememberVolume";
constexpr char kVolume[] = "audio/volume";

}  // namespace

double AppSettings::startupVolume() const { return rememberVolume ? core::clampVolume(volume) : 100.0; }

SettingsStore::SettingsStore(QString filePath) : path_(std::move(filePath)) {}

AppSettings SettingsStore::load() const {
  QMutexLocker lock(&mutex_);
  const QSettings ini(path_, QSettings::IniFormat);
  AppSettings s;
  s.resumePlayback = ini.value(kResume, s.resumePlayback).toBool();
  s.rememberVolume = ini.value(kRememberVolume, s.rememberVolume).toBool();
  bool ok = false;
  const double volume = ini.value(kVolume, s.volume).toDouble(&ok);
  if (ok) s.volume = core::clampVolume(volume);
  return s;
}

void SettingsStore::save(const AppSettings& s) {
  QMutexLocker lock(&mutex_);
  QSettings ini(path_, QSettings::IniFormat);
  ini.setValue(kResume, s.resumePlayback);
  ini.setValue(kRememberVolume, s.rememberVolume);
  ini.setValue(kVolume, core::clampVolume(s.volume));
  ini.sync();
}

}  // namespace chobi
