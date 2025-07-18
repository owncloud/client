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

QString NewAccountModel::syncRootDir() const
{
    return _syncRootDir;
}

void NewAccountModel::setSyncRootDir(const QString &newSyncRootDir)
{
    if (_syncRootDir == newSyncRootDir)
        return;
    _syncRootDir = newSyncRootDir;
    Q_EMIT syncRootDirChanged(_syncRootDir);
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

QUrl NewAccountModel::effectiveAuthenticationServerUrl() const
{
    if (!_webfingerAuthenticationUrl.isEmpty())
        return _webfingerAuthenticationUrl;
    return _serverUrl;
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
}
