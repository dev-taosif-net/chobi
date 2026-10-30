// SPDX-License-Identifier: GPL-3.0-or-later
// The hand-off between a second launch and the running Chobi: request encoding, a real local
// socket round trip, no-server and stale-socket cases, and a second server on the same name.
#include <QtTest>

#include <QElapsedTimer>
#include <QFile>
#include <QLocalSocket>
#include <QTemporaryDir>

#include <atomic>
#include <thread>

#include "app/SingleInstance.h"

using chobi::InstanceServer;
using chobi::OpenRequest;

Q_DECLARE_METATYPE(chobi::OpenRequest)

class SingleInstanceTest : public QObject {
  Q_OBJECT

  QTemporaryDir dir;
  int counter = 0;

  // A fresh name per test. Unix sockets are paths; Windows pipe names must not contain one.
  QString uniqueName() {
#ifdef Q_OS_WIN
    return QStringLiteral("chobi-test-%1-%2").arg(QCoreApplication::applicationPid()).arg(++counter);
#else
    return dir.filePath(QStringLiteral("instance-%1").arg(++counter));
#endif
  }

  // forward() blocks, and the server needs this thread's event loop, so it runs on its own thread.
  static bool forwardOnThread(const QString& name, const OpenRequest& request) {
    std::atomic<bool> done{false};
    bool accepted = false;
    std::thread client([&] {
      accepted = chobi::forwardToRunningInstance(name, request, 3000);
      done = true;
    });
    QElapsedTimer timer;
    timer.start();
    while (!done && timer.elapsed() < 5000) QTest::qWait(5);
    client.join();
    return accepted;
  }

 private slots:
  void requestRoundTrip() {
    const OpenRequest in{QString::fromUtf8(u8"/home/me/ছবি ✓ 100% \"Movie\".mkv"),
                         QByteArrayLiteral("gnome-shell/Files/1234-5-host_TIME42")};
    const QByteArray line = chobi::encodeOpenRequest(in);
    QVERIFY(line.endsWith('\n'));
    QCOMPARE(line.count('\n'), 1);  // one line, whatever the path contains
    const std::optional<OpenRequest> out = chobi::decodeOpenRequest(line);
    QVERIFY(out);
    QCOMPARE(out->location, in.location);
    QCOMPARE(out->activationToken, in.activationToken);
  }

  void garbageIsRejected() {
    QVERIFY(!chobi::decodeOpenRequest("not json\n"));
    QVERIFY(!chobi::decodeOpenRequest("[1,2]\n"));
  }

  void noServerFailsFast() {
    QElapsedTimer timer;
    timer.start();
    QVERIFY(!chobi::forwardToRunningInstance(uniqueName(), {}, 3000));
    QVERIFY(timer.elapsed() < 1000);  // the first launch must not wait for a timeout
  }

  void requestReachesTheRunningInstance() {
    const QString name = uniqueName();
    InstanceServer server(name);
    QVERIFY(server.listen());
    QSignalSpy received(&server, &InstanceServer::openRequested);

    QVERIFY(forwardOnThread(name, {QStringLiteral("/videos/b.mkv"), QByteArrayLiteral("tok")}));
    QTRY_COMPARE(received.size(), 1);
    const auto request = received.at(0).at(0).value<OpenRequest>();
    QCOMPARE(request.location, QStringLiteral("/videos/b.mkv"));
    QCOMPARE(request.activationToken, QByteArrayLiteral("tok"));

    QVERIFY(forwardOnThread(name, {}));  // plain relaunch: just come to the front
    QTRY_COMPARE(received.size(), 2);
    QVERIFY(received.at(1).at(0).value<OpenRequest>().location.isEmpty());
  }

  void secondServerDoesNotTakeOver() {
    const QString name = uniqueName();
    InstanceServer first(name);
    QVERIFY(first.listen());
    InstanceServer second(name);
    QVERIFY(!second.listen());
    QSignalSpy received(&first, &InstanceServer::openRequested);
    QVERIFY(forwardOnThread(name, {QStringLiteral("/v.mp4"), {}}));
    QTRY_COMPARE(received.size(), 1);
  }

  void closedServerStopsAccepting() {
    const QString name = uniqueName();
    InstanceServer server(name);
    QVERIFY(server.listen());
    server.close();  // e.g. restarting into an update
    QVERIFY(!chobi::forwardToRunningInstance(name, {}, 1000));
    QVERIFY(server.listen());  // and can take over again
  }

#ifndef Q_OS_WIN
  void staleSocketFromACrashIsReplaced() {
    const QString name = uniqueName();
    QFile leftover(name);
    QVERIFY(leftover.open(QIODevice::WriteOnly));
    leftover.close();
    QVERIFY(!chobi::forwardToRunningInstance(name, {}, 1000));
    InstanceServer server(name);
    QVERIFY(server.listen());
    QSignalSpy received(&server, &InstanceServer::openRequested);
    QVERIFY(forwardOnThread(name, {QStringLiteral("/v.mp4"), {}}));
    QTRY_COMPARE(received.size(), 1);
  }
#endif

  void silentClientIsDropped() {
    // A connection that never sends a request (e.g. another Chobi probing) must not break
    // the next real one.
    const QString name = uniqueName();
    InstanceServer server(name);
    QVERIFY(server.listen());
    {
      QLocalSocket probe;
      probe.connectToServer(name);
      QVERIFY(probe.waitForConnected(1000));
    }
    QSignalSpy received(&server, &InstanceServer::openRequested);
    QVERIFY(forwardOnThread(name, {QStringLiteral("/v.mp4"), {}}));
    QTRY_COMPARE(received.size(), 1);
  }
};

QTEST_MAIN(SingleInstanceTest)
#include "test_single_instance.moc"
