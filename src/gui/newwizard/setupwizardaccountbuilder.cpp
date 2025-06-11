/*
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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

#include "setupwizardaccountbuilder.h"

#include "gui/guiutility.h"

#include <QDir>
#include <QFileInfo>

namespace OCC::Wizard {

AbstractAuthenticationStrategy::~AbstractAuthenticationStrategy() { }

QString AbstractAuthenticationStrategy::davUser()
{
    return _davUser;
}

void AbstractAuthenticationStrategy::setDavUser(const QString &user)
{
    _davUser = user;
}

OAuthAuthenticationStrategy::OAuthAuthenticationStrategy(const QString &token, const QString &refreshToken)
    : _token(token)
    , _refreshToken(refreshToken)
{
}

HttpCredentialsGui *OAuthAuthenticationStrategy::makeCreds()
{
    Q_ASSERT(isValid());
    return new HttpCredentialsGui(davUser(), _token, _refreshToken);
}

bool OAuthAuthenticationStrategy::isValid()
{
    return !davUser().isEmpty() && !_token.isEmpty() && !_refreshToken.isEmpty();
}

FetchUserInfoJobFactory OAuthAuthenticationStrategy::makeFetchUserInfoJobFactory(QNetworkAccessManager *nam)
{
    return FetchUserInfoJobFactory::fromOAuth2Credentials(nam, _token);
}

void SetupWizardAccountBuilder::setServerUrl(const QUrl &serverUrl, AuthenticationType authType)
{
    _serverUrl = serverUrl;
    _authType = authType;

    // to not keep credentials longer than necessary, we purge them whenever the URL is set
    // for this reason, we also don't insert already-known credentials on the credentials pages when switching to them
    _authenticationStrategy.reset();
}

QUrl SetupWizardAccountBuilder::serverUrl() const
{
    return _serverUrl;
}


AccountPtr SetupWizardAccountBuilder::build()
{
    auto newAccountPtr = Account::create(QUuid::createUuid());

    Q_ASSERT(!_serverUrl.isEmpty() && _serverUrl.isValid());
    newAccountPtr->setUrl(_serverUrl);

    if (!_webFingerSelectedInstance.isEmpty()) {
        Q_ASSERT(_serverUrl.isValid());
        newAccountPtr->setUrl(_webFingerSelectedInstance);
    }

    Q_ASSERT(hasValidCredentials());

    // TODO: perhaps _authenticationStrategy->setUpAccountPtr(...) would be more elegant? no need for getters then
    newAccountPtr->setDavUser(_authenticationStrategy->davUser());
    newAccountPtr->setCredentials(_authenticationStrategy->makeCreds());

    newAccountPtr->setDavDisplayName(_displayName);

    newAccountPtr->addApprovedCerts({ _customTrustedCaCertificates.begin(), _customTrustedCaCertificates.end() });

    if (!_defaultSyncTargetDir.isEmpty()) {
        newAccountPtr->setDefaultSyncRoot(_defaultSyncTargetDir);
        if (!QFileInfo::exists(_defaultSyncTargetDir)) {
            OC_ASSERT(QDir().mkpath(_defaultSyncTargetDir));
        }
        Utility::markDirectoryAsSyncRoot(_defaultSyncTargetDir, newAccountPtr->uuid());
    }

    return newAccountPtr;
}

bool SetupWizardAccountBuilder::hasValidCredentials() const
{
    if (_authenticationStrategy == nullptr) {
        return false;
    }

    return _authenticationStrategy->isValid();
}

QString SetupWizardAccountBuilder::displayName() const
{
    return _displayName;
}

void SetupWizardAccountBuilder::setDisplayName(const QString &displayName)
{
    _displayName = displayName;
}

void SetupWizardAccountBuilder::setAuthenticationStrategy(AbstractAuthenticationStrategy *strategy)
{
    _authenticationStrategy.reset(strategy);
}

void SetupWizardAccountBuilder::addCustomTrustedCaCertificate(const QSslCertificate &customTrustedCaCertificate)
{
    _customTrustedCaCertificates.insert(customTrustedCaCertificate);
}

void SetupWizardAccountBuilder::clearCustomTrustedCaCertificates()
{
    _customTrustedCaCertificates.clear();
}

AbstractAuthenticationStrategy *SetupWizardAccountBuilder::authenticationStrategy() const
{
    return _authenticationStrategy.get();
}

void SetupWizardAccountBuilder::setSyncTargetDir(const QString &syncTargetDir)
{
    _defaultSyncTargetDir = syncTargetDir;
}

QString SetupWizardAccountBuilder::syncTargetDir() const
{
    return _defaultSyncTargetDir;
}

void SetupWizardAccountBuilder::setDynamicRegistrationData(const QVariantMap &dynamicRegistrationData)
{
    _dynamicRegistrationData = dynamicRegistrationData;
}

QVariantMap SetupWizardAccountBuilder::dynamicRegistrationData() const
{
    return _dynamicRegistrationData;
}

void SetupWizardAccountBuilder::setWebFingerAuthenticationServerUrl(const QUrl &url)
{
    _webFingerAuthenticationServerUrl = url;
    _authType = AuthenticationType::OAuth;
}

QUrl SetupWizardAccountBuilder::webFingerAuthenticationServerUrl() const
{
    return _webFingerAuthenticationServerUrl;
}

void SetupWizardAccountBuilder::setWebFingerInstances(const QVector<QUrl> &instancesList)
{
    _webFingerInstances = instancesList;
}

QVector<QUrl> SetupWizardAccountBuilder::webFingerInstances() const
{
    return _webFingerInstances;
}

void SetupWizardAccountBuilder::setWebFingerSelectedInstance(const QUrl &instance)
{
    _webFingerSelectedInstance = instance;
}

QUrl SetupWizardAccountBuilder::webFingerSelectedInstance() const
{
    return _webFingerSelectedInstance;
}
}
