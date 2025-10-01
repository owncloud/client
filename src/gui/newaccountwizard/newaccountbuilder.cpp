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
#include "newaccountbuilder.h"
#include "account.h"
#include "accountmanager.h"
#include "accountsettings.h"
#include "accountstate.h"


namespace OCC {

NewAccountBuilder::NewAccountBuilder(const NewAccountModel &model, QObject *parent)
    : QObject{parent}
{
    _account = AccountManager::instance()->createAccount(model);
    _syncType = model.syncType();
}

void NewAccountBuilder::buildAccount()
{
    // the account manager triggers the first checkConnection in the state when it's added - there should be no need to call it again explicitly
    // either in the application or the folderman, as was the case previously
    _accountState = AccountManager::instance()->addAccount(_account);
    connect(_accountState, &AccountState::stateChanged, this, &NewAccountBuilder::onAccountStateChanged);
    _accountState->setSettingUp(true);
}

void NewAccountBuilder::onAccountStateChanged(AccountState::State state)
{
    if (state == AccountState::Connected) {
        disconnect(_accountState, nullptr, this, nullptr);
        completeAccountSetup();
    }
    // On the off chance that the server has somehow broken down between first auth during the wizard's info collection process, and "now", when the
    // user has accepted the wizard and the actual account is created, we want to remove the account and notify there was a problem.
    else if (state == AccountState::ServiceUnavailable || state == AccountState::NetworkError || state == AccountState::MaintenanceMode
        || state == AccountState::ConfigurationError) {
        disconnect(_accountState, nullptr, this, nullptr);
        QString serverError = _accountState->connectionErrors().isEmpty() ? tr("no error details are available") : _accountState->connectionErrors().first();
        QString error = tr("Unable to connect to server during account creation: %1.").arg(serverError);
        Q_EMIT unableToCompleteAccountCreation(error);
        // do this last as the wizard is going to auto-pop as soon as the account is deleted :/
        AccountManager::instance()->deleteAccount(_accountState);
    }
}

void NewAccountBuilder::completeAccountSetup()
{
    AccountManager::instance()->saveAccount(_account.get());

    // the creds should always be ready when creating new account as we don't get here unless the auth and all other checks succeeded
    // the check is perfunctory.
    if (_account->credentials()->ready()) {
        // this really should be called by the creds, internally. Unfortunately there is no good way to do that yet. I think the account
        // should call a creds->finishSetup in account::setCredentials or something like that to ensure the creds are fully viable once the account exists.
        // unfortuantely, at the moment the account is not fully configured at the point that we call setCredentails, due to the upside
        // down deps with accountState, which is why the account state has to check "was ever fetched" on the creds, then run the
        // keychain retrieval for the first time if necessary = the creds are FINALLY ready to go. Someday we will be able to correct this, sadly, not now.
        _account->credentials()->persist();
        // emitting credentialsFetched is required because we didn't use the credentials to auth the new user/account. In theory we should have a
        // way to trigger the creds to emit this itself, but this is a future topic. same with calling persist. todo for future.
        Q_EMIT _account->credentialsFetched();
    }

    // if we are doing sync all or vfs the folder man should configure all the folders automatically
    if (_syncType != NewAccount::SyncType::SELECTIVE_SYNC) {
        bool useVfs = (_syncType == NewAccount::SyncType::USE_VFS);
        Q_EMIT requestSetUpSyncFoldersForAccount(_accountState, useVfs);
    }
    // remove the placeholder state in the gui - this impl is not great as the account state can't decide for itself whether it's setting up
    // or not, but for now, it's what we have
    _accountState->setSettingUp(false);

    // if selective sync is selected, we need to run the folder wizard asap.
    if (_syncType == NewAccount::SyncType::SELECTIVE_SYNC)
        Q_EMIT requestFolderWizard(_account.get());

    deleteLater();
}
}
