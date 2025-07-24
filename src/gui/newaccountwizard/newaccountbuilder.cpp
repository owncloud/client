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
#include "accountstate.h"
// #include "owncloudgui.h"
#include "folderman.h"

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
    // consider: what if the new account never successfully connects? how and when is is this possible?
    // and what on earth can we do about it?
    if (state == AccountState::Connected) {
        disconnect(_accountState, nullptr, this, nullptr);
        completeAccountSetup();
    }
}

void NewAccountBuilder::completeAccountSetup()
{
    AccountManager::instance()->saveAccount(_account.get(), true);
    // emitting credentialsFetched seems to be "required" to get the folder gui to show that x of y folders are synced?!
    // it appears that credentialsFetched triggers the spaces manager to refresh. and THAT is what notifies the gui of the number of synced folders?
    // this is normally triggered when the httpcredentials have fetched the creds, but when setting up an account we have already collected the creds and
    // set them in the account at the same time we create it.
    // BUT no one is aware that the account exists at that point in time as everyone is waiting to hear that the account STATE
    // was added before connecting various listeners.
    if (_account->credentials()->ready())
        Q_EMIT _account->credentialsFetched();
    if (_syncType != NewAccount::SyncType::SELECTIVE_SYNC) {
        bool useVfs = (_syncType == NewAccount::SyncType::USE_VFS);
        // replace with signal
        FolderMan::instance()->setUpInitialSyncFolders(_accountState, useVfs);
    }
    // the account is now ready, emulate a normal account loading and Q_EMIT that the credentials are ready
    // Refactoring todo: the account should ideally emit this when it meets some internal state, not left to some gui controller
    // Q_EMIT requestSetUpSyncFoldersForAccount(_accountState, true);
    _accountState->setSettingUp(false);

    // if selective sync is the type, we need to run the folder wizard asap. naturally the owncloudGui doesn't have a reusable way of doing that yet
    // so it comes in the next step.

    delete this;
}
}
