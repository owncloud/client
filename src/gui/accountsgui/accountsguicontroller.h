/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
