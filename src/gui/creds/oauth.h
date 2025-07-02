/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#pragma once
#include "accountfwd.h"
#include "owncloudlib.h"

#include <QNetworkReply>
#include <QPointer>
#include <QTcpServer>
#include <QUrl>
#include <QUrlQuery>


namespace OCC {
class JsonJob;

/**
 * Job that do the authorization grant and fetch the access token
 *
 * Normal workflow:
 *
 *   --> start()
 *       |
 *       +----> fetchWellKnown() query the ".well-known/openid-configuration" endpoint
 *       |
 *       +----> openBrowser() open the browser after fetchWellKnown finished to the specified page
 *       |                    (or the default 'oauth2/authorize' if fetchWellKnown does not exist)
 *       |                    Then the browser will redirect to http://localhost:xxx
 *       |
 *       +----> _server starts listening on a TCP port waiting for an HTTP request with a 'code'
 *                |
 *                v
 *             request the access_token and the refresh_token via 'apps/oauth2/api/v1/token'
 *                |
 *                +-> Request the user_id is not present
 *                |     |
 *                v     v
 *              finalize(...): Q_EMIT result(...)
 *
 */
class OAuth : public QObject
{
    Q_OBJECT
public:
    enum Result { NotSupported, LoggedIn, Error, ErrorInsecureUrl };
    Q_ENUM(Result)
    enum class TokenEndpointAuthMethods : char { client_secret_basic, client_secret_post };
    Q_ENUM(TokenEndpointAuthMethods)

    enum class PromptValuesSupported : char { none = 0, consent = 1 << 0, select_account = 1 << 1, login = 1 << 2, create = 1 << 3 };
    Q_ENUM(PromptValuesSupported)
    Q_DECLARE_FLAGS(PromptValuesSupportedFlags, PromptValuesSupported)

    OAuth(const QUrl &serverUrl, const QString &davUser, QNetworkAccessManager *networkAccessManager,QObject *parent);
    ~OAuth() override;

    virtual void startAuthentication();
    void openBrowser();
    QUrl authorisationLink() const;

Q_SIGNALS:
    /**
     * The state has changed.
     * when logged in, token has the value of the token.
     */
    void result(OAuth::Result result, const QString &token = QString(), const QString &refreshToken = QString());

    /**
     * emitted when the call to the well-known endpoint is finished
     */
    void authorisationLinkChanged();

    void fetchWellKnownFinished();


protected:

    QUrl _serverUrl;
    QString _davUser;
    QNetworkAccessManager *_networkAccessManager;
    bool _isRefreshingToken = false;

    QString _clientId;
    QString _clientSecret;


    virtual void fetchWellKnown();

    QNetworkReply *postTokenRequest(QUrlQuery &&queryItems);

protected Q_SLOTS:
    void handleSocketReadyRead();

private:
    void handleTcpConnection();
    void getTokens();
    void checkUserInfo();
    void finalize(const QString &accessToken, const QString &refreshToken, const QUrl &messageUrl);
    void httpReplyAndClose(
        QPointer<QTcpSocket> socket, const QString &code, const QString &title, const QString &body = {}, const QStringList &additionalHeader = {});

    void httpReplyAndClose(const QString &code, const QString &title, const QString &body, const QStringList &additionalHeader = {});

    QByteArray generateRandomString(size_t size) const;

    // The actual data we're collecting!
    QString _accessToken;
    QString _refreshToken;
    QUrl _messageUrl;

    // peripheral deps
    QTcpServer _server;
    QPointer<QTcpSocket> _socket;
    QUrlQuery _queryArgs;
    quint16 _serverPort;

    bool _wellKnownFinished = false;

    QUrl _authEndpoint;
    QUrl _tokenEndpoint;
    QString _redirectUrl;
    QByteArray _pkceCodeVerifier;
    QByteArray _state;

    TokenEndpointAuthMethods _endpointAuthMethod = TokenEndpointAuthMethods::client_secret_basic;
    PromptValuesSupportedFlags _supportedPromptValues = {PromptValuesSupported::consent, PromptValuesSupported::select_account};
};

/**
 * This variant of OAuth uses an account's network access manager etc.
 * Instead of relying on the user to provide a working server URL, a CheckServerJob is run upon start(), which also stores the fetched cookies in the account's state.
 * Furthermore, it takes care of storing and loading the dynamic registration data in the account's credentials manager.
 */
class AccountBasedOAuth : public OAuth
{
    Q_OBJECT

public:
    explicit AccountBasedOAuth(AccountPtr account, QObject *parent = nullptr);

    void startAuthentication() override;

    void refreshAuthentication(const QString &refreshToken);

Q_SIGNALS:
    void refreshError(QNetworkReply::NetworkError error, const QString &errorString);
    void refreshFinished(const QString &accessToken, const QString &refreshToken);

protected:
    void fetchWellKnown() override;

private:
    AccountPtr _account;
};

QString toString(OAuth::PromptValuesSupportedFlags s);
Q_DECLARE_OPERATORS_FOR_FLAGS(OAuth::PromptValuesSupportedFlags)
} // namespace OCC
