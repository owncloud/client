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
    // it is ALSO used by the checkServerJobFactory where it sets the parent to the parent of that job. This is so
    // shady and needs to be fixed, but we may have to leave that to a future refactoring.
    // the main point is I am not sure the creds should be creating the nam - I think this should be an account responsibility in the end.
    AccessManager *createAccessManager() const override;

    // todo: DC-112 I think this needs a naming update.
    bool ready() const override;

    // this is invoked by AccountState::checkConnectivity. Totally uncool.
    // todo: DC-112 at least rename this to fetchCredentials() or something - also in base class. the "keychain" naming is an
    // impl detail we don't need to expose
    void fetchFromKeychain() override;

    bool stillValid(QNetworkReply *reply) override;
    void persist() override;

    QString user() const override;
    void invalidateToken() override;
    void forgetSensitiveData() override;


    /* If we don't have a valid refresh token, return false.
     * otherwise:
     *   if the refresh routine is running, return true
     *   if it is not running, start the refresh and return true.
     */
    bool refreshAccessToken();

protected:
    bool networkAvailable();


    void handleRefreshError(QNetworkReply::NetworkError error, const QString &message);
    void handleRefreshSuccess(const QString &accessToken, const QString &refreshToken);
    void finishFailedRefresh();
    void handleKeychainError(const QString &message);

    void slotAuthentication(QNetworkReply *reply, QAuthenticator *authenticator);
    void fetchCredentialsFromKeychain();
    void refreshAccessTokenInternal();

    QString _accessToken;
    QString _refreshToken;
    QString _previousAccessToken;

    QString _fetchErrorString;
    bool _ready = false;
    int _tokenRefreshRetriesCount = 0;

    QPointer<AccountBasedOAuth> _oAuthJob;
};


} // namespace OCC
