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

#include <QObject>
#include <QPointer>

#include <QHash>
#include <QUuid>

class QToolBar;
class QAction;
class QActionGroup;

namespace OCC {

class AccountState;
class AccountManager;
class Account;
class MainWindow;
class AccountViewController;

class AccountsGuiController : public QObject
{
    Q_OBJECT

public:
    AccountsGuiController(AccountManager *accountMgr, MainWindow *window, QObject *parent);
    void runAccountWizard();


private:
    QPointer<AccountManager> _accountMgr;
    QPointer<MainWindow> _window;
    QHash<QUuid, QAction *> _actionForAccount;
    QHash<QUuid, AccountViewController *> _viewControllerForAccount;

    void onAccountAdded(AccountState *state);
    void onAccountRemoved(AccountState *state);
    void onLastAccountRemoved();

    void onAccountAvatarChanged();

    void runFolderWizard(QUuid accountId);
    void handleAccountSetupError(const QString &error);

    void startModal(QUuid accountId);
    void endModal(QUuid accountId);

    void setupAccountPlaceholder();
    void removeAccountPlaceholder();
};
}
