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

#include "account.h"
#include "accessmanager.h"
#include "capabilities.h"
#include "cookiejar.h"
#include "creds/abstractcredentials.h"
#include "creds/credentialmanager.h"
#include "graphapi/spacesmanager.h"
#include "networkjobs.h"
#include "networkjobs/resources.h"

#include <QAuthenticator>
#include <QDir>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkDiskCache>
#include <QSslKey>
#include <QStandardPaths>

namespace OCC {

Q_LOGGING_CATEGORY(lcAccount, "sync.account", QtInfoMsg)

QString Account::_customCommonCacheDirectory = {};

void Account::setCommonCacheDirectory(const QString &directory)
{
    _customCommonCacheDirectory = directory;
}

QString Account::commonCacheDirectory()
{
    if (_customCommonCacheDirectory.isEmpty()) {
        return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    }

    return _customCommonCacheDirectory;
}

Account::Account(const QUuid &uuid, const QString &user, const QUrl &url, QObject *parent)
    : QObject(parent)
    , _uuid(uuid)
    , _davUser(user)
    , _url(url)
    , _capabilities({}, {})
    , _jobQueue(this)
    , _queueGuard(&_jobQueue)
    , _credentialManager(new CredentialManager(this))
{
    qRegisterMetaType<AccountPtr>("AccountPtr");

    _cacheDirectory = QStringLiteral("%1/accounts/%2").arg(commonCacheDirectory(), _uuid.toString(QUuid::WithoutBraces));
    QDir().mkpath(_cacheDirectory);

    // we need to make sure the directory we pass to the resources cache exists
    const QString resourcesCacheDir = QStringLiteral("%1/resources/").arg(_cacheDirectory);
    if (QFileInfo::exists(resourcesCacheDir)) {
        // Some versions of the client failed to remove the temporary cache directory on exit, and
        // after a crash the directory would not be removed either. This can result in multiple
        // GB of cached space images. So clean this up before creating a new cache.
        // todo: #32
        if (!QDir(resourcesCacheDir).removeRecursively()) {
            qCWarning(lcAccount) << "Resources cache was not fully cleaned";
        }
    }
    QDir().mkpath(resourcesCacheDir);
    _resourcesCache = new ResourcesCache(resourcesCacheDir, this);

    _spacesManager = new GraphApi::SpacesManager(this);
}

/*AccountPtr Account::create(const QUuid &uuid, const QString &user, const QUrl &url)
{
    AccountPtr acc = AccountPtr(new Account(uuid, user, url));
    acc->setSharedThis(acc);
    return acc;
}*/

Account::~Account()
{
}

void Account::cleanupForRemoval()
{
    // Abort any pending jobs: the account credentials are invalidated.
    jobQueue()->clear();

    // Stop the spaces manager, because it might start a new `Drives` job before the account is deleted.
    if (_spacesManager) {
        delete _spacesManager;
        _spacesManager = nullptr;
    }

    // Stop the resource cache (including any pending jobs), because this uses the cache directory.
    delete _resourcesCache;
    _resourcesCache = nullptr;

    if (!QDir(_cacheDirectory).removeRecursively()) {
        qCWarning(lcAccount) << "Cache directory" << _cacheDirectory << "was not fully removed";
    }
}

QString Account::davPath() const
{
    return QLatin1String("/remote.php/dav/files/") + davUser() + QLatin1Char('/');
}

// todo: #20. I'm speechless
void Account::setSharedThis(AccountPtr sharedThis)
{
    _sharedThis = sharedThis.toWeakRef();
}

CredentialManager *Account::credentialManager() const
{
    return _credentialManager;
}

QUuid Account::uuid() const
{
    return _uuid;
}

// todo: #20
AccountPtr Account::sharedFromThis()
{
    return _sharedThis.toStrongRef();
}

QString Account::davUser() const
{
    return _davUser;
}

QIcon Account::avatar() const
{
    return _avatarImg;
}

void Account::setAvatar(const QIcon &img)
{
    _avatarImg = img;
    Q_EMIT avatarChanged();
}

bool Account::hasAvatar() const
{
    return !_avatarImg.isNull();
}

QString Account::displayNameWithHost() const
{
    QString user = davDisplayName();
    QString host = _url.host();
    const int port = url().port();
    if (port > 0 && port != 80 && port != 443) {
        host += QStringLiteral(":%1").arg(QString::number(port));
    }
    return tr("%1@%2").arg(user, host);
}

QString Account::initials() const
{
    QString out;
    for (const auto &p : davDisplayName().split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        out.append(p.first(1));
    }
    return out;
}

QGradient::Preset Account::avatarGradient() const
{
    return static_cast<QGradient::Preset>(qHash(displayNameWithHost()) % QGradient::NumPresets + 1);
}

QString Account::davDisplayName() const
{
    if (_displayName.isEmpty()) {
        return davUser();
    }
    return _displayName;
}

void Account::setDavDisplayName(const QString &newDisplayName)
{
    if (_displayName != newDisplayName) {
        _displayName = newDisplayName;
        Q_EMIT displayNameChanged();
    }
}

QString Account::groupIndex() const
{
    return _groupIndex;
}

AbstractCredentials *Account::credentials() const
{
    return _credentials;
}

// todo: in a sane world the credentials should be instantiated by the account (as parent) as using this setter there isthe chance that it could be set or unset
// randomly or even multiple times like this is really not good. For now we have to pass the creds in as the tests use their own subclass of AbstractCredentials
// = FakeCredentials, so that has to be worked out before we can move this creds handling into the account proper, where it should be.
void Account::setCredentials(AbstractCredentials *cred)
{
    if (!cred || _credentials == cred)
        return;

    // keep any cookies for new nam
    QNetworkCookieJar *jar = nullptr;
    if (_am) {
        jar = _am->cookieJar();
        jar->setParent(nullptr);
    }
    // get rid of the old credentials if they exist - imo this should never happen but who knows
    // note the access manager is parented by the creds so let that take care of deleting the am, but verify it's gone
    if (_credentials) {
        delete _credentials;
        _credentials = nullptr;
        Q_ASSERT(_am == nullptr);
    }

    _credentials = cred;
    _am = _credentials->createAccessManager();
    if (jar) {
        _am->setCookieJar(jar);
    }

    // the network access manager takes ownership when setCache is called, so we have to reinitialize it every time we reset the manager
    _networkCache = new QNetworkDiskCache(this);
    const QString networkCacheLocation = (QStringLiteral("%1/network/").arg(_cacheDirectory));
    _networkCache->setCacheDirectory(networkCacheLocation);
    _am->setCache(_networkCache);

    connect(_credentials, &AbstractCredentials::fetched, this, [this] {
        Q_EMIT credentialsFetched();
        _queueGuard.unblock();
    });
    connect(_credentials, &AbstractCredentials::authenticationStarted, this, [this] { _queueGuard.block(); });
    connect(_credentials, &AbstractCredentials::authenticationFailed, this, [this] { _queueGuard.clear(); });
}

QUrl Account::davUrl() const
{
    return Utility::concatUrlPath(url(), davPath());
}

/**
 * clear all cookies. (Session cookies or not)
 */
void Account::clearCookieJar()
{
    qCInfo(lcAccount) << "Clearing cookies";
    _am->cookieJar()->deleteLater();
    _am->setCookieJar(new CookieJar);
}

AccessManager *Account::accessManager() const
{
    return _am;
}

QNetworkReply *Account::sendRawRequest(const QByteArray &verb, const QUrl &url, QNetworkRequest req, QIODevice *data)
{
    Q_ASSERT(verb.isUpper());
    req.setUrl(url);
    if (verb == "HEAD" && !data) {
        return _am->head(req);
    } else if (verb == "GET" && !data) {
        return _am->get(req);
    } else if (verb == "POST") {
        return _am->post(req, data);
    } else if (verb == "PUT") {
        return _am->put(req, data);
    } else if (verb == "DELETE" && !data) {
        return _am->deleteResource(req);
    }
    return _am->sendCustomRequest(req, verb, data);
}

void Account::setApprovedCerts(const QSet<QSslCertificate> &certs)
{
    _approvedCerts = certs;
    _am->setCustomTrustedCaCertificates(_approvedCerts);
}

void Account::addApprovedCerts(const QSet<QSslCertificate> &certs)
{
    _approvedCerts.unite(certs);
    _am->setCustomTrustedCaCertificates(_approvedCerts);
    Q_EMIT wantsAccountSaved(this);
}

QUrl Account::url() const
{
    return _url;
}

QString Account::hostName() const
{
    return _url.host();
}

JobQueue *Account::jobQueue()
{
    return &_jobQueue;
}

void Account::clearAMCache()
{
    _am->clearAccessCache();
}

const Capabilities &Account::capabilities() const
{
    return _capabilities;
}

bool Account::hasCapabilities() const
{
    return _capabilities.isValid();
}

void Account::setCapabilities(const Capabilities &caps)
{
    if (!caps.isValid())
        return;

    // this should not happen:
    if (!caps.spacesSupport().enabled) {
        qCWarning(lcAccount) << "trying to set capabilities instance that doesn't support spaces - aborting";
        return;
    }

    const bool versionChanged =
        caps.status().legacyVersion != _capabilities.status().legacyVersion || caps.status().productversion != _capabilities.status().productversion;
    _capabilities = caps;
    if (versionChanged) {
        Q_EMIT serverVersionChanged();
    }
}

Account::ServerSupportLevel Account::serverSupportLevel() const
{
    if (!hasCapabilities()) {
        // not detected yet
        // should only happen when reloading an account from config?
        return ServerSupportLevel::Unknown;
    }

    // todo: #34
    // ocis and kiteworks allegedly
    // this seems awfully loosey goosey but I've been told it should work
    if (!capabilities().status().productversion.isEmpty()) {
        return ServerSupportLevel::Supported;
    }

    return ServerSupportLevel::Unsupported;
}

QString Account::defaultSyncRoot() const
{
    return _defaultSyncRoot;
}
// todo: #43 this seems to be used as a quasi switch for filtering oc10 from spaces support but needs more investigation
// if we need to eg update the sync root because it is not yet set, use defaultSyncRoot.isEmpty, not this thing.
bool Account::hasDefaultSyncRoot() const
{
    return !_defaultSyncRoot.isEmpty();
}

void Account::setDefaultSyncRoot(const QString &syncRoot)
{
    Q_ASSERT(_defaultSyncRoot.isEmpty());
    if (!syncRoot.isEmpty()) {
        _defaultSyncRoot = syncRoot;
    }
}

void Account::setAppProvider(AppProvider &&p)
{
    _appProvider = std::move(p);
}

const AppProvider &Account::appProvider() const
{
    return _appProvider;
}

void Account::invalidCredentialsEncountered()
{
    Q_EMIT invalidCredentials(Account::QPrivateSignal());
}

ResourcesCache *Account::resourcesCache() const
{
    return _resourcesCache;
}

} // namespace OCC


QDebug operator<<(QDebug debug, const OCC::Account *acc)
{
    QDebugStateSaver saver(debug);
    debug.setAutoInsertSpaces(false);
    debug << "OCC::Account(" << acc->displayNameWithHost() << ")";
    return debug.maybeSpace();
}
