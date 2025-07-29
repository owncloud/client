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
    connect(_accountState.get(), &AccountState::stateChanged, this, &NewAccountBuilder::onAccountStateChanged);
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
    AccountManager::instance()->saveAccount(_account.get(), true);
    // emitting credentialsFetched seems to be "required" to get the folder gui to show that x of y folders are synced
    // it appears that credentialsFetched triggers the spaces manager to refresh, and that refresh notifies the gui of the number of synced folders.
    // this is normally triggered when the httpcredentials have fetched the creds, but when setting up an account we have already collected the creds and
    // set them in the account at the same time we create it.
    // BUT no one is aware that the account exists at that point in time as everyone is waiting to hear that the account STATE
    // was added before connecting various listeners. If we don't emit the signal directly it takes some time for the update to happen via some polling
    // mechanism
    if (_account->credentials()->ready())
        Q_EMIT _account->credentialsFetched();
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
        Q_EMIT requestFolderWizard(_account);

    deleteLater();
}
}
