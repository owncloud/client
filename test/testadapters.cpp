/*
 * Copyright (C) Erik Verbruggen <erik.verbruggenkiteworks.com>
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

#include "../src/gui/networkadapters/discoverwebfingerserviceadapter.h"
#include "testutils/syncenginetestutils.h"

using namespace OCC;

class AdapterReply : public FakeReply
{
    Q_OBJECT

public:
    AdapterReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &payload, QObject *parent)
        : FakeReply(parent)
        , payload(payload)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        open(QIODevice::ReadOnly);

        QMetaObject::invokeMethod(this, &AdapterReply::respond, Qt::QueuedConnection);
    }

    Q_INVOKABLE void respond()
    {
        setHeader(QNetworkRequest::ContentLengthHeader, payload.size());
        setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        Q_EMIT metaDataChanged();
        if (bytesAvailable())
            Q_EMIT readyRead();
        checkedFinished();
    }

    qint64 bytesAvailable() const override
    {
        return payload.size();
    }

    qint64 readData(char *data, qint64 maxlen) override
    {
        qint64 len = std::min(qint64 { payload.size() }, maxlen);
        std::copy(payload.cbegin(), payload.cbegin() + len, data);
        payload.remove(0, static_cast<int>(len));
        return len;
    }

private:
    QByteArray payload;
};

class TestAdapters : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testWebFingerDiscoveryFailure()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device);
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);
            // no well-known endpoints defined
            return new FakeErrorReply(op, req, &fakeAM, 404);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, QUrl::fromUserInput(QStringLiteral("http://localhost/")));
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

    void testWebFingerDiscoverySuccess()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);
            return new AdapterReply(op, req, {ValidResponse}, &fakeAM);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, QUrl(QStringLiteral("https://demo.test")));
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.href, QStringLiteral("https://demo.test/realms/test"));
    }

    void testWebFingerDiscoveryBrokenJSON()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);
            return new AdapterReply(op, req, QByteArray(ValidResponse).first(80), &fakeAM);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, QUrl(QStringLiteral("https://demo.test")));
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

private:
    static char ValidResponse[];
};

char TestAdapters::ValidResponse[] = "{\"subject\":\"https://demo.test\",\"links\":[{\"rel\":\"http://openid.net/specs/connect/1.0/issuer\",\"href\":\"https://demo.test/realms/test\"}]}";

QTEST_GUILESS_MAIN(TestAdapters)
#include "testadapters.moc"
