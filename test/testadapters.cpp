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

#include "../src/gui/networkadapters/determineauthtypeadapter.h"
#include "../src/gui/networkadapters/discoverwebfingerserviceadapter.h"
#include "../src/gui/networkadapters/fetchcapabilitiesadapter.h"
// #include "../src/gui/networkadapters/resolveurladapter.h"
#include "../src/gui/networkadapters/webfingerlookupadapter.h"
#include "testutils/syncenginetestutils.h"

using namespace OCC;

/**
 * @brief A reply object carrying json payload, with the headers set accordingly, and a 200 Ok
 *        response code.
 */
class AdapterJSonReply : public FakeReply
{
    Q_OBJECT

public:
    AdapterJSonReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &payload, QObject *parent)
        : FakeReply(parent)
        , payload(payload)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        open(QIODevice::ReadOnly);

        QMetaObject::invokeMethod(this, &AdapterJSonReply::respond, Qt::QueuedConnection);
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

/*
 * Note: in the override for the fake NAM we cannot use QCOMPARE/QVERIFY/etc, so we use asserts to
 *       validate the requests.
 */
class TestAdapters : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testWebFingerDiscovery_Failure()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device);

            // Validate the request
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);

            // Send the reply
            // no well-known endpoints defined
            return new FakeErrorReply(op, req, &fakeAM, 404);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, QUrl::fromUserInput(QStringLiteral("http://localhost/")));
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

    void testWebFingerDiscovery_Success()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)

            // Validate the request
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);

            // Send the reply
            return new AdapterJSonReply(op, req, {ValidWebFingerResponse}, &fakeAM);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, testUrl());
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.href, QStringLiteral("https://demo.test/realms/test"));
    }

    void testWebFingerDiscovery_BrokenJSON()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)

            // Validate the request
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);

            // Send the reply
            return new AdapterJSonReply(op, req, ValidWebFingerResponse.first(80), &fakeAM);
        });

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, testUrl());
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

    void testDetermineAuthType_OAUTH()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)

            // Send the reply
            auto reply = new FakeErrorReply(op, req, &fakeAM, 401, {});
            reply->setRawHeader(QByteArrayLiteral("www-authenticate"), QByteArrayLiteral("Bearer realm=\"ownCloudTest\""));
            return reply;
        });

        DetermineAuthTypeAdapter adapter(&fakeAM, testUrl());
        DetermineAuthTypeResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.type, AuthenticationType::OAuth);
    }

    void testFetchCapabilitiesAdapter_Success()
    {
        FakeAM fakeAM({}, nullptr);
        fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)

            // Validate the request
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);
            Q_ASSERT(req.url().path().endsWith(QStringLiteral("ocs/v2.php/cloud/capabilities")));
            Q_ASSERT(QUrlQuery(req.url()).queryItemValue(QStringLiteral("format")) == QStringLiteral("json"));

            // Send the reply
            return new AdapterJSonReply(op, req, ValidFetchCapabilitiesResponse, &fakeAM);
        });

        FetchCapabilitiesAdapter adapter(&fakeAM, QStringLiteral("theToken"), testUrl(), nullptr);
        FetchCapabilitiesResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
    }

    void testWebFingerLookupAdapter_Success()
    {
        FakeAM fakeAM({}, nullptr);
        const QString authToken = QStringLiteral("theToken");
        fakeAM.setOverride([&fakeAM, authToken](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            Q_UNUSED(device)

            // Validate the request
            Q_ASSERT(op == QNetworkAccessManager::GetOperation);
            Q_ASSERT(req.url().path().endsWith(QStringLiteral("/.well-known/webfinger")));
            const QString resource = QStringLiteral("acct:me@%1").arg(testUrl().host());
            Q_ASSERT(QUrlQuery(req.url()).queryItemValue(QStringLiteral("resource")) == resource);
            Q_ASSERT(req.rawHeader("Authorization") == QByteArrayLiteral("Bearer ").append(authToken.toUtf8()));

            // Send the reply
            return new AdapterJSonReply(op, req, ValidWebFingerLookupResponse, &fakeAM);
        });

        WebFingerLookupAdapter adapter(&fakeAM, QStringLiteral("theToken"), testUrl(), nullptr);
        WebFingerLookupResult result = adapter.getResult();
        QVERIFY(result.success());
        QCOMPARE(result.urls.size(), 2);
    }

    // void testResolveUrlAdapter_Success()
    // {
    //     FakeAM fakeAM({}, nullptr);
    //     fakeAM.setOverride([&fakeAM](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
    //         Q_UNUSED(device)

    //         // Validate the request
    //         Q_ASSERT(op == QNetworkAccessManager::GetOperation);
    //         Q_ASSERT(req.url().path() == QStringLiteral("status.php"));

    //         // Send the reply, content doesn't matter: this adapter is only used to see if the
    //         // server exists at exactly this URL (i.e. no redirects).
    //         return new AdapterJSonReply(op, req, QByteArrayLiteral("{}"), &fakeAM);
    //     });

    //     ResolveUrlAdapter adapter(&fakeAM, testUrl(), nullptr);
    //     ResolveUrlResult result = adapter.getResult();
    //     QVERIFY(result.success());
    //     QVERIFY(result.acceptedCertificates.isEmpty());
    //     QCOMPARE(result.resolvedUrl, testUrl());
    // }

