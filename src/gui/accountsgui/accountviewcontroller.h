/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "accountstate.h"
#include <QObject>
#include <QPointer>

class QAction;

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
    void runFolderWizard();

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

    void onFolderWizardAccepted();

    void finishAccountModalWidget(AccountModalWidget *widget);

private:
    QPointer<AccountView> _view = nullptr;
    QPointer<AccountState> _accountState;

    QAction *_logInOut = nullptr;
    QAction *_reconnect = nullptr;
    QAction *_showInBrowser = nullptr;
    QAction *_remove = nullptr;

    void buildManageAccountMenu();
    void refreshAccountActions();
    QIcon lookupStatusIcon(StatusIcon status);
};

}
