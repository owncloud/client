/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "newaccountmodel.h"
#include <QObject>

#include "libsync/account.h"
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
    void requestSetUpSyncFoldersForAccount(OCC::AccountState *, bool useVfs);
    void requestLoadSpacesOnly(OCC::AccountState *);
    void requestFolderWizard(QUuid accountId);
    void unableToCompleteAccountCreation(const QString &error);

private:
    void onAccountStateChanged(AccountState::State state);
    void completeAccountSetup();

    Account *_account = nullptr;
    AccountState *_accountState = nullptr;
    NewAccount::SyncType _syncType = NewAccount::SyncType::NONE;
};
}