private:
    static QByteArray ValidWebFingerResponse;
    static QByteArray ValidWebFingerLookupResponse;
    static QByteArray ValidFetchCapabilitiesResponse;

    static QUrl testUrl() { return QUrl(QStringLiteral("https://demo.test")); }
};

QByteArray TestAdapters::ValidWebFingerResponse = QByteArrayLiteral(
    "{\"subject\":\"https://demo.test\",\"links\":[{\"rel\":\"http://openid.net/specs/connect/1.0/issuer\",\"href\":\"https://demo.test/realms/test\"}]}");
QByteArray TestAdapters::ValidWebFingerLookupResponse =
    QByteArrayLiteral("{\"links\":[{\"rel\":\"http://webfinger.owncloud/rel/server-instance\",\"href\":\"one.demo.test\"},{\"rel\":\"http://webfinger.owncloud/"
                      "rel/server-instance\",\"href\":\"two.demo.test\"}]}");
QByteArray TestAdapters::ValidFetchCapabilitiesResponse = QByteArrayLiteral(
    "{\"ocs\":{\"data\":{\"capabilities\":{\"checksums\":{\"preferredUploadType\":\"SHA3-256\",\"supportedTypes\":[\"SHA3-256\"]},\"core\":{\"pollinterval\":"
    "30000,\"status\":{\"edition\":\"Enterprise\",\"installed\":true,\"maintenance\":false,\"needsDbUpgrade\":false,\"product\":\"test\",\"productname\":"
    "\"test\",\"productversion\":\"8.4.1\",\"version\":\"10.0.11.5\",\"versionstring\":\"10.0.11\"},\"support-url-signing\":false},\"dav\":{\"chunking\":\"\","
    "\"chunkingParallelUploadDisabled\":false,\"reports\":null,\"trashbin\":\"\"},\"files\":{\"app_providers\":null,\"archivers\":null,\"bigfilechunking\":"
    "false,\"blacklisted_files\":null,\"favorites\":false,\"permanent_deletion\":false,\"privateLinks\":true,\"tus_support\":null,\"undelete\":false,"
    "\"versioning\":false},\"files_sharing\":{\"allow_custom\":false,\"api_enabled\":false,\"auto_accept_share\":false,\"can_rename\":false,\"default_"
    "permissions\":0,\"deny_access\":false,\"federation\":null,\"group_sharing\":false,\"public\":null,\"resharing\":false,\"resharing_default\":false,"
    "\"search_min_length\":0,\"share_with_group_members_only\":false,\"share_with_membership_groups_only\":false,\"user\":{\"profile_picture\":true,\"send_"
    "mail\":false,\"settings\":null},\"user_enumeration\":null},\"notifications\":{},\"spaces\":{\"enabled\":true,\"has_multiple_personal_spaces\":true,"
    "\"projects\":true,\"version\":\"1.0.0\"}},\"version\":{\"edition\":\"Enterprise\",\"major\":10,\"micro\":11,\"minor\":0,\"product\":\"test\",\"string\":"
    "\"10.0.11\"}},\"meta\":{\"message\":\"OK\",\"status\":\"ok\",\"statuscode\":100}}}");

QTEST_GUILESS_MAIN(TestAdapters)
#include "testadapters.moc"
