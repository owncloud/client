/*
 * Copyright (C) by Thomas Müller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include <QtTest>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <chrono>
using namespace std::chrono_literals;

#include "socketapi/socketapi.h"
#include "folderman.h"
#include "accountmanager.h"

#include "testutils/syncenginetestutils.h"
#include "testutils/testutils.h"

using namespace OCC;

// A minimal fake reply that emits a Depth:0 PROPFIND multistatus response
// with status 207 and the correct content-type, suitable for private link lookups.
class FakeMultistatReply : public FakeReply
{
    Q_OBJECT
public:
    QByteArray _body;

    FakeMultistatReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
                       const QByteArray &body, QObject *parent)
        : FakeReply(parent), _body(body)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        open(QIODevice::ReadOnly);
        QTimer::singleShot(10ms, this, &FakeMultistatReply::respond);
    }

    void respond()
    {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 207);
        setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/xml; charset=utf-8"));
        setHeader(QNetworkRequest::ContentLengthHeader, _body.size());
        Q_EMIT metaDataChanged();
        Q_EMIT readyRead();
        checkedFinished();
    }

    qint64 readData(char *buf, qint64 max) override
    {
        max = qMin<qint64>(max, _body.size());
        std::copy(_body.cbegin(), _body.cbegin() + max, buf);
        _body.remove(0, static_cast<int>(max));
        return max;
    }

    qint64 bytesAvailable() const override { return _body.size(); }
};

class TestSocketApi : public QObject
{
    Q_OBJECT

    // Path of the folder registered during a test (reset by cleanup()).
    QString _registeredFolderPath;

    // Compute the socket path the same way guiutility_unix.cpp does.
    // Utility::socketApiSocketPath() provides the same logic, but it lives in the
    // GUI library and is not exposed as a public method of SocketApi, so the
    // computation is duplicated here rather than introducing an extra dependency.
    static QString socketApiPath()
    {
        return QStringLiteral("%1/ownCloud/socket")
            .arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));
    }

    // Connect to the SocketApi's server and wait for the connection to be accepted
    // (slotNewConnection fired). Returns the connected socket or nullptr on failure.
    static QLocalSocket *connectAndWaitForListener()
    {
        const QString path = socketApiPath();
        auto *sock = new QLocalSocket;
        sock->connectToServer(path);
        if (!sock->waitForConnected(3000)) {
            qWarning() << "Failed to connect to socket" << path
                       << "error:" << sock->errorString();
            delete sock;
            return nullptr;
        }
        // Give the server side a chance to run slotNewConnection and
        // insert the listener into _listeners.
        QCoreApplication::processEvents();
        return sock;
    }

private Q_SLOTS:
    void initTestCase()
    {
        // Ensure FolderMan singleton exists before any test
        TestUtils::folderMan();
    }

    void cleanup()
    {
        // Remove any folder registered during the test to avoid polluting later tests.
        if (!_registeredFolderPath.isEmpty()) {
            Folder *folder = TestUtils::folderMan()->folderForPath(_registeredFolderPath);
            if (folder) {
                TestUtils::folderMan()->removeFolderFromGui(folder);
            }
            _registeredFolderPath.clear();
        }
    }

    void testGetPrivateLinkSendsUrlOverSocket()
    {
        // ----- Set up a paused sync folder so no sync fires -----

        // Create a real directory on disk as the local sync root
        const QTemporaryDir tempDir = TestUtils::createTempDir();
        QVERIFY(tempDir.isValid());
        // Ensure trailing slash (FolderDefinition normalises it, but be explicit)
        const QString localPath = QDir::cleanPath(tempDir.path()) + QLatin1Char('/');

        // createDummyAccount creates an account with a FakeAM (empty FileInfo)
        auto accountState = createDummyAccount();
        Account *account = accountState->account();

        // Enable private links capability
        auto cap = TestUtils::testCapabilities();
        cap[QStringLiteral("files")] = QVariantMap{{QStringLiteral("privateLinks"), true}};
        account->setCapabilities({account->url(), cap});

        // Set a server override: intercept the Depth:0 PROPFIND that fetchPrivateLinkUrl issues
        // and reply with a 207 multistatus containing the private link URL.
        const QString expectedUrl = QStringLiteral("https://example.com/f/abc123");
        auto *fakeAm = dynamic_cast<FakeAM *>(account->accessManager());
        QVERIFY(fakeAm);
        fakeAm->setOverride([expectedUrl](QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &req,
                                          QIODevice *) -> QNetworkReply * {
            if (op != QNetworkAccessManager::CustomOperation)
                return nullptr;
            if (req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray()
                != QByteArrayLiteral("PROPFIND"))
                return nullptr;
            if (req.rawHeader(QByteArrayLiteral("Depth")) != QByteArrayLiteral("0"))
                return nullptr;

            const QString path = req.url().path();
            const QByteArray body =
                QByteArrayLiteral("<?xml version=\"1.0\"?>"
                                  "<d:multistatus xmlns:d=\"DAV:\" xmlns:oc=\"http://owncloud.org/ns\">"
                                  "<d:response><d:href>") + path.toUtf8()
                + QByteArrayLiteral("</d:href><d:propstat><d:prop>"
                                    "<oc:privatelink>") + expectedUrl.toUtf8()
                + QByteArrayLiteral("</oc:privatelink>"
                                    "</d:prop><d:status>HTTP/1.1 200 OK</d:status></d:propstat>"
                                    "</d:response></d:multistatus>");
            return new FakeMultistatReply(op, req, body, nullptr);
        });

        // Register the directory with FolderMan (paused so no sync fires)
        auto def = TestUtils::createDummyFolderDefinition(account, localPath);
        def.setPaused(true);
        Folder *folder = TestUtils::folderMan()->addFolder(accountState.get(), def);
        QVERIFY(folder);
        // Track the path so cleanup() can remove it after the test.
        _registeredFolderPath = localPath;

        // ----- Start SocketApi server -----
        SocketApi api;
        api.startShellIntegration();

        // Connect a client socket (triggers slotNewConnection → listener registered)
        std::unique_ptr<QLocalSocket> clientSocket(connectAndWaitForListener());
        QVERIFY(clientSocket);

        // Drain any broadcast messages sent on connect (e.g. REGISTER_PATH)
        QCoreApplication::processEvents();
        clientSocket->readAll();

        // ----- Send the GET_PRIVATE_LINK command -----
        // Use the sync-folder root as the localFile argument.
        // isSyncFolder() is true for the root path, so the journal record
        // check is bypassed.
        const QString command =
            QStringLiteral("GET_PRIVATE_LINK:") + localPath + QLatin1Char('\n');
        clientSocket->write(command.toUtf8());
        QVERIFY(clientSocket->flush());

        // ----- Wait for the PRIVATE_LINK response -----
        QSignalSpy readySpy(clientSocket.get(), &QLocalSocket::readyRead);
        QVERIFY(readySpy.wait(5000));

        // Collect all data (may arrive in chunks)
        QByteArray received;
        for (int retries = 5; retries > 0; --retries) {
            received += clientSocket->readAll();
            if (received.contains("PRIVATE_LINK:"))
                break;
            if (!readySpy.wait(1000))
                break;
        }

        const QByteArray expectedResponse =
            QByteArrayLiteral("PRIVATE_LINK:") + expectedUrl.toUtf8() + '\n';
        QVERIFY2(received.contains(expectedResponse),
            qPrintable(QStringLiteral("Expected '%1' in received data '%2'")
                .arg(QString::fromUtf8(expectedResponse), QString::fromUtf8(received))));
    }

    void testGetPrivateLinkNoResponseWhenFileNotSynced()
    {
        // The path /tmp/not_a_synced_file_testsocketapi.txt is not under any registered
        // sync root → command_GET_PRIVATE_LINK should return early with no reply.

        SocketApi api;
        api.startShellIntegration();

        std::unique_ptr<QLocalSocket> clientSocket(connectAndWaitForListener());
        QVERIFY(clientSocket);

        // Drain broadcast messages
        QCoreApplication::processEvents();
        clientSocket->readAll();

        const QString command =
            QStringLiteral("GET_PRIVATE_LINK:/tmp/not_a_synced_file_testsocketapi.txt\n");
        clientSocket->write(command.toUtf8());
        QVERIFY(clientSocket->flush());

        // Wait briefly — there should be no PRIVATE_LINK message.
        // Using QTest::qWait (unconditional) rather than QSignalSpy::wait so the
        // assertion is always evaluated and cannot pass vacuously when no data arrives.
        QTest::qWait(500);
        const QByteArray received = clientSocket->readAll();
        QVERIFY2(!received.contains("PRIVATE_LINK:"),
            qPrintable(QStringLiteral("Expected no PRIVATE_LINK response for unsynced file, got: ") + received));
    }
};

QTEST_GUILESS_MAIN(TestSocketApi)
#include "testsocketapi.moc"
