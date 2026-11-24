// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

// The probe starts by itself when QT_GUI_TEST_PROBE is set before the application is constructed.

#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTest>

namespace {
QString socketName()
{
    return QDir::tempPath() + QStringLiteral("/qgp-start-%1").arg(QCoreApplication::applicationPid());
}
}

class TestStartup : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void probeListensOnceTheEventLoopRuns()
    {
        QLocalSocket socket;
        QVERIFY(QTest::qWaitFor(
            [&socket] {
                socket.connectToServer(socketName());
                return socket.waitForConnected(100);
            },
            5000));
        socket.write(R"({"id": 1, "method": "hello"})"
                     "\n");
        QVERIFY(QTest::qWaitFor([&socket] { return socket.canReadLine(); }, 5000));
        const QJsonObject response = QJsonDocument::fromJson(socket.readLine()).object();
        QCOMPARE(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("pid")).toInteger(), QCoreApplication::applicationPid());
    }
};

int main(int argc, char **argv)
{
    // applicationPid() is the process id, known before the application exists
    qputenv("QT_GUI_TEST_PROBE", socketName().toLocal8Bit());
    QApplication app(argc, argv);
    TestStartup test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_startup.moc"
