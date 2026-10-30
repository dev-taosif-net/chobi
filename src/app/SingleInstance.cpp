// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/SingleInstance.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTimer>

#include <utility>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace chobi {
namespace {

constexpr qint64 kMaxRequestBytes = 64 * 1024;
constexpr int kClientTimeoutMs = 5000;  // drop connections that never send a full request
constexpr int kProbeTimeoutMs = 300;
const QByteArray kAccepted = QByteArrayLiteral("ok\n");

}  // namespace

QByteArray encodeOpenRequest(const OpenRequest& request) {
  QJsonObject o;
  o[QStringLiteral("open")] = request.location;
  o[QStringLiteral("token")] = QString::fromUtf8(request.activationToken);
  return QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n';
}

std::optional<OpenRequest> decodeOpenRequest(const QByteArray& line) {
  QJsonParseError error{};
  const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed(), &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) return std::nullopt;
  const QJsonObject o = doc.object();
  return OpenRequest{o.value(QStringLiteral("open")).toString(),
                     o.value(QStringLiteral("token")).toString().toUtf8()};
}

QString defaultInstanceName() {
#ifdef Q_OS_WIN
  return QStringLiteral("io.github.dev_taosif_net.Chobi-") + qEnvironmentVariable("USERNAME");
#else
  // $XDG_RUNTIME_DIR is private to the user (Qt falls back to a 0700 /tmp/runtime-$USER).
  return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
         QStringLiteral("/chobi-instance");
#endif
}

bool forwardToRunningInstance(const QString& name, const OpenRequest& request, int timeoutMs) {
  QLocalSocket socket;
  socket.connectToServer(name);
  if (!socket.waitForConnected(timeoutMs)) return false;
#ifdef Q_OS_WIN
  // We were started by the user (Explorer, "Open with"), so we may bring a window forward;
  // pass that right on to the running Chobi.
  AllowSetForegroundWindow(ASFW_ANY);
#endif
  socket.write(encodeOpenRequest(request));
  if (!socket.waitForBytesWritten(timeoutMs)) return false;
  while (!socket.canReadLine()) {
    if (!socket.waitForReadyRead(timeoutMs)) return false;
  }
  return socket.readLine() == kAccepted;
}

InstanceServer::InstanceServer(QString name, QObject* parent)
    : QObject(parent), name_(std::move(name)), server_(new QLocalServer(this)) {
  server_->setSocketOptions(QLocalServer::UserAccessOption);
  connect(server_, &QLocalServer::newConnection, this, &InstanceServer::onNewConnection);
}

InstanceServer::~InstanceServer() { close(); }

bool InstanceServer::listen() {
  if (server_->isListening()) return true;
  // Ask first: Qt's listen() would silently replace a live Chobi's socket (with access
  // options it binds elsewhere and renames into place), and Windows lets pipe names be
  // shared. Nobody answering means any socket file there was left by a crash.
  QLocalSocket probe;
  probe.connectToServer(name_);
  if (probe.waitForConnected(kProbeTimeoutMs)) return false;
  QLocalServer::removeServer(name_);
  return server_->listen(name_);
}

void InstanceServer::close() {
  if (server_->isListening()) server_->close();  // also removes the Unix socket file
}

bool InstanceServer::isListening() const { return server_->isListening(); }

void InstanceServer::onNewConnection() {
  while (QLocalSocket* socket = server_->nextPendingConnection()) {
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    connect(socket, &QLocalSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
    QTimer::singleShot(kClientTimeoutMs, socket, [socket] { socket->abort(); });
    if (socket->canReadLine()) onReadyRead(socket);
  }
}

void InstanceServer::onReadyRead(QLocalSocket* socket) {
  if (!socket->canReadLine()) {
    if (socket->bytesAvailable() > kMaxRequestBytes) socket->abort();
    return;
  }
  const std::optional<OpenRequest> request = decodeOpenRequest(socket->readLine(kMaxRequestBytes));
  if (!request) {
    socket->abort();
    return;
  }
  socket->write(kAccepted);
  socket->flush();
  socket->disconnectFromServer();  // after the reply is written
  emit openRequested(*request);
}

}  // namespace chobi
