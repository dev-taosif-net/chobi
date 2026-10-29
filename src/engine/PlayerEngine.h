// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>

#include "core/IPlayerEngine.h"

namespace chobi {

// Qt-facing engine abstraction: the command interface from core plus change notifications.
// The UI depends only on this class; MpvEngine is one implementation, tests use a fake.
class PlayerEngine : public QObject, public core::IPlayerEngine {
  Q_OBJECT

 public:
  using QObject::QObject;

 signals:
  void fileStarted();
  void fileLoaded();
  void fileFailed(const QString& message);
  void playbackRestarted();  // first frame after a load or seek is ready
  void positionChanged(double seconds);
  void durationChanged(double seconds);
  void pausedChanged(bool paused);
  void volumeChanged(double volume);
  void mutedChanged(bool muted);
  void speedChanged(double speed);
  void subtitlesVisibleChanged(bool visible);
  void eofReachedChanged(bool eof);
  void bufferingChanged(bool buffering);
  void tracksChanged();
  void mediaTitleChanged(const QString& title);
  void hwdecChanged(const QString& hwdec);
  void warningRaised(const QString& message);
};

}  // namespace chobi
