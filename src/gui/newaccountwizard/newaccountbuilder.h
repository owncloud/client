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
#pragma once

#include "newaccountmodel.h"
#include <QObject>

#include "accountfwd.h"
#include "accountstate.h"

#include "newaccountenums.h"

namespace OCC {

class NewAccountBuilder : public QObject
{
    Q_OBJECT
public:
    explicit NewAccountBuilder(const NewAccountModel &model, QObject *parent = nullptr);

    void buildAccount();

Q_SIGNALS:
    void requestSetUpSyncFoldersForAccount(AccountState *, bool useVfs);
    void requestFolderWizard(OCC::AccountPtr account);
    void unableToCompleteAccountCreation(const QString &error);

private:
    void onAccountStateChanged(AccountState::State state);
    void completeAccountSetup();

    AccountPtr _account = nullptr;
    AccountState *_accountState = nullptr;
    NewAccount::SyncType _syncType = NewAccount::SyncType::NONE;
};
}
