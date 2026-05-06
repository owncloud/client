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
#include <QBuffer>
#include <QTemporaryDir>

#include <chrono>
using namespace std::chrono_literals;

#include "socketapi/socketapi.h"
#include "socketapi/socketapi_p.h"
#include "folderman.h"
#include "accountmanager.h"

#include "testutils/syncenginetestutils.h"
#include "testutils/testutils.h"

using namespace OCC;

// Minimal 207 multistatus reply for intercepting PROPFIND requests in tests.
class FakeMultistatReply : public FakeReply
{
    Q_OBJECT
public:
    QByteArray _body;
    FakeMultistatReply(QNetworkAccessManager::Operation op, const QNetworkRequest &req,
                       const QByteArray &body, QObject *parent)
        : FakeReply(parent), _body(body)
    {
        setRequest(req);
        setUrl(req.url());
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
        memcpy(buf, _body.constData(), static_cast<size_t>(max));
        _body.remove(0, static_cast<int>(max));
        return max;
    }
    qint64 bytesAvailable() const override { return _body.size(); }
};

class TestSocketApi : public QObject
{
    Q_OBJECT

    QString _registeredFolderPath;

private Q_SLOTS:
    void initTestCase()
    {
        TestUtils::folderMan();
    }

    void cleanup()
    {
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
        const QTemporaryDir tempDir = TestUtils::createTempDir();
        QVERIFY(tempDir.isValid());
        const QString localPath = QDir::cleanPath(tempDir.path()) + QLatin1Char('/');

        auto accountState = createDummyAccount();
        Account *account = accountState->account();

        auto cap = TestUtils::testCapabilities();
        cap[QStringLiteral("files")] = QVariantMap{{QStringLiteral("privateLinks"), true}};
        account->setCapabilities({account->url(), cap});

        const QString expectedUrl = QStringLiteral("https://example.com/f/abc123");
        auto *fakeAm = dynamic_cast<FakeAM *>(account->accessManager());
        QVERIFY(fakeAm);
        fakeAm->setOverride([expectedUrl](QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &req,
                                          QIODevice *) -> QNetworkReply * {
            if (op != QNetworkAccessManager::CustomOperation)
                return nullptr;
            if (req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() != QByteArrayLiteral("PROPFIND"))
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

        auto def = TestUtils::createDummyFolderDefinition(account, localPath);
        def.setPaused(true);
        Folder *folder = TestUtils::folderMan()->addFolder(accountState.get(), def);
        QVERIFY(folder);
        _registeredFolderPath = localPath;

        // Use a QBuffer as the socket so SocketListener::sendMessage has somewhere to write.
        QBuffer capture;
        capture.open(QIODevice::ReadWrite);
        SocketListener listener(&capture);

        SocketApi api;
        QMetaObject::invokeMethod(&api, "command_GET_PRIVATE_LINK",
            Q_ARG(QString, localPath), Q_ARG(OCC::SocketListener *, &listener));

        const QByteArray expected =
            QByteArrayLiteral("PRIVATE_LINK:") + expectedUrl.toUtf8() + '\n';
        QTRY_VERIFY2(capture.data().contains(expected),
            qPrintable(QStringLiteral("Expected '%1' in buffer '%2'")
                .arg(QString::fromUtf8(expected), QString::fromUtf8(capture.data()))));
    }

    void testGetPrivateLinkNoResponseWhenFileNotSynced()
    {
        QBuffer capture;
        capture.open(QIODevice::ReadWrite);
        SocketListener listener(&capture);

        SocketApi api;
        QMetaObject::invokeMethod(&api, "command_GET_PRIVATE_LINK",
            Q_ARG(QString, QStringLiteral("/tmp/not_a_synced_file_testsocketapi.txt")),
            Q_ARG(OCC::SocketListener *, &listener));

        QTest::qWait(500);
        QVERIFY2(!capture.data().contains("PRIVATE_LINK:"),
            qPrintable(QStringLiteral("Expected no PRIVATE_LINK response for unsynced file, got: ")
                + QString::fromUtf8(capture.data())));
    }
};

QTEST_GUILESS_MAIN(TestSocketApi)
#include "testsocketapi.moc"
