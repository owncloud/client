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

    Q_PROPERTY(QUrl serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged);
    Q_PROPERTY(QUrl webfingerAuthenticationUrl READ webfingerAuthenticationUrl WRITE setWebfingerAuthenticationUrl NOTIFY webfingerAuthenticationUrlChanged);
    Q_PROPERTY(QUrl webfingerUserUrl READ webfingerUserUrl WRITE setWebfingerUserUrl NOTIFY webfingerUserUrlChanged);
    Q_PROPERTY(QSet<QSslCertificate> trustedCertificates READ trustedCertificates WRITE setTrustedCertificates NOTIFY trustedCertificatesChanged);
    Q_PROPERTY(QString syncRootDir READ syncRootDir WRITE setSyncRootDir NOTIFY syncRootDirChanged);

    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged);
    Q_PROPERTY(QString davUser READ davUser WRITE setDavUser NOTIFY davUserChanged);

    Q_PROPERTY(QString authToken READ authToken WRITE setAuthToken NOTIFY authTokenChanged);
    Q_PROPERTY(QString refreshToken READ refreshToken WRITE setRefreshToken NOTIFY refreshTokenChanged);

    Q_PROPERTY(Capabilities capabilities READ capabilities WRITE setCapabilities NOTIFY capabilitiesChanged);

public:
    NewAccountModel(QObject *parent);

    /** The serverUrl is the first url we process in the account wizard. It may be provided by the user or the theme
     * If no webfinger user urls are found, this is the effective url that ends up in the Account
     */
    QUrl serverUrl() const;
    void setServerUrl(const QUrl &newServerUrl);

    /** The webfingerAuthenticationUrl is the primary url used for authentication if webfinger is in play.
     *  It is used to look up the actual user webfinger urls. The list may be empty even if we have a webfinger authentication url,
     *  eg with a stock ocis server. This url does not become part of the new account, it is only needed for the authentication process.
     */
    QUrl webfingerAuthenticationUrl() const;
    void setWebfingerAuthenticationUrl(const QUrl &newWebfingerAuthenticationUrl);

    /** the webfingerUserUrl is the effective url passed to the account if webfinger lookup retrieved a non-empty list of user urls
     *  if the list is non-empty, we always take the first element in the list as the webfingerUserUrl, and when the account is built,
     *  this becomes the effective url for the account.
     */
    QUrl webfingerUserUrl() const;
    void setWebfingerUserUrl(const QUrl &newWebfingerUserUrl);

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

    QString davUser() const;
    void setDavUser(const QString &newDavUser);

    QString authToken() const;
    void setAuthToken(const QString &newAuthToken);

    QString refreshToken() const;
    void setRefreshToken(const QString &newRefreshToken);

    Capabilities capabilities() const;
    void setCapabilities(const Capabilities &newCapabilities);

Q_SIGNALS:
    void serverUrlChanged(const QUrl &newUrl);
    void webfingerAuthenticationUrlChanged(const QUrl &newWebfingerAuthenticationUrl);
    void webfingerUserUrlChanged(const QUrl &newWebfingerUserUrl);
    void trustedCertificatesChanged(QSet<QSslCertificate> trustedCertificates);
    void syncRootDirChanged(QString syncRootDir);
    void displayNameChanged(QString displayName);
    void davUserChanged(QString davUser);
    void authTokenChanged(QString authToken);
    void refreshTokenChanged(QString refreshToken);

    void capabilitiesChanged(OCC::Capabilities capabilities);

private:
    QUrl _serverUrl;
    QUrl _webfingerAuthenticationUrl;
    QUrl _webfingerUserUrl;
    QSet<QSslCertificate> _trustedCertificates;
    QString _syncRootDir;
    QString _displayName;
    QString _davUser;
    QString _authToken;
    QString _refreshToken;
    Capabilities _capabilities{QUrl(), {}};
};
}
