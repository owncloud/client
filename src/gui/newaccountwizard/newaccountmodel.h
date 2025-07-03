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
    // I can't remember if/how we use this from the oc10 blocker pr, it may be obsolete
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged);
    // todo: OAuth credentials
    // might need authentication type too - decide after DC-49 is merged. in general if the authentication type check is not oauth, the whole account setup
    // needs to fail so I don't see a future for it in this model but let's see.
    // todo: advanced settings props

public:
    NewAccountModel(QObject *parent);

    /** The serverUrl is the first url we process in the account wizard. It may be provided by the user or the theme
     * if webfinger is not in play, this is the effective url that ends up in the Account
     */
    QUrl serverUrl() const;
    void setServerUrl(const QUrl &newServerUrl);

    /** the webfingerAuthenticationUrl is the primary url used for authentication if webfinger is in play
     *  it is used to look up the actual user webfinger url which becomes the effective url for the account
     *  if webfinger is in play
     */
    QUrl webfingerAuthenticationUrl() const;
    void setWebfingerAuthenticationUrl(const QUrl &newWebfingerAuthenticationUrl);

    /** the webfingerUserUrl is the effective url passed to the account if webfinger is in play */
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

    /** I can't remember what we do with the display name so I will update this asap */
    QString displayName() const;
    void setDisplayName(const QString &newDisplayName);

    QUrl effectiveAuthenticationServerUrl() const;

Q_SIGNALS:
    void serverUrlChanged(const QUrl &newUrl);
    void webfingerAuthenticationUrlChanged(const QUrl &newWebfingerAuthenticationUrl);
    void webfingerUserUrlChanged(const QUrl &newWebfingerUserUrl);
    void trustedCertificatesChanged(QSet<QSslCertificate> trustedCertificates);
    void syncRootDirChanged(QString syncRootDir);
    void displayNameChanged(QString displayName);

private:
    QUrl _serverUrl;
    QUrl _webfingerAuthenticationUrl;
    QUrl _webfingerUserUrl;
    QSet<QSslCertificate> _trustedCertificates;
    QString _syncRootDir;
    QString _displayName;
};

}
