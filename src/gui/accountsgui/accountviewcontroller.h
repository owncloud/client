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

#include "accountstate.h"
#include <QObject>
#include <QPointer>

namespace OCC {

class AccountView;
class AccountModalWidget;

enum class StatusIcon { None, Connected, Disconnected, Info, Warning };

class AccountViewController : public QObject
{
    Q_OBJECT
public:
    explicit AccountViewController(AccountView *view, AccountState *state, QObject *parent);

    void addAccountModalWidget(AccountModalWidget *widget);

signals:
    // these are sent when the account view starts and ends a "modal" operation
    // at the moment I'm not blocking access to main window toolbar actions as there is really no need, imo,
    // just because the account is in the middle of something. At least we will try it this way and see
    // if it's preferred. So long as the account modal widget blocks *account* related activity I think we're good
    void accountEndModal(QUuid accountId);
    void accountBeginModal(QUuid accountId);

protected:
    void onAccountStateChanged(OCC::AccountState::State state);
    void onDeleteAccount();
    void onOpenAccountInBrowser();
    void onToggleSignInState();

    void onAddFolder();
    void onFolderWizardAccepted();

    void finishAccountModalWidget(AccountModalWidget *widget);

private:
    QPointer<AccountView> _view = nullptr;
    QPointer<AccountState> _accountState;

    void buildManageAccountMenu();
    QIcon lookupIcon(StatusIcon status);
};

}
