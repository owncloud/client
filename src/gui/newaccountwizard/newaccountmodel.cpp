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

#include "newaccountmodel.h"

namespace OCC {

NewAccountModel::NewAccountModel(QObject *parent)
    : QObject(parent)
{
}

QUrl NewAccountModel::serverUrl() const
{
    return _serverUrl;
}

void NewAccountModel::setServerUrl(const QUrl &newServerUrl)
{
    // consider clearing auth info any time the url changes. it might be overkill though
    if (_serverUrl == newServerUrl)
        return;
    _serverUrl = newServerUrl;
    Q_EMIT serverUrlChanged(_serverUrl);
}

QUrl NewAccountModel::webfingerAuthenticationUrl() const
{
    return _webfingerAuthenticationUrl;
}

void NewAccountModel::setWebfingerAuthenticationUrl(const QUrl &newWebfingerAuthenticationUrl)
{
    if (_webfingerAuthenticationUrl == newWebfingerAuthenticationUrl)
        return;
    _webfingerAuthenticationUrl = newWebfingerAuthenticationUrl;
    Q_EMIT webfingerAuthenticationUrlChanged(_webfingerAuthenticationUrl);
}

QUrl NewAccountModel::webfingerUserInfoUrl() const
{
    return _webfingerUserInfoUrl;
}

void NewAccountModel::setWebfingerUserInfoUrl(const QUrl &newWebfingerUserUrl)
{
    if (_webfingerUserInfoUrl == newWebfingerUserUrl)
        return;
    _webfingerUserInfoUrl = newWebfingerUserUrl;
    Q_EMIT webfingerUserInfoUrlChanged(_webfingerUserInfoUrl);
}

QSet<QSslCertificate> NewAccountModel::trustedCertificates() const
{
    return _trustedCertificates;
}

void NewAccountModel::setTrustedCertificates(const QSet<QSslCertificate> &newTrustedCertificates)
{
    if (_trustedCertificates == newTrustedCertificates)
        return;
    _trustedCertificates = newTrustedCertificates;
    Q_EMIT trustedCertificatesChanged(_trustedCertificates);
}

QString NewAccountModel::displayName() const
{
    return _displayName;
}

void NewAccountModel::setDisplayName(const QString &newDisplayName)
{
    if (_displayName == newDisplayName)
        return;
    _displayName = newDisplayName;
    Q_EMIT displayNameChanged(_displayName);
}

QString NewAccountModel::davUser() const
{
    return _davUser;
}

void NewAccountModel::setDavUser(const QString &newDavUser)
{
    if (_davUser == newDavUser)
        return;
    _davUser = newDavUser;
    Q_EMIT davUserChanged(_davUser);
}

QString NewAccountModel::authToken() const
{
    return _authToken;
}

void NewAccountModel::setAuthToken(const QString &newAuthToken)
{
    if (_authToken == newAuthToken)
        return;
    _authToken = newAuthToken;
    Q_EMIT authTokenChanged(_authToken);
}

QString NewAccountModel::refreshToken() const
{
    return _refreshToken;
}

void NewAccountModel::setRefreshToken(const QString &newRefreshToken)
{
    if (_refreshToken == newRefreshToken)
        return;
    _refreshToken = newRefreshToken;
    Q_EMIT refreshTokenChanged(_refreshToken);
}

Capabilities NewAccountModel::capabilities() const
{
    return _capabilities;
}

void NewAccountModel::setCapabilities(const Capabilities &newCapabilities)
{
    // if (_capabilities == newCapabilities)
    //    return;
    _capabilities = newCapabilities;
    Q_EMIT capabilitiesChanged(_capabilities);
}

QString NewAccountModel::defaultSyncRoot() const
{
    return _defaultSyncRoot;
}

void NewAccountModel::setDefaultSyncRoot(const QString &newDefaultSyncRoot)
{
    if (_defaultSyncRoot != newDefaultSyncRoot) {
        _defaultSyncRoot = newDefaultSyncRoot;
        Q_EMIT defaultSyncRootChanged(_defaultSyncRoot);
    }
}

NewAccount::SyncType NewAccountModel::syncType() const
{
    return _syncType;
}

void NewAccountModel::setSyncType(NewAccount::SyncType newSyncType)
{
    if (_syncType == newSyncType)
        return;
    _syncType = newSyncType;
    Q_EMIT syncTypeChanged(_syncType);
}


QUrl NewAccountModel::effectiveAuthenticationServerUrl() const
{
    if (!_webfingerAuthenticationUrl.isEmpty())
        return _webfingerAuthenticationUrl;
    return _serverUrl;
}

QUrl NewAccountModel::effectiveUserInfoUrl() const
{
    if (!_webfingerUserInfoUrl.isEmpty())
        return _webfingerUserInfoUrl;
    else
        return _serverUrl;
}

bool NewAccountModel::isComplete() const
{
    bool authInfoComplete = !_davUser.isEmpty() && !_authToken.isEmpty() && !_refreshToken.isEmpty();
    bool syncInfoComplete = false;
    if (_syncType != NewAccount::SyncType::NONE) {
        if (_syncType == NewAccount::SyncType::SELECTIVE_SYNC && _defaultSyncRoot.isEmpty())
            syncInfoComplete = true;
        else if ((_syncType == NewAccount::SyncType::USE_VFS || _syncType == NewAccount::SyncType::SYNC_ALL) && !_defaultSyncRoot.isEmpty())
            syncInfoComplete = true;
    }

    bool complete = authInfoComplete && syncInfoComplete && effectiveUserInfoUrl().isValid() && _capabilities.isValid();
    return complete;
}
}
