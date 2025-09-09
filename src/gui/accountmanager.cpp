/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#include "accountmanager.h"
#include "account.h"
#include "configfile.h"
#include "creds/credentialmanager.h"
#include "creds/credentials.h"
#include "guiutility.h"

#include <theme.h>

#ifdef Q_OS_WIN
#include "common/utility_win.h"
#endif

#include <QDir>
#include <QSettings>

namespace {
auto urlC()
{
    return QStringLiteral("url");
}

auto defaultSyncRootC()
{
    return QStringLiteral("default_sync_root");
}

const QString davUserC()
{
    return QStringLiteral("dav_user");
}

const QString davUserDisplyNameC()
{
    return QStringLiteral("display-name");
}

const QString userUUIDC()
{
    return QStringLiteral("uuid");
}

auto caCertsKeyC()
{
    return QStringLiteral("CaCertificates");
}

auto accountsC()
{
    return QStringLiteral("Accounts");
}

auto capabilitesC()
{
    return QStringLiteral("capabilities");
}
}


namespace OCC {

Q_LOGGING_CATEGORY(lcAccountManager, "gui.account.manager", QtInfoMsg)

AccountManager *AccountManager::instance()
{
    static AccountManager instance;
    return &instance;
}

AccountManager *AccountManager::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(qmlEngine->thread() == AccountManager::instance()->thread());
    QJSEngine::setObjectOwnership(AccountManager::instance(), QJSEngine::CppOwnership);
    return instance();
}

bool AccountManager::restore()
{
    auto settings = ConfigFile::settingsWithGroup(accountsC());
    if (settings->status() != QSettings::NoError) {
        qCWarning(lcAccountManager) << "Could not read settings from" << settings->fileName()
                                    << settings->status();
        return false;
    }

    if (settings->contains("version")) {
        // Migration from pre-7.0:
        settings->remove("version");
    }

    const auto &childGroups = settings->childGroups();
    for (const auto &accountId : childGroups) {
        settings->beginGroup(accountId);
        if (auto acc = loadAccountHelper(*settings)) {
            acc->_id = accountId;
            if (auto accState = AccountState::loadFromSettings(acc, *settings)) {
                addAccountState(std::move(accState));
            }
        }
        settings->endGroup();
    }

    return true;
}
  
AccountPtr AccountManager::createAccount(const NewAccountModel &model)
{
    auto newAccountPtr = Account::create(QUuid::createUuid());

    newAccountPtr->setUrl(model.effectiveUserInfoUrl());
    newAccountPtr->setDavUser(model.davUser());
    newAccountPtr->setDavDisplayName(model.displayName());

    Credentials *creds = new Credentials(model.authToken(), model.refreshToken(), newAccountPtr.get());
    newAccountPtr->setCredentials(creds);

    newAccountPtr->addApprovedCerts(model.trustedCertificates());
    QString syncRoot = model.defaultSyncRoot();
    if (!syncRoot.isEmpty()) {
        newAccountPtr->setDefaultSyncRoot(syncRoot);
        if (!QFileInfo::exists(syncRoot)) {
            OC_ASSERT(QDir().mkpath(syncRoot));
        }
        Utility::markDirectoryAsSyncRoot(syncRoot, newAccountPtr->uuid());
    }

    newAccountPtr->setCapabilities(model.capabilities());

    return newAccountPtr;
}

void AccountManager::save(bool saveCredentials)
{
    for (const auto &acc : std::as_const(_accounts)) {
        saveAccount(acc->account().data(), saveCredentials);
    }

    qCInfo(lcAccountManager) << "Saved all account settings";
}

// todo: DC-112 I think this save creds thing needs to go. we only persist the refresh token and that should be handled by the creds directly
void AccountManager::saveAccount(Account *account, bool saveCredentials)
{
    qCDebug(lcAccountManager) << "Saving account" << account->url().toString();
    auto settings = ConfigFile::settingsWithGroup(accountsC());
    settings->beginGroup(account->id());

    settings->setValue(urlC(), account->_url.toString());
    settings->setValue(davUserC(), account->_davUser);
    settings->setValue(davUserDisplyNameC(), account->_displayName);
    settings->setValue(userUUIDC(), account->uuid());
    if (account->hasCapabilities()) {
        settings->setValue(capabilitesC(), account->capabilities().raw());
    }
    if (account->hasDefaultSyncRoot()) {
        settings->setValue(defaultSyncRootC(), account->defaultSyncRoot());
    }
    if (account->_credentials) {
        if (saveCredentials) {
            // Only persist the credentials if the parameter is set, on migration from 1.8.x
            // we want to save the accounts but not overwrite the credentials
            // (This is easier than asynchronously fetching the credentials from keychain and then
            // re-persisting them)
            account->_credentials->persist();
        }
    }

    // Save accepted certificates.
    settings->beginGroup(QStringLiteral("General"));
    qCInfo(lcAccountManager) << "Saving " << account->approvedCerts().count() << " unknown certs.";
    const auto approvedCerts = account->approvedCerts();
    QByteArray certs;
    for (const auto &cert : approvedCerts) {
        certs += cert.toPem() + '\n';
    }
    if (!certs.isEmpty()) {
        settings->setValue(caCertsKeyC(), certs);
    }
    settings->endGroup();

    // save the account state but only if it actually exists!!!
    AccountState *state = accountState(account->uuid());
    if (state) {
        state->writeToSettings(*settings);
    }
    settings->endGroup();

    settings->sync();
    qCDebug(lcAccountManager) << "Saved account settings, status:" << settings->status();
}

