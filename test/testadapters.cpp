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
 * Note: in this class we cannot use QCOMPARE/QVERIFY/etc, so we use asserts to validate the requests.
 */
class TestAdaptersAM : public FakeAM
{
    Q_OBJECT

public:
    TestAdaptersAM(const QUrl &urlWithoutPath, const QString &authToken = {})
        : FakeAM({}, nullptr)
        , _urlWithoutPath(urlWithoutPath)
        , _authToken(authToken)
    {
        setOverride([&](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) -> QNetworkReply * {
            return this->handleRequest(op, req, device);
        });
    }

protected:
    virtual QNetworkReply *handleRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device)
    {
        if (op == QNetworkAccessManager::GetOperation) {
            QString reqPath = req.url().path();
            if (reqPath == QStringLiteral("/.well-known/webfinger")) {
                const QString resource = QUrlQuery(req.url()).queryItemValue(QStringLiteral("resource"));
                if (resource == _urlWithoutPath.toString()) {
                    return handleWebFingerDiscoveryRequest(op, req);
                } else if (resource == QStringLiteral("acct:me@%1").arg(_urlWithoutPath.host())) {
                    return handleWebFingerLookupRequest(op, req);
                }
            } else if (reqPath == QStringLiteral("/ocs/v2.php/cloud/capabilities")) {
                return handleCapabilitiesRequest(op, req);
            }
        } else if (op == QNetworkAccessManager::CustomOperation && req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() == "PROPFIND") {
            auto reply = new FakeErrorReply(op, req, this, 401, {});
            reply->setRawHeader(QByteArrayLiteral("www-authenticate"), QByteArrayLiteral("Bearer realm=\"ownCloudTest\""));
            return reply;
        }

        Q_UNREACHABLE(); // Bomb out, the adapter send a strange request operation or path
    }

    virtual QNetworkReply *handleWebFingerDiscoveryRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req)
    {
        return new AdapterJSonReply(op, req, {ValidWebFingerResponse}, this);
    }

    virtual QNetworkReply *handleWebFingerLookupRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req)
    {
        verifyAuthentication(req);
        const QString expectedResource = QStringLiteral("acct:me@%1").arg(_urlWithoutPath.host());
        Q_ASSERT(QUrlQuery(req.url()).queryItemValue(QStringLiteral("resource")) == expectedResource);

        return new AdapterJSonReply(op, req, {ValidWebFingerLookupResponse}, this);
    }

    virtual QNetworkReply *handleCapabilitiesRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req)
    {
        verifyAuthentication(req);
        Q_ASSERT(QUrlQuery(req.url()).queryItemValue(QStringLiteral("format")) == QStringLiteral("json"));

        return new AdapterJSonReply(op, req, ValidFetchCapabilitiesResponse, this);
    }

    QNetworkReply *errorResponse(QNetworkAccessManager::Operation op, const QNetworkRequest &req, int httpErrorCode = 404)
    {
        return new FakeErrorReply(op, req, this, httpErrorCode);
    }

    void verifyAuthentication(const QNetworkRequest &req)
    {
        Q_ASSERT(req.rawHeader("Authorization") == QByteArrayLiteral("Bearer ").append(_authToken.toUtf8()));
    }

    static QByteArray ValidWebFingerResponse;
    static QByteArray ValidWebFingerLookupResponse;
    static QByteArray ValidFetchCapabilitiesResponse;

private:
    const QUrl _urlWithoutPath;
    const QString _authToken;
};

QByteArray TestAdaptersAM::ValidWebFingerResponse = QByteArrayLiteral(
    "{\"subject\":\"https://demo.test\",\"links\":[{\"rel\":\"http://openid.net/specs/connect/1.0/issuer\",\"href\":\"https://demo.test/realms/test\"}]}");
QByteArray TestAdaptersAM::ValidWebFingerLookupResponse =
    QByteArrayLiteral("{\"links\":[{\"rel\":\"http://webfinger.owncloud/rel/server-instance\",\"href\":\"one.demo.test\"},{\"rel\":\"http://webfinger.owncloud/"
                      "rel/server-instance\",\"href\":\"two.demo.test\"}]}");
QByteArray TestAdaptersAM::ValidFetchCapabilitiesResponse = QByteArrayLiteral(
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


class FailureAM : public TestAdaptersAM
{
public:
    FailureAM(const QUrl &urlWithoutPath)
        : TestAdaptersAM(urlWithoutPath)
    {
    }

protected:
    QNetworkReply *handleRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) override
    {
        Q_UNUSED(device);

        // Send the reply
        // no well-known endpoints defined
        return errorResponse(op, req);
    }
};

class TestAdapters : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testWebFingerDiscovery_Failure()
    {
        FailureAM fakeAM(testUrl());

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, testUrl());
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

    void testWebFingerDiscovery_Success()
    {
        TestAdaptersAM fakeAM(testUrl());

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, testUrl());
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.href, QStringLiteral("https://demo.test/realms/test"));
    }

    void testWebFingerDiscovery_BrokenJSON()
    {
        class BrokenAM : public TestAdaptersAM
        {
        public:
            BrokenAM()
                : TestAdaptersAM(testUrl())
            {
            }

        protected:
            QNetworkReply *handleWebFingerDiscoveryRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req) override
            {
                return new AdapterJSonReply(op, req, ValidWebFingerResponse.first(80), this);
            }
        } fakeAM;

        DiscoverWebFingerServiceAdapter adapter(&fakeAM, testUrl());
        DiscoverWebFingerServiceResult result = adapter.getResult();
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.href.isEmpty());
    }

    void testDetermineAuthType_OAUTH()
    {
        TestAdaptersAM fakeAM(testUrl());

        DetermineAuthTypeAdapter adapter(&fakeAM, testUrl());
        DetermineAuthTypeResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.type, AuthenticationType::OAuth);
    }

    void testFetchCapabilitiesAdapter_Success()
    {
        TestAdaptersAM fakeAM(testUrl(), testToken());

        FetchCapabilitiesAdapter adapter(&fakeAM, testToken(), testUrl(), nullptr);
        FetchCapabilitiesResult result = adapter.getResult();
        QVERIFY(result.error.isEmpty());
    }

    void testWebFingerLookupAdapter_Success()
    {
        TestAdaptersAM fakeAM(testUrl(), testToken());

        WebFingerLookupAdapter adapter(&fakeAM, testToken(), testUrl(), nullptr);
        WebFingerLookupResult result = adapter.getResult();
        QVERIFY(result.success());
        QCOMPARE(result.urls.size(), 2);
    }

private:
    static QUrl testUrl() { return QUrl(QStringLiteral("https://demo.test")); }
    static QString testToken() { return QStringLiteral("theToken"); }
};

QTEST_GUILESS_MAIN(TestAdapters)
#include "testadapters.moc"
