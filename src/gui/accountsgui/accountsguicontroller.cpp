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

#include "accountsguicontroller.h"

#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QToolBar>

#include "accountmanager.h"
#include "accountplaceholderwidget.h"
#include "accountsgui/accountviewcontroller.h"
#include "accountstate.h"
#include "accountview.h"
#include "creds/abstractcredentials.h"
#include "folderman.h"
#include "mainwindow/mainwindow.h"
#include "newaccountwizard/newaccountbuilder.h"
#include "newaccountwizard/newaccountwizard.h"
#include "newaccountwizard/newaccountwizardcontroller.h"
#include "resources.h"

namespace OCC {

AccountsGuiController::AccountsGuiController(AccountManager *accountMgr, MainWindow *window, QObject *parent)
    : QObject(parent)
    , _accountMgr(accountMgr)
    , _window(window)
{
    Q_ASSERT(_accountMgr && _window);
    // load any existing accounts from the manager.
    const auto accounts = _accountMgr->accounts();
    for (const auto &accountState : accounts) {
        onAccountAdded(accountState);
    }
    if (!accounts.isEmpty()) {
        _actionForAccount.value(accounts.first()->account()->uuid(), nullptr)->setChecked(true);
    }

    connect(_accountMgr, &AccountManager::accountAdded, this, &AccountsGuiController::onAccountAdded);
    connect(_accountMgr, &AccountManager::accountRemoved, this, &AccountsGuiController::onAccountRemoved);
    connect(_accountMgr, &AccountManager::lastAccountRemoved, this, &AccountsGuiController::onLastAccountRemoved);

    if (accounts.isEmpty()) {
        // this is a genuinely reasonable use of timer to execute an operation after construction and any
        // other event loop actions have finished
        QTimer::singleShot(0, this, &AccountsGuiController::runAccountWizard);
    }
}

void AccountsGuiController::onAccountAdded(AccountState *state)
{
    if (!_window || !state || !state->account())
        return;

    Account *account = state->account();
    QUuid accountId = account->uuid();
    connect(account, &Account::avatarChanged, this, &AccountsGuiController::onAccountAvatarChanged);

    auto accountView = new AccountView(nullptr);
    // for both the view and the action, we create a unique objectName using the account uuid
    // to support squish test object identification
    accountView->setObjectName(QString("accountView_%1").arg(accountId.toString()));

    AccountViewController *viewController = new AccountViewController(accountView, state, this);
    connect(account->credentials(), &AbstractCredentials::requestAccountModal, viewController, &AccountViewController::addAccountModalWidget);

    connect(viewController, &AccountViewController::accountBeginModal, this, &AccountsGuiController::startModal);
    connect(viewController, &AccountViewController::accountEndModal, this, &AccountsGuiController::endModal);

    QAction *accountAction = new QAction(this);
    accountAction->setObjectName(QString("accountAction_%1").arg(accountId.toString()));
    _actionForAccount.insert(accountId, accountAction);

    accountAction->setIcon(account->avatar());
    // eventually make this elided
    // problem is, the action has no idea what the size of the button should be.
    // it's really dumb to have "long" text on any toolbar component in the first place. We hope to fix this someday
    // by allowing the user to set an account alias which will show as the account name but that needs a major release
    // to support the config change
    accountAction->setText(account->hostName());

    accountAction->setToolTip(QString("%1\n%2").arg(account->davDisplayName(), account->url().toDisplayString()));
    accountAction->setData(QVariant::fromValue(accountView));
    accountAction->setCheckable(true);

    _window->addAccountAction(accountAction);

    accountAction->setChecked(true);

    if (_actionForAccount.contains(QUuid()))
        removeAccountPlaceholder();
}

void AccountsGuiController::onAccountRemoved(AccountState *state)
{
    if (!_window || !state || !state->account())
        return;

    Account *acc = state->account();
    QUuid uid = acc->uuid();
    if (_actionForAccount.contains(uid)) {
        QAction *action = _actionForAccount[uid];
        _window->removeAction(action);
        _actionForAccount.remove(uid);
        action->deleteLater();

        if (!_accountMgr)
            return;

        const QList<AccountState *> accounts = _accountMgr->accounts();
        if (!accounts.isEmpty()) {
            QUuid newuid = accounts.last()->account()->uuid();
            if (_actionForAccount.contains(newuid))
                _actionForAccount[newuid]->setChecked(true);
        }
    }
}

void AccountsGuiController::onLastAccountRemoved()
{
    runAccountWizard();
}

void AccountsGuiController::onAccountAvatarChanged()
{
    Account *account = qobject_cast<Account *>(sender());
    if (!account)
        return;
    QAction *action = _actionForAccount.value(account->uuid(), nullptr);

    if (action) {
        action->setIcon(account->avatar());
    }
}

void AccountsGuiController::runAccountWizard()
{
    if (!_window)
        return;
    NewAccountWizard wizard(_window);
    NewAccountModel model(nullptr);
    NewAccountWizardController wizardController(&model, &wizard, nullptr);
    ownCloudGui::raise();
    int result = wizard.exec();
    if (result == QDialog::Accepted) {
        // the builder needs to be a pointer as it has to wait for the connection state to go to connected
        // it will delete itself once it has completed its mission.
        // pass this as parent only as a safeguard.
        if (!model.isComplete()) {
            QMessageBox::warning(
                _window, tr("New account failure"), tr("The information required to create a new account is incomplete. Please run the wizard again."));
        } else {
            NewAccountBuilder *builder = new NewAccountBuilder(model, this);
            FolderMan *folderman = FolderMan::instance();
            connect(builder, &NewAccountBuilder::requestSetUpSyncFoldersForAccount, folderman, &FolderMan::setUpInitialSyncFolders);
            connect(builder, &NewAccountBuilder::requestLoadSpacesOnly, folderman, &FolderMan::setUpInitialSpaces);
            connect(builder, &NewAccountBuilder::requestFolderWizard, this, &AccountsGuiController::runFolderWizard);
            connect(builder, &NewAccountBuilder::unableToCompleteAccountCreation, this, &AccountsGuiController::handleAccountSetupError);
            builder->buildAccount();
        }
    } else {
        // if there are no accounts, add a placeholder "page" to the main window that informs the user how to create one
        setupAccountPlaceholder();
    }
}

void AccountsGuiController::setupAccountPlaceholder()
{
    if (!_window || !_accountMgr || !_accountMgr->accounts().isEmpty())
        return;

    QUuid uid;
    QAction *placeholderAccountAction = nullptr;
    if (_actionForAccount.contains(uid)) {
        placeholderAccountAction = _actionForAccount[uid];
    } else {
        placeholderAccountAction = new QAction(tr("Accounts"));
        placeholderAccountAction->setObjectName("placeholderAccountAction");
        placeholderAccountAction->setCheckable(true);
        placeholderAccountAction->setIcon(Resources::getCoreIcon("warning"));
        // use null uuid for placeholder action since there IS no account for it
        _actionForAccount.insert(QUuid(), placeholderAccountAction);
        auto placeholderWidget = new AccountPlaceholderWidget(_window);
        placeholderAccountAction->setData(QVariant::fromValue(placeholderWidget));
        _window->addAccountAction(placeholderAccountAction);
    }
    placeholderAccountAction->setChecked(true);
}

void AccountsGuiController::removeAccountPlaceholder()
{
    if (!_window)
        return;
    QUuid uid;
    if (_actionForAccount.contains(uid)) {
        QAction *action = _actionForAccount[uid];
        _window->removeAction(action);
        _actionForAccount.remove(uid);
        action->deleteLater();
    }
}

void AccountsGuiController::handleAccountSetupError(const QString &error)
{
    if (!_window)
        return;

    QMessageBox::warning(_window, tr("New account failure"),
        tr("The account could not be created due to an error:\n%1\nPlease check the server's availability then run the wizard again.").arg(error));

    // note that the current behavior for the setup failure is that the account wizard auto-pops again because the
    // accountState for the new account was already created and it is subsequently deleted by the account builder if the final
    // setup fails for some reason. The accountRemoved handler then runs the wizard again if there are no other accounts.
    // In case we change that in future, this will create the dummy account page if the wizard doesn't auto-run again
    setupAccountPlaceholder();
}

void AccountsGuiController::runFolderWizard(Account *account)
{
    if (!account)
        return;

    QAction *action = _actionForAccount.value(account->uuid(), nullptr);
    if (!action)
        return;

    action->setChecked(true);
    AccountView *view = action->data().value<AccountView *>();
    if (view)
        view->slotAddFolder();
}

void AccountsGuiController::startModal(QUuid accountId)
{
    QAction *action = _actionForAccount.value(accountId, nullptr);
    if (!action)
        return;

    action->setIcon(Resources::getCoreIcon("states/warning"));
    action->setChecked(true);
    ownCloudGui::raise();
}

void AccountsGuiController::endModal(QUuid accountId)
{
    if (!_accountMgr)
        return;

    QAction *action = _actionForAccount.value(accountId, nullptr);
    if (!action)
        return;
    // get the icon from the account again and put it back on the action
    AccountState *state = _accountMgr->accountState(accountId);
    if (state && state->account()) {
        Account *account = _accountMgr->accountState(accountId)->account();
        action->setIcon(account->avatar());
    }
    // I don't think we need to set the action checked again
    Q_ASSERT(action->isChecked());
}
}
