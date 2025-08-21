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
    friend class CredentialsAccessManager;

public:
    /// Don't add credentials if this is set on a QNetworkRequest

    explicit Credentials(const QString &userId, const QString &token, const QString &refreshToken);

    // ideally should go - this string is used to create settings dynamically.
    QString credentialsType() const override;

    // this access manager is monitored for authenticationRequired signal
    // it also becomes the access manager member in the account. Try to invert this so the account
    // creates it and connects the creds slot to the _am.
    // it is ALSO used by the checkServerJobFactory where it sets the parent to the parent of that job. This is so
    // shady and needs to be fixed, but we may have to leave that to a future refactoring.
    // the main point is I am not sure the creds should be creating the nam - I think this should be an account responsibility in the end.
    AccessManager *createAccessManager() const override;

    // this needs a naming update. the ready state seems to go to false only when the user actually needs to reauthenticate
    // but needs review
    bool ready() const override;
    // this is invoked by AccountState::checkConnectivity. Totally uncool. This should be private with appropriate logic
    // to trigger it only under x circumstances - presumably it would be associated with the ready() stuff but not sure yet
    void fetchFromKeychain() override;

    bool stillValid(QNetworkReply *reply) override;
    void persist() override;
    QString user() const override;
    void invalidateToken() override;
    void forgetSensitiveData() override;
    QString fetchUser();

    /* If we still have a valid refresh token, try to refresh it asynchronously and Q_EMIT fetched()
     * otherwise return false
     */
    bool refreshAccessToken();

    // To fetch the user name as early as possible
    // Lisa todo: what does that mean? we should already have the user name before we even create the account
    // this has a special aroma. Also the idea from the base is that the account can only be set once
    // which is a shining beacon in the night that screams: "this belongs in the ctr"
    void setAccount(Account *account) override;

protected:
    Credentials() = default;

    void slotAuthentication(QNetworkReply *reply, QAuthenticator *authenticator);
    void fetchFromKeychainHelper();

    QString _userId;
    QString _accessToken;
    QString _refreshToken;
    QString _previousAccessToken;

    QString _fetchErrorString;
    bool _ready = false;
    QPointer<AccountBasedOAuth> _oAuthJob;


private:
    bool refreshAccessTokenInternal(int tokenRefreshRetriesCount);
};


} // namespace OCC
