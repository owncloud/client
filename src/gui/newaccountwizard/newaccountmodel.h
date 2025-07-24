/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

#include "capabilities.h"
#include "newaccountenums.h"

#include <QObject>
#include <QSet>
#include <QSslCertificate>
#include <QUrl>
#include <QVariantMap>


namespace OCC {
/**
 * @brief The NewAccountModel class is used to store data that is accumulated in the process of creating a new account
 *
 * Our models should just be "dumb containers". Any business logic related to these values needs to be handled in some kind of controller element,
 * in this case the NewAccountWizardController.
 * Business logic does not belong in any gui model!
 */
class NewAccountModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QUrl webfingerAuthenticationUrl READ webfingerAuthenticationUrl WRITE setWebfingerAuthenticationUrl NOTIFY webfingerAuthenticationUrlChanged)
    Q_PROPERTY(QUrl webfingerUserInfoUrl READ webfingerUserInfoUrl WRITE setWebfingerUserInfoUrl NOTIFY webfingerUserInfoUrlChanged)
    Q_PROPERTY(QSet<QSslCertificate> trustedCertificates READ trustedCertificates WRITE setTrustedCertificates NOTIFY trustedCertificatesChanged)
    Q_PROPERTY(QString syncRootDir READ syncRootDir WRITE setSyncRootDir NOTIFY syncRootDirChanged)

    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString davUser READ davUser WRITE setDavUser NOTIFY davUserChanged)

    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(QString refreshToken READ refreshToken WRITE setRefreshToken NOTIFY refreshTokenChanged)

    Q_PROPERTY(QString defaultSyncRoot READ defaultSyncRoot WRITE setDefaultSyncRoot NOTIFY defaultSyncRootChanged)
    Q_PROPERTY(NewAccount::SyncType syncType READ syncType WRITE setSyncType NOTIFY syncTypeChanged)

    Q_PROPERTY(Capabilities capabilities READ capabilities WRITE setCapabilities NOTIFY capabilitiesChanged)

public:
    NewAccountModel(QObject *parent);

    /** The serverUrl is the first url we process in the account wizard. It may be provided by the user or the theme.
     *  If this url supports webfinger, it is used get the authentication url as well as the user info endpoint url which becomes the de facto
     *  url for all user related information and activities.
     *  If the url does not support webfinger, this is an all purpose url used for both authentication and all user related activities.
     */
    QUrl serverUrl() const;
    void setServerUrl(const QUrl &newServerUrl);

    /** The webfingerAuthenticationUrl is the url used for authentication if webfinger is in play.
     *  It is used for authentication only! (eg as the url argument for the initial oauth routine)
     */
    QUrl webfingerAuthenticationUrl() const;
    void setWebfingerAuthenticationUrl(const QUrl &newWebfingerAuthenticationUrl);

    /** the webfingerUserInfoUrl is the effective url passed to the account if webfinger lookup retrieved a non-empty list of user urls
     *  If the list is non-empty, we always take the first element in the list as the webfingerUserInfoUrl, and when the account is built,
     *  this becomes the effective url for the account.
     */
    QUrl webfingerUserInfoUrl() const;
    void setWebfingerUserInfoUrl(const QUrl &newWebfingerUserUrl);

    /** certificates may be provided by the server during the authentication routine. If they are provided the user must accept the certificates
     *  and we store them for future use on that server/account.
     *  If the server provides certificates and the user rejects them, the account creation cannot continue.
     */
    QSet<QSslCertificate> trustedCertificates() const;
    void setTrustedCertificates(const QSet<QSslCertificate> &newTrustedCertificates);

    /** the syncRootDir is the local directory which will host all folder/space subdirectories */
    QString syncRootDir() const;
    void setSyncRootDir(const QString &newSyncRootDir);

    QString displayName() const;
    void setDisplayName(const QString &newDisplayName);

    // convenience function that returns the webfingerAuthenticationUrl if it is non-empty, else it returns the serverUrl
    QUrl effectiveAuthenticationServerUrl() const;
    // convenience function that returns the webfingerUserInfoUrl if it is nonempty, else it returns the serverUrl
    QUrl effectiveUserInfoUrl() const;

    QString davUser() const;
    void setDavUser(const QString &newDavUser);

    QString authToken() const;
    void setAuthToken(const QString &newAuthToken);

    QString refreshToken() const;
    void setRefreshToken(const QString &newRefreshToken);

    Capabilities capabilities() const;
    void setCapabilities(const Capabilities &newCapabilities);

    QString defaultSyncRoot() const;
    void setDefaultSyncRoot(const QString &newDefaultSyncRoot);

    NewAccount::SyncType syncType() const;
    void setSyncType(NewAccount::SyncType newSyncType);

Q_SIGNALS:
    void serverUrlChanged(const QUrl &newUrl);
    void webfingerAuthenticationUrlChanged(const QUrl &newWebfingerAuthenticationUrl);
    void webfingerUserInfoUrlChanged(const QUrl &newWebfingerUserUrl);
    void trustedCertificatesChanged(QSet<QSslCertificate> trustedCertificates);
    void syncRootDirChanged(QString syncRootDir);
    void displayNameChanged(QString displayName);
    void davUserChanged(QString davUser);
    void authTokenChanged(QString authToken);
    void refreshTokenChanged(QString refreshToken);
    void capabilitiesChanged(OCC::Capabilities capabilities);
    void defaultSyncRootChanged(QString defaultSyncRoot);

    void syncTypeChanged(NewAccount::SyncType syncType);

private:
    QUrl _serverUrl;
    QUrl _webfingerAuthenticationUrl;
    QUrl _webfingerUserInfoUrl;
    QSet<QSslCertificate> _trustedCertificates;
    QString _syncRootDir;
    QString _displayName;
    QString _davUser;
    QString _authToken;
    QString _refreshToken;
    Capabilities _capabilities{QUrl(), {}};
    QString _defaultSyncRoot;
    NewAccount::SyncType _syncType;
};
}
