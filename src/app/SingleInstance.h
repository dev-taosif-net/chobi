// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>

#include <QByteArray>
#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

namespace chobi {

// What a second launch hands to the running Chobi: the video to play (absolute path or URL,
// empty to just bring the window forward) and the launcher's activation token, without which
// Wayland compositors won't let the running window come to the front.
struct OpenRequest {
  QString location;
  QByteArray activationToken;
};

// One line of JSON, newline-terminated.
QByteArray encodeOpenRequest(const OpenRequest& request);
std::optional<OpenRequest> decodeOpenRequest(const QByteArray& line);

// Per-user name of the local socket (Unix: in $XDG_RUNTIME_DIR) or named pipe (Windows).
QString defaultInstanceName();

// Client side: hands `request` to the Chobi listening on `name`. True once that Chobi has
// accepted it; false at once when none is running, or after `timeoutMs` if it doesn't answer.
bool forwardToRunningInstance(const QString& name, const OpenRequest& request,
                              int timeoutMs = 3000);

// Server side, owned by the Chobi that has the window. Requests arrive on the UI thread.
class InstanceServer final : public QObject {
  Q_OBJECT

 public:
  explicit InstanceServer(QString name, QObject* parent = nullptr);
  ~InstanceServer() override;

  // False when another Chobi already answers on the name. A socket file left behind by a
  // crashed Chobi is replaced.
  bool listen();
  void close();  // e.g. before restarting into an update, so the new process takes over
  bool isListening() const;

 signals:
  void openRequested(const OpenRequest& request);

 private:
  void onNewConnection();
  void onReadyRead(QLocalSocket* socket);

  QString name_;
  QLocalServer* server_;
};

}  // namespace chobi
