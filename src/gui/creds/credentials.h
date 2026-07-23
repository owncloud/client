/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "creds/abstractcredentials.h"
#include "creds/credentialssupport.h"
#include "gui/owncloudguilib.h"
#include "oauth.h"

#include <QMap>
#include <QSslCertificate>
#include <QSslKey>
#include <QNetworkRequest>

class QNetworkReply;
class QAuthenticator;

namespace OCC {
class OAuth;
class RequestAuthenticationController;

class OWNCLOUDGUI_EXPORT Credentials : public AbstractCredentials
{
    Q_OBJECT
    // ick
    friend class CredentialsAccessManager;

public:
    /// Don't add credentials if this is set on a QNetworkRequest

    explicit Credentials(const QString &token, const QString &refreshToken, Account *account);

    explicit Credentials(Account *account);
    // this access manager is monitored for authenticationRequired signal
    // it also becomes the access manager member in the account. Try to invert this so the account
    // creates it and connects the creds slot to the _am.
    // note this is used by the checkServerJobFactory where a new nam is "needed" for every job due to options to clear cookies
    // and other factors that mean we don't want to use the account's nam directly.
    AccessManager *createAccessManager() const override;

    // todo: DC-112 I think this needs a naming update.
    bool ready() const override;

    // this is invoked by AccountState::checkConnectivity. Totally uncool.
    // todo: DC-112 at least rename this to fetchCredentials() or something - also in base class. the "keychain" naming is an
    // impl detail we don't need to expose
    void fetchFromKeychain() override;

    void askFromUser() override;
    bool stillValid(QNetworkReply *reply) override;
    void persist() override;

    void invalidateToken() override;
    void forgetSensitiveData() override;


    /* If we don't have a valid refresh token, return false.
     * otherwise:
     *   if the refresh routine is running, return true
     *   if it is not running, start the refresh and return true.
     */
    bool refreshAccessToken() override;

protected:
    bool networkAvailable();


    void handleRefreshError(QNetworkReply::NetworkError error, const QString &message);
    void handleRefreshSuccess(const QString &accessToken, const QString &refreshToken);
    void finishFailedRefresh();
    void handleKeychainError(const QString &message);

    void slotAuthentication(QNetworkReply *reply, QAuthenticator *authenticator);
    void fetchCredentialsFromKeychain();
    void refreshAccessTokenInternal();

    void askFromUserSucceeded(const QString &token, const QString &refreshToken);
    void askFromUserLogout();

    QString _accessToken;
    QString _refreshToken;

    QString _fetchErrorString;
    bool _ready = false;
    const OpenIdConfig _openIdConfig;
    int _tokenRefreshRetriesCount = 0;

    QPointer<AccountBasedOAuth> _oAuthJob;

    RequestAuthenticationController *_requestAuth = nullptr;
};


} // namespace OCC