QStringList AccountManager::accountNames() const
{
    QStringList accountNames;
    accountNames.reserve(_accounts.count());
    for (const auto &a : _accounts) {
        accountNames << a->account()->displayNameWithHost();
    }
    accountNames.sort();
    return accountNames;
}

bool AccountManager::accountForLoginExists(const QUrl &url, const QString &davUser) const
{
    for (const auto &state : _accounts) {
        if (state) {
            AccountPtr account = state->account();
            if (account && account->url() == url && account->davUser() == davUser) {
                return true;
            }
        }
    }
    return false;
}

const QList<AccountState *> AccountManager::accounts() const
{
    return _accounts.values();
}

AccountPtr AccountManager::loadAccountHelper(QSettings &settings)
{
    auto urlConfig = settings.value(urlC());
    if (!urlConfig.isValid()) {
        // No URL probably means a corrupted entry in the account settings
        qCWarning(lcAccountManager) << "No URL for account " << settings.group();
        return AccountPtr();
    }
    QUrl url = urlConfig.toUrl();
    QVariantMap capsValue = settings.value(capabilitesC()).value<QVariantMap>();
    Capabilities caps(url, capsValue);
    QUuid uid = settings.value(userUUIDC(), QVariant::fromValue(QUuid::createUuid())).toUuid();
    // if spaces are not enabled, this is an oc10 account = nogo.
    // if the starting caps are not even valid, forget all of it as well
    if (!caps.isValid() || !caps.spacesSupport().enabled) {
        // ignore this account and strip it from the config
        qCWarning(lcAccountManager) << "The capabilities for this account " << urlConfig << " are not supported by this client";
        // this should remove all keys in the group which == the account index
        settings.remove("");
        Q_ASSERT(settings.childKeys().isEmpty());

        // also kill any credentials associated with this account. This makes me very unhappy but the creds are stored in a top level group
        // and I don't want to mess up the group used for the actual account settings so just do a one off instance here. Yes it adds overhead
        // but should happen so rarely I think it's worth the trade-off to safeguard the more important behavior of the account settings, which is
        // ACTUALLY what we should be dealing with.
        // todo: future: refactor the credentials manager to store the creds keys in the account's group! you can't create a credentialsManager without
        // an account so that is where the settings belong to avoid this mess.
        std::unique_ptr<QSettings> credsGroup = ConfigFile::settingsWithGroup("Credentials");
        QStringList credsKeys = credsGroup->allKeys();
        for (const QString &key : std::as_const(credsKeys)) {
            if (key.contains(uid.toString(QUuid::WithoutBraces)))
                credsGroup->remove(key);
        }
        return AccountPtr();
    }

    // 7.0 change - this special handling can be removed in 8.0
    QString supportsSpacesKey = "supportsSpaces";
    if (settings.contains(supportsSpacesKey)) {
        settings.remove(supportsSpacesKey);
    }

    if (settings.contains("version")) {
        // Migration from pre-7.0:
        settings.remove("version");
    }


    auto acc = createAccount(uid);

    acc->setUrl(url);

    acc->_davUser = settings.value(davUserC()).toString();
    acc->_displayName = settings.value(davUserDisplyNameC()).toString();
    acc->setCapabilities(caps);
    acc->setDefaultSyncRoot(settings.value(defaultSyncRootC()).toString());

    acc->setCredentials(new Credentials(acc.get()));

    // now the server cert, it is in the general group
    settings.beginGroup(QStringLiteral("General"));
    const auto certs = QSslCertificate::fromData(settings.value(caCertsKeyC()).toByteArray());
    qCInfo(lcAccountManager) << "Restored: " << certs.count() << " unknown certs.";
    acc->setApprovedCerts({certs.begin(), certs.end()});
    settings.endGroup();

    // 7.0 config settings maintenance: clean up legacy settings related to the old style credentials
    // todo: remove this when we get to 8.0
    QString credsVersion = QStringLiteral("http_CredentialVersion");
    if (settings.contains(credsVersion)) {
        settings.remove(QStringLiteral("user"));
        const QString authTypePrefix = QStringLiteral("http_");
        const auto childKeys = settings.childKeys();
        for (const auto &key : childKeys) {
            if (!key.startsWith(authTypePrefix))
                continue;
            settings.remove(key);
        }
    }

    return acc;
}

