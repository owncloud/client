/*
 * Copyright (C) by Lisa Reese <lisa.reese@kiteworks.com>
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
#include "credentials.h"

#include "accessmanager.h"
#include "account.h"
#include "common/asserts.h"
#include "configfile.h"
#include "creds/credentialmanager.h"
#include "oauth.h"

#include <QAuthenticator>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkInformation>
#include <QNetworkReply>

#include <chrono>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcCredentials, "gui.credentials", QtInfoMsg)

namespace {
constexpr int TokenRefreshMaxRetries = 3;
constexpr std::chrono::seconds TokenRefreshDefaultTimeout = 30s;
constexpr int CredentialVersion = 1;
const char authenticationFailedC[] = "owncloud-authentication-failed";

// when creating new keys we should probably stick with this idea related to a creds "type" in case we need to add or remove
// other cred types in future. all of the updates to transition to oauth only would have been so much easier if the last round had actually implmented
// separate http and oauth creds instead of rolling all of it into the httpCreds "type".
QString refreshTokenKeyC()
{
    return QStringLiteral("http/oauthtoken");
}

QString CredentialVersionKeyC()
{
    return QStringLiteral("CredentialVersion");
}

const QString userIdC()
{
    return QStringLiteral("user");
}
}

namespace OCC {

class CredentialsAccessManager : public AccessManager
{
    Q_OBJECT
public:
    CredentialsAccessManager(const Credentials *cred, QObject *parent = nullptr)
        : AccessManager(parent)
        , _cred(cred)
    {
    }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData) override
    {
        QNetworkRequest req(request);
        if (!req.attribute(DontAddCredentialsAttribute).toBool()) {
            if (_cred && !_cred->_accessToken.isEmpty()) {
                req.setRawHeader("Authorization", "Bearer " + _cred->_accessToken.toUtf8());
            }
        }
        return AccessManager::createRequest(op, req, outgoingData);
    }

private:
    // The credentials object dies along with the account, while the QNAM might
    // outlive both.
    // Lisa todo: that statement is very concerning. investigate.
    QPointer<const Credentials> _cred;
};

Credentials::Credentials(const QString &userId, const QString &token, const QString &refreshToken)
    : _userId(userId)
    , _accessToken(token)
    , _refreshToken(refreshToken)
    , _ready(true)
{
}

//
QString Credentials::credentialsType() const
{
    return QStringLiteral("http");
}

QString Credentials::user() const
{
    return _userId;
}

void Credentials::setAccount(Account *account)
{
    AbstractCredentials::setAccount(account);
    if (_userId.isEmpty()) {
        fetchUser();
    }
}

AccessManager *Credentials::createAccessManager() const
{
    AccessManager *am = new CredentialsAccessManager(this);

    connect(am, &QNetworkAccessManager::authenticationRequired, this, &Credentials::slotAuthentication);

    return am;
}

bool Credentials::ready() const
{
    return _ready;
}

QString Credentials::fetchUser()
{
    // it makes no sense to overwrite an existing username with a config file value
    if (_userId.isEmpty()) {
        qCDebug(lcCredentials) << "user not set, populating from settings";
        _userId = _account->credentialSetting(userIdC()).toString();
    } else {
        qCDebug(lcCredentials) << "user already set, no need to fetch from settings";
    }
    return _userId;
}

void Credentials::fetchFromKeychain()
{
    _wasFetched = true;

    // User must be fetched from config file
    fetchUser();

    if (!_ready && !_refreshToken.isEmpty()) {
        // This happens if the credentials are still loaded from the keychain, bur we are called
        // here because the auth is invalid, so this means we simply need to refresh the credentials
        refreshAccessToken();
        return;
    }

    if (_ready) {
        Q_EMIT fetched();
    } else {
        fetchFromKeychainHelper();
    }
}

void Credentials::fetchFromKeychainHelper()
{
    if (_userId.isEmpty()) {
        _accessToken.clear();
        _ready = false;
        Q_EMIT fetched();
        return;
    }
    auto job = _account->credentialManager()->get(refreshTokenKeyC());
    if (!job) {
        qCWarning(lcCredentials) << "get credentials job is null - most likely the key does not exist in the credentials manager";
        // doing this temp copy paste since handle error is not particularly reusable. I will fix this in the new class
        _fetchErrorString = QString();
        _accessToken.clear();
        _ready = false;
        Q_EMIT fetched();
        return;
    }
    connect(job, &CredentialJob::finished, this, [job, this] {
        auto handleError = [job, this] {
            qCWarning(lcCredentials) << "Could not retrieve client password from keychain" << job->errorString();

            // we come here if the password is empty or any other keychain
            // error happend.

            _fetchErrorString = job->error() != QKeychain::EntryNotFound ? job->errorString() : QString();

            _accessToken.clear();
            _ready = false;
            Q_EMIT fetched();
        };
        if (job->error() != QKeychain::NoError) {
            handleError();
            return;
        }
        const auto data = job->data().toString();
        if (OC_ENSURE(!data.isEmpty())) {
                _refreshToken = data;
                refreshAccessToken();

        } else {
            handleError();
        }
    });
}

// Lisa todo: this is called periodically, find the trigger
bool Credentials::stillValid(QNetworkReply *reply)
{
    // The function is called in order to determine whether we need to ask the user for a password
    // since we are using OAuth, we already started a refresh in slotAuthentication, at least in theory, ensure the auth is started.
    // If the refresh fails, we are going to Q_EMIT authenticationFailed ourselves
    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        slotAuthentication(reply, nullptr);
    }
        return true;
}

void Credentials::slotAuthentication(QNetworkReply *reply, QAuthenticator *authenticator)
{
    qCDebug(lcCredentials) << Q_FUNC_INFO << reply;
    if (!_ready)
        return;
    Q_UNUSED(authenticator)
    // Because of issue #4326, we need to set the login and password manually at every requests
    // Thus, if we reach this signal, those credentials were invalid and we terminate.
    qCWarning(lcCredentials) << "Stop request: Authentication failed for " << reply->url().toString() << reply->request().rawHeader("Original-Request-ID");
    reply->setProperty(authenticationFailedC, true);

    if (!_oAuthJob) {
        qCInfo(lcCredentials) << "Refreshing token";
        refreshAccessToken();
    }
}

bool Credentials::refreshAccessToken()
{
    return refreshAccessTokenInternal(0);
}

bool Credentials::refreshAccessTokenInternal(int tokenRefreshRetriesCount)
{
    if (_refreshToken.isEmpty())
        return false;
    if (_oAuthJob) {
        return true;
    }

    // don't touch _ready or the account state will start a new authentication
    // _ready = false;

    // parent with nam to ensure we reset when the nam is reset
    _oAuthJob = new AccountBasedOAuth(_account->sharedFromThis(), _account->accessManager());
    connect(_oAuthJob, &AccountBasedOAuth::refreshError, this, [tokenRefreshRetriesCount, this](QNetworkReply::NetworkError error, const QString &) {
        _oAuthJob->deleteLater();

        auto networkUnavailable = []() {
            if (auto qni = QNetworkInformation::instance()) {
                if (qni->reachability() == QNetworkInformation::Reachability::Disconnected) {
                    return true;
                }
            }

            return false;
        };

        int nextTry = tokenRefreshRetriesCount + 1;
        std::chrono::seconds timeout = {};

        if (networkUnavailable()) {
            nextTry = 0;
            timeout = TokenRefreshDefaultTimeout;
        } else {
            switch (error) {
            case QNetworkReply::ContentNotFoundError:
                // 404: bigip f5?
                timeout = 0s;
                break;
            case QNetworkReply::HostNotFoundError:
                [[fallthrough]];
            case QNetworkReply::TimeoutError:
                [[fallthrough]];
            // Qt reports OperationCanceledError if the request timed out
            case QNetworkReply::OperationCanceledError:
                [[fallthrough]];
            case QNetworkReply::TemporaryNetworkFailureError:
                [[fallthrough]];
            // VPN not ready?
            case QNetworkReply::ConnectionRefusedError:
                nextTry = 0;
                [[fallthrough]];
            default:
                timeout = TokenRefreshDefaultTimeout;
            }
        }

        if (nextTry >= TokenRefreshMaxRetries) {
            qCWarning(lcCredentials) << "Too many failed refreshes" << nextTry << "-> log out";
            forgetSensitiveData();
            Q_EMIT authenticationFailed();
            Q_EMIT fetched();
            return;
        }
        QTimer::singleShot(timeout, this, [nextTry, this] {
            refreshAccessTokenInternal(nextTry);
        });
        Q_EMIT authenticationFailed();
    });

    connect(_oAuthJob, &AccountBasedOAuth::refreshFinished, this, [this](const QString &accessToken, const QString &refreshToken) {
        _oAuthJob->deleteLater();
        if (refreshToken.isEmpty()) {
            // an error occured, log out
            forgetSensitiveData();
            Q_EMIT authenticationFailed();
            Q_EMIT fetched();
            return;
        }
        _refreshToken = refreshToken;
        if (!accessToken.isNull()) {
            _ready = true;
            _accessToken = accessToken;
            persist();
        }
        Q_EMIT fetched();
    });
    Q_EMIT authenticationStarted();
    _oAuthJob->refreshAuthentication(_refreshToken);

    return true;
}

void Credentials::invalidateToken()
{
    qCWarning(lcCredentials) << "Invalidating the credentials";

    if (!_accessToken.isEmpty()) {
        _previousAccessToken = _accessToken;
    }
    _accessToken = QString();
    _ready = false;

    // User must be fetched from config file to generate a valid key
    fetchUser();

    // clear the session cookie.
    _account->clearCookieJar();

    if (!_refreshToken.isEmpty()) {
        // Only invalidate the access_token (_password) but keep the _refreshToken in the keychain
        // (when coming from forgetSensitiveData, the _refreshToken is cleared)
        return;
    }

    _account->credentialManager()->clear(QStringLiteral("http"));
    // let QNAM forget about the password
    // This needs to be done later in the event loop because we might be called (directly or
    // indirectly) from QNetworkAccessManagerPrivate::authenticationRequired, which itself
    // is a called from a BlockingQueuedConnection from the Qt HTTP thread. And clearing the
    // cache needs to synchronize again with the HTTP thread.
    QTimer::singleShot(0, _account, &Account::clearAMCache);
}

void Credentials::forgetSensitiveData()
{
    // need to be done before invalidateToken, so it actually deletes the refresh_token from the keychain
    _refreshToken.clear();

    invalidateToken();
    _previousAccessToken.clear();
}

void Credentials::persist()
{
    if (_userId.isEmpty()) {
        // We never connected or fetched the user, there is nothing to save.
        return;
    }
    // the version key is going away but we should READ it then use it's existence to migrate
    // any existing creds to the new persistence style
    _account->addCredentialSetting(CredentialVersionKeyC(), CredentialVersion);
    _account->addCredentialSetting(userIdC(), _userId);
    Q_EMIT _account->wantsAccountSaved(_account);

    // write secrets to the keychain
        // _refreshToken should only be empty when we are logged out...
        if (!_refreshToken.isEmpty()) {
            _account->credentialManager()->set(refreshTokenKeyC(), _refreshToken);
        } 
}

} // namespace OCC


#include "credentials.moc"
