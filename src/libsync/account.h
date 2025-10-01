/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
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


#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include "common/utility.h"

#include "appprovider.h"
#include "capabilities.h"
#include "jobqueue.h"

#include <QByteArray>
#include <QGradient>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QPixmap>
#include <QSharedPointer>
#include <QSslCertificate>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QUrl>
#include <QUuid>
#include <QtQmlIntegration/QtQmlIntegration>

class QSettings;
class QNetworkReply;
class QUrl;
class AccessManager;

namespace OCC {

class CredentialManager;
class AbstractCredentials;
class Account;
typedef QSharedPointer<Account> AccountPtr;
class AccessManager;
class SimpleNetworkJob;

namespace GraphApi {
    class SpacesManager;
}

class ResourcesCache;

/**
 * @brief The Account class represents an account on an ownCloud Server
 * @ingroup libsync
 *
 * The Account has a name and url. It also has information about credentials,
 * SSL errors and certificates.
 */
class OWNCLOUDSYNC_EXPORT Account : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUuid uid READ uuid CONSTANT)
    Q_PROPERTY(QString davUser READ davUser CONSTANT)
    Q_PROPERTY(QString davDisplayName READ davDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString displayNameWithHost READ displayNameWithHost NOTIFY displayNameChanged)
    Q_PROPERTY(QString initials READ initials NOTIFY displayNameChanged)
    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(bool hasAvatar READ hasAvatar NOTIFY avatarChanged)
    Q_PROPERTY(QGradient::Preset avatarGradient READ avatarGradient NOTIFY displayNameChanged)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    QML_ELEMENT
    QML_UNCREATABLE("Only created in the C++ code")

public:
    /**
     * Set a custom directory which all accounts created after this call will share to store their cached files in.
     */
    static void setCommonCacheDirectory(const QString &directory);
    static QString commonCacheDirectory();

    // static AccountPtr create(const QUuid &uuid, const QString &user, const QUrl &url);
    Account(const QUuid &uuid, const QString &user, const QUrl &url, QObject *parent = nullptr);
    ~Account() override;

    void cleanupForRemoval();

    AccountPtr sharedFromThis();

    /**
     *  The unique identifier for the account.
     *  This value is immutable after construction
     */
    QUuid uuid() const;

    /**
     * The user associated with the account.
     * This value is immutable after construction.
     *
     */
    QString davUser() const;

    /**
     * url is the user endpoint for the account
     * This value is immutable after construction.
     */
    QUrl url() const;

    /***
     * This is the default folder containing all spaces.
     */
    QString defaultSyncRoot() const;

    /***
     * Whether we have defaultSyncRoot defined.
     */
    // todo: #43
    bool hasDefaultSyncRoot() const;

    /***
     * Set defaultSyncRoot and creates the path on the filesystem.
     * Setting an empty string will have no effect.
     */
    void setDefaultSyncRoot(const QString &syncRoot);

    QString davDisplayName() const;
    void setDavDisplayName(const QString &newDisplayName);

    QIcon avatar() const;
    void setAvatar(const QIcon &img);
    bool hasAvatar() const;

    /// The name of the account as shown in the toolbar
    QString displayNameWithHost() const;
    QString initials() const;
    QGradient::Preset avatarGradient() const;

    /// The value used to group the account's setttings
    QString groupIndex() const;

    QString hostName() const;

    /**
     * @brief The possibly themed dav path for the account. It has
     *        a trailing slash.
     * @returns the (themeable) dav path for the account.
     */
    QString davPath() const;

    /** Returns WebDAV entry URL, based on url() */
    QUrl davUrl() const;

    /** Holds the accounts credentials */
    AbstractCredentials *credentials() const;
    void setCredentials(AbstractCredentials *cred);

    /** Create a network request on the account's QNAM.
     *
     * Network requests in AbstractNetworkJobs are created through
     * this function. Other places should prefer to use jobs or
     * sendRequest().
     */
    QNetworkReply *sendRawRequest(const QByteArray &verb,
        const QUrl &url,
        QNetworkRequest req = QNetworkRequest(),
        QIODevice *data = nullptr);

    /** The certificates of the account */
    QSet<QSslCertificate> approvedCerts() const { return _approvedCerts; }

    /***
     * Warning calling those will break running network jobs on the current access manager
     */
    void setApprovedCerts(const QSet<QSslCertificate> &certs);

    /***
     * Warning calling those will break running network jobs on the current access manager
     */
    void addApprovedCerts(const QSet<QSslCertificate> &certs);

    /** Access the server capabilities */
    const Capabilities &capabilities() const;
    void setCapabilities(const Capabilities &caps);

    bool hasCapabilities() const;

    void setAppProvider(AppProvider &&p);
    const AppProvider &appProvider() const;

    enum class ServerSupportLevel {
        Supported,
        Unknown,
        Unsupported
    };
    Q_ENUMS(ServerSupportLevel)
    ServerSupportLevel serverSupportLevel() const;

    /** True when the server connection is using HTTP2  */
    bool isHttp2Supported() { return _http2Supported; }
    void setHttp2Supported(bool value) { _http2Supported = value; }

    void clearCookieJar();

    AccessManager *accessManager() const;

    JobQueue *jobQueue();

    CredentialManager *credentialManager() const;

    GraphApi::SpacesManager *spacesManager() const { return _spacesManager; }

    /**
     * We encountered an authentication error.
     */
    void invalidCredentialsEncountered();

    ResourcesCache *resourcesCache() const;

public Q_SLOTS:
    /// Used when forgetting credentials
    void clearAMCache();

Q_SIGNALS:
    /// Triggered by invalidCredentialsEncountered()
    // this signal is emited when a network job failed due to invalid credentials
    void invalidCredentials(QPrivateSignal);

    void credentialsFetched();

    // e.g. when the approved SSL certificates changed
    // todo: #15
    void wantsAccountSaved(Account *acc);

    void serverVersionChanged();

    void avatarChanged();
    void displayNameChanged();

    void unknownConnectionState();

    // the signal exists on the Account object as the Approvider itself can change during runtime
    void appProviderErrorOccured(const QString &error);

private:
    // directory all newly created accounts store their various caches in
    static QString _customCommonCacheDirectory;

    void setSharedThis(AccountPtr sharedThis);

    QWeakPointer<Account> _sharedThis;
    QString _groupIndex;
    QUuid _uuid;
    QString _davUser;
    QString _displayName;
    QString _defaultSyncRoot;
    QIcon _avatarImg;
    
    QUrl _url;
    QString _cacheDirectory;

    QSet<QSslCertificate> _approvedCerts;
    Capabilities _capabilities;
    QPointer<AccessManager> _am;
    QPointer<QNetworkDiskCache> _networkCache = nullptr;
    QPointer<ResourcesCache> _resourcesCache;
    QPointer<AbstractCredentials> _credentials;
    bool _http2Supported = false;

    JobQueue _jobQueue;
    JobQueueGuard _queueGuard;
    CredentialManager *_credentialManager;
    AppProvider _appProvider;

    GraphApi::SpacesManager *_spacesManager = nullptr;
    friend class AccountManager;
};
}

Q_DECLARE_METATYPE(OCC::AccountPtr)


QDebug OWNCLOUDSYNC_EXPORT operator<<(QDebug debug, const OCC::Account *job);

#endif //SERVERCONNECTION_H