AccountState *AccountManager::account(const QString &name)
{
    for (const auto &acc : std::as_const(_accounts)) {
        if (acc->account()->displayNameWithHost() == name) {
            return acc;
        }
    }
    return nullptr;
}

AccountState *AccountManager::accountState(const QUuid uuid)
{
    return _accounts.value(uuid);
}

AccountState *AccountManager::addAccount(const AccountPtr &newAccount)
{
    auto id = newAccount->id();
    if (id.isEmpty() || !isAccountIdAvailable(id)) {
        id = generateFreeAccountId();
    }
    newAccount->_id = id;


    return addAccountState(AccountState::fromNewAccount(newAccount));
}

void AccountManager::deleteAccount(AccountState *account)
{
    // do these notifications asap so anyone trying to use the account stops doing that
    // unfortunately, if we call these early, the "button" for the account is never removed, and if it's the last
    // account (so there will be none now) the wizard doesn't auto-pop to create a new one. Obviously something is out of
    // whack with gui elements holding onto the pointers or who knows what. I leave this here as this should work!
    // Q_EMIT accountRemoved(account);
    // Q_EMIT accountsChanged();

    if (!_accounts.contains(account->account()->uuid())) {
        qDebug() << "why is the account we are trying to remove not in the accounts map?";
        return;
    }

    _accounts.remove(account->account()->uuid());

    if (account->account()->hasDefaultSyncRoot()) {
        Utility::unmarkDirectoryAsSyncRoot(account->account()->defaultSyncRoot());
    }

    // Forget account credentials, cookies
    account->account()->credentials()->forgetSensitiveData();
    account->account()->credentialManager()->clear();

    auto settings = ConfigFile::settingsWithGroup(accountsC());
    settings->remove(account->account()->id());

    // when called this way the gui stuff gets cleaned up eventually, though not as quickly as I would expect. Still it works
    // better than having these calls at the start.
    Q_EMIT accountRemoved(account);
    Q_EMIT accountsChanged();

    account->account()->cleanupForRemoval();

    // it should be ok to hard delete the account state. if this starts causing trouble we should make sure the destructor
    // does everything it should to ensure a clean exit.
    // in this case we really don't want to put the cleanup in the hands of the event loop, as other activities can get in
    // the way, eg the account wizard auto-run
    delete account;

    if (_accounts.isEmpty())
        Q_EMIT lastAccountRemoved();
}

AccountPtr AccountManager::createAccount(const QUuid &uuid)
{
    AccountPtr acc = Account::create(uuid);
    return acc;
}

void AccountManager::shutdown()
{
    const auto accounts = std::move(_accounts);
    for (AccountState *acc : accounts) {
        Q_EMIT accountRemoved(acc);
        delete acc;
    }
}

bool AccountManager::isAccountIdAvailable(const QString &id) const
{
    for (const auto &acc : _accounts) {
        if (acc->account()->id() == id) {
            return false;
        }
    }
    if (_additionalBlockedAccountIds.contains(id))
        return false;
    return true;
}

QString AccountManager::generateFreeAccountId() const
{
    int i = 0;
    while (true) {
        QString id = QString::number(i);
        if (isAccountIdAvailable(id)) {
            return id;
        }
        ++i;
    }
}

AccountState *AccountManager::addAccountState(std::unique_ptr<AccountState> &&accountState)
{
    AccountState *statePtr = accountState.release();
    if (!statePtr) // just bail. I have no idea why this is happening but fine. it's null and not usable
        return statePtr;

    _accounts.insert(statePtr->account()->uuid(), statePtr);

    auto *rawAccount = statePtr->account().get();
    // this slot can't be connected until the account state exists because saveAccount uses the state
    connect(rawAccount, &Account::wantsAccountSaved, this, [rawAccount, this] {
        // persist the account, not the credentials, we don't know whether they are ready yet
        // Refactoring todo: how about we make those two completely different saves? then we can ditch this lambda
        saveAccount(rawAccount, false);
    });

    Q_EMIT accountAdded(statePtr);
    Q_EMIT accountsChanged();

    statePtr->checkConnectivity();

    return statePtr;
}
}
