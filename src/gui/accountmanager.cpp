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

// todo: move these to static const instances in the class once the tests are fixed to allow qstring from ascii
namespace {
QString urlC()
{
    return "url";
}

QString defaultSyncRootC()
{
    return "default_sync_root";
}

QString davUserC()
{
    return "dav_user";
}

QString davUserDisplyNameC()
{
    return "display-name";
}

QString userUUIDC()
{
    return "uuid";
}

QString caCertsKeyC()
{
    return "CaCertificates";
}

QString accountsC()
{
    return "Accounts";
}

QString capabilitesC()
{
    return "capabilities";
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
    for (const auto &accountIndex : childGroups) {
        settings->beginGroup(accountIndex);
        if (auto acc = loadAccountHelper(*settings)) {
            acc->_groupIndex = accountIndex;
            AccountState *accState = new AccountState(acc);
            accState->loadSettings(*settings);
            addAccountState(accState);
        }
        settings->endGroup();
    }

    return true;
}

Account *AccountManager::createAccount(const NewAccountModel &model)
{
    auto account = new Account(QUuid::createUuid(), model.davUser(), model.effectiveUserInfoUrl());

    account->setDavDisplayName(model.displayName());

    Credentials *creds = new Credentials(model.authToken(), model.refreshToken(), account);
    account->setCredentials(creds);

    account->addApprovedCerts(model.trustedCertificates());
    QString syncRoot = model.defaultSyncRoot();
    if (!syncRoot.isEmpty()) {
        account->setDefaultSyncRoot(syncRoot);
        if (!QFileInfo::exists(syncRoot)) {
            OC_ASSERT(QDir().mkpath(syncRoot));
        }
        Utility::markDirectoryAsSyncRoot(syncRoot, account->uuid());
    }

    account->setCapabilities(model.capabilities());

    return account;
}

void AccountManager::save()
{
    for (const auto &acc : std::as_const(_accounts)) {
        saveAccount(acc->account());
    }

    qCInfo(lcAccountManager) << "Saved all account settings";
}

void AccountManager::saveAccount(Account *account)
{
    if (!account)
        return;

    qCDebug(lcAccountManager) << "Saving account" << account->url().toString();
    auto settings = ConfigFile::settingsWithGroup(accountsC());
    settings->beginGroup(account->groupIndex());

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

    // Save accepted certificates.
    settings->beginGroup("General");
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
        if (state && state->account()) {
            Account *account = state->account();
            if (account->url() == url && account->davUser() == davUser) {
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

Account *AccountManager::loadAccountHelper(QSettings &settings)
{
    QVariant urlConfig = settings.value(urlC());
    if (!urlConfig.isValid()) {
        // No URL probably means a corrupted entry in the account settings
        qCWarning(lcAccountManager) << "No URL for account " << settings.group();
        return nullptr;
    }

    QString user = settings.value(davUserC()).toString();
    if (user.isEmpty()) {
        qCWarning(lcAccountManager) << "No user name provided for account " << settings.group();
        return nullptr;
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
        return nullptr;
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


    auto acc = new Account(uid, user, url);

    acc->setDavDisplayName(settings.value(davUserDisplyNameC()).toString());
    acc->setCapabilities(caps);
    acc->setDefaultSyncRoot(settings.value(defaultSyncRootC()).toString());

    acc->setCredentials(new Credentials(acc));

    // now the server cert, it is in the general group
    settings.beginGroup("General");
    const auto certs = QSslCertificate::fromData(settings.value(caCertsKeyC()).toByteArray());
    qCInfo(lcAccountManager) << "Restored: " << certs.count() << " unknown certs.";
    acc->setApprovedCerts({certs.begin(), certs.end()});
    settings.endGroup();

    // 7.0 config settings maintenance: clean up legacy settings related to the old style credentials
    // todo: remove this when we get to 8.0
    QString credsVersion = "http_CredentialVersion";
    if (settings.contains(credsVersion)) {
        settings.remove("user");
        const QString authTypePrefix = "http_";
        const auto childKeys = settings.childKeys();
        for (const auto &key : childKeys) {
            if (key.startsWith(authTypePrefix))
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

AccountState *AccountManager::addAccount(Account *newAccount)
{
    auto id = newAccount->groupIndex();
    if (id.isEmpty() || !isAccountIndexAvailable(id)) {
        id = generateFreeAccountIndex();
    }
    newAccount->_groupIndex = id;


    return addAccountState(new AccountState(newAccount));
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

    // todo: DC-150 try moving these to the account::cleanupForRemoval routine. I'm worried that calling these
    // after the accountRemoved/accountsChanged signals could cause problems, as we had when I put those notifications
    // at the start
    // Forget account credentials, cookies
    account->account()->credentials()->forgetSensitiveData();
    account->account()->credentialManager()->clear();

    auto settings = ConfigFile::settingsWithGroup(accountsC());
    settings->remove(account->account()->groupIndex());

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

void AccountManager::shutdown()
{
    const auto accounts = std::move(_accounts);
    for (AccountState *acc : accounts) {
        Q_EMIT accountRemoved(acc);
        delete acc;
    }
}

bool AccountManager::isAccountIndexAvailable(const QString &index) const
{
    for (const auto &acc : _accounts) {
        if (acc->account()->groupIndex() == index) {
            return false;
        }
    }

    return true;
}

QString AccountManager::generateFreeAccountIndex() const
{
    int i = 0;
    while (true) {
        QString index = QString::number(i);
        if (isAccountIndexAvailable(index)) {
            return index;
        }
        ++i;
    }
}

AccountState *AccountManager::addAccountState(AccountState *accountState)
{
    if (!accountState || !accountState->account()) // just bail. I have no idea why this is happening but fine. it's null and not usable
        return nullptr;

    _accounts.insert(accountState->account()->uuid(), accountState);

    Account *acc = accountState->account();

    // this slot can't be connected until the account state exists because saveAccount uses the state
    connect(acc, &Account::wantsAccountSaved, this, [acc, this] { saveAccount(acc); });

    Q_EMIT accountAdded(accountState);
    Q_EMIT accountsChanged();
    accountState->checkConnectivity();

    return accountState;
}
}
