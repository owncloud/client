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

QUrl NewAccountModel::webfingerUserUrl() const
{
    return _webfingerUserUrl;
}

void NewAccountModel::setWebfingerUserUrl(const QUrl &newWebfingerUserUrl)
{
    if (_webfingerUserUrl == newWebfingerUserUrl)
        return;
    _webfingerUserUrl = newWebfingerUserUrl;
    Q_EMIT webfingerUserUrlChanged(_webfingerUserUrl);
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

}
