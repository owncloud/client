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

#include "accountviewcontroller.h"

#include "FoldersGui/accountfolderscontroller.h"
#include "accountmanager.h"
#include "accountmodalwidget.h"
#include "accountview.h"
#include "commonstrings.h"
#include "configfile.h"
#include "folderman.h"
#include "folderwizard.h"
#include "iconresources.h"
#include "libsync/theme.h"

#include <QDesktopServices>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

namespace OCC {

Q_LOGGING_CATEGORY(lcAccountViewController, "gui.account.viewcontroller", QtInfoMsg)

AccountViewController::AccountViewController(AccountView *view, AccountState *state, QObject *parent)
    : QObject{parent}
    , _view(view)
    , _accountState(state)
{
    if (!_view || !_accountState || !_accountState->account())
        return;

    connect(_view, &AccountView::requestMenuActionUpdate, this, &AccountViewController::refreshAccountActions);

    AccountFoldersController *foldersController = new AccountFoldersController(_accountState, _view->foldersView(), this);
    connect(foldersController, &AccountFoldersController::requestAddFolder, this, &AccountViewController::runFolderWizard);
    connect(foldersController, &AccountFoldersController::requestAccountModalWidget, this, &AccountViewController::addAccountModalWidget);

    connect(_accountState, &AccountState::stateChanged, this, &AccountViewController::onAccountStateChanged);
    connect(_accountState, &AccountState::isSettingUpChanged, _view, &AccountView::accountSettingUpChanged);

    buildManageAccountMenu();

    onAccountStateChanged(_accountState->state());
}

void AccountViewController::onOpenAccountInBrowser()
{
    if (!_accountState) {
        return;
    }

    QUrl url = _accountState->account()->url();
    if (!Theme::instance()->overrideServerPath().isEmpty()) {
        // There is an override for the WebDAV endpoint. Remove it for normal web browsing.
        url.setPath({});
    }
    QDesktopServices::openUrl(url);
}

void AccountViewController::onToggleSignInState()
{
    if (!_accountState) {
        return;
    }

    if (_accountState->isSignedOut()) {
        _accountState->signIn();
    } else {
        _accountState->signOutByUi();
    }
}

void AccountViewController::buildManageAccountMenu()
{
    if (!_view || !_accountState)
        return;

    QList<QAction *> actions;

    _logInOut = new QAction(tr("Log in"), this);
    _logInOut->setObjectName("logInOutAction");
    connect(_logInOut, &QAction::triggered, this, &AccountViewController::onToggleSignInState);
    actions.push_back(_logInOut);

    _reconnect = new QAction(tr("Reconnect"), this);
    _reconnect->setObjectName("reconnectAction");
    connect(_reconnect, &QAction::triggered, this, [this] { _accountState->checkConnectivity(true); });
    actions.push_back(_reconnect);

    _showInBrowser = new QAction(CommonStrings::showInWebBrowser(), this);
    _showInBrowser->setObjectName("showInBrowserAction");
    connect(_showInBrowser, &QAction::triggered, this, &AccountViewController::onOpenAccountInBrowser);
    actions.push_back(_showInBrowser);

    _remove = new QAction(tr("Remove"), this);
    _remove->setObjectName("removeAction");
    connect(_remove, &QAction::triggered, this, &AccountViewController::onDeleteAccount);
    actions.push_back(_remove);

    _rename = new QAction(tr("Change alias..."));
    _rename->setObjectName("changeAliasAction");
    connect(_rename, &QAction::triggered, this, &AccountViewController::onChangeAlias);
    actions.push_back(_rename);

    _view->setAccountMenuActions(actions);
}

void AccountViewController::refreshAccountActions()
{
    Q_ASSERT(_logInOut && _reconnect && _showInBrowser && _remove);

    // no we don't need to set enabled to match account state existence, if account state is null it's NOT coming back.
    if (!_accountState) {
        _logInOut->setEnabled(false);
        _reconnect->setEnabled(false);
        _showInBrowser->setEnabled(false);
        _remove->setEnabled(false);
        _rename->setEnabled(false);
        return;
    }

    _logInOut->setText(_accountState->isSignedOut() ? tr("Log in") : tr("Log out"));
    _reconnect->setEnabled(!_accountState->isConnected() && !_accountState->isSignedOut());
}

void AccountViewController::addAccountModalWidget(AccountModalWidget *widget)
{
    if (!_accountState || !_accountState->account()) {
        return;
    }

    _view->setTopStackWidget(widget);

    connect(widget, &AccountModalWidget::finished, this, &AccountViewController::finishAccountModalWidget);

    emit accountBeginModal(_accountState->account()->uuid());
}

void AccountViewController::finishAccountModalWidget(AccountModalWidget *widget)
{
    if (!widget)
        return;

    _view->removeStackWidget(widget);
    widget->deleteLater();

    Q_ASSERT(_accountState && _accountState->account());

    emit accountEndModal(_accountState->account()->uuid());
}

void AccountViewController::onDeleteAccount()
{
    if (!_accountState) {
        return;
    }

    // Deleting the account potentially deletes 'this', so
    // the QMessageBox should be destroyed before that happens.
    // todo: this is an unnecessarily complicated def for the message box and should exec instead of open.
    auto messageBox = new QMessageBox(QMessageBox::Question, tr("Confirm Account Removal"),
        tr("<p>Do you really want to remove the connection to the account <i>%1</i>?</p>"
           "<p><b>Note:</b> This will <b>not</b> delete any files.</p>")
            .arg(_accountState->account()->displayNameWithHost()),
        QMessageBox::NoButton, _view);
    messageBox->setObjectName("confirmRemoveAccountDialog");
    auto yesButton = messageBox->addButton(tr("Remove connection"), QMessageBox::YesRole);
    yesButton->setObjectName("removeAccountButton");
    auto noButton = messageBox->addButton(tr("Cancel"), QMessageBox::NoRole);
    noButton->setObjectName("cancelRemoveAccountButton");

    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    connect(messageBox, &QMessageBox::finished, this, [this, messageBox, yesButton] {
        if (messageBox->clickedButton() == yesButton) {
            auto manager = AccountManager::instance();
            manager->deleteAccount(_accountState);
            manager->save();
        }
    });
    messageBox->open();
}

void AccountViewController::onChangeAlias()
{
    if (!_accountState || !_accountState->account())
        return;


    // naturally this thing is STUPIDLY TINY by default so using the convenience method is out:
    // QString newAlias = QInputDialog::getText(_view, tr("Change alias"), tr("Account alias:"), QLineEdit::Normal, _accountState->account()->accountAlias());

    QInputDialog dialog(_view);
    dialog.setWindowTitle(tr("Change alias"));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setLabelText(tr("Account alias:"));
    dialog.setTextValue(_accountState->account()->accountAlias());
    QSize minHint = dialog.minimumSizeHint(); // width is 192?
    dialog.resize(minHint.width() * 2, minHint.height());

    int res = dialog.exec();
    if (res == QDialog::Accepted && !dialog.textValue().isEmpty())
        _accountState->account()->setAccountAlias(dialog.textValue());
}

QIcon AccountViewController::lookupStatusIcon(StatusIcon status)
{
    QIcon icon;
    switch (status) {
    case StatusIcon::None:
        break;
    case StatusIcon::Connected:
        icon = IconResources::getCoreIcon(QStringLiteral("states/ok"));
        break;
    case StatusIcon::Disconnected:
        icon = IconResources::getCoreIcon(QStringLiteral("states/offline"));
        break;
    case StatusIcon::Info:
        icon = IconResources::getCoreIcon(QStringLiteral("states/information"));
        break;
    case StatusIcon::Warning:
        icon = IconResources::getCoreIcon(QStringLiteral("states/warning"));
        break;
    }
    return icon;
}

void AccountViewController::onAccountStateChanged(AccountState::State state)
{
    if (!_accountState || !_accountState->account() || !_view) {
        return;
    }

    Account *account = _accountState->account();
    qCDebug(lcAccountViewController()) << "Account state changed to" << state << "for account" << account;

    QStringList errors;
    QString text;
    StatusIcon icon;

    switch (state) {
    case AccountState::Connected: {
        icon = StatusIcon::Connected;
        if (account->serverSupportLevel() != Account::ServerSupportLevel::Supported) {
            errors << tr("The server version %1 is unsupported! Proceed at your own risk.").arg(account->capabilities().status().versionString());
            icon = StatusIcon::Warning;
        }
        text = tr("Connected");
        break;
    }
    case AccountState::ServiceUnavailable:
        text = tr("Server is temporarily unavailable.");
        icon = StatusIcon::Disconnected;
        break;
    case AccountState::MaintenanceMode:
        text = tr("Server is currently in maintenance mode.");
        icon = StatusIcon::Disconnected;
        break;
    case AccountState::SignedOut:
        text = tr("Signed out");
        icon = StatusIcon::Disconnected;
        break;
    case AccountState::AskingCredentials:
        text = tr("Updating credentials…");
        icon = StatusIcon::Info;
        break;
    case AccountState::Connecting:
        if (NetworkInformation::instance()->isBehindCaptivePortal()) {
            text = tr("Captive portal prevents connections to the server.");
            icon = StatusIcon::Disconnected;
        } else if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
            text = tr("Sync is paused due to metered internet connection.");
            icon = StatusIcon::Disconnected;
        } else {
            text = tr("Connecting…");
            icon = StatusIcon::Info;
        }
        break;
    case AccountState::ConfigurationError:
        text = tr("Server configuration error");
        icon = StatusIcon::Warning;
        errors = _accountState->connectionErrors();
        break;
    case AccountState::NetworkError:
        text = tr("Server is temporarily unavailable");
        icon = StatusIcon::Disconnected;
        errors = _accountState->connectionErrors();
        break;
    case AccountState::Disconnected:
        text = tr("Disconnected");
        icon = StatusIcon::Disconnected;
        break;
    default:
        text = tr("Invalid connection status");
        icon = StatusIcon::None;
    }

    _view->setConnectionLabel(text, lookupStatusIcon(icon), errors);
}

void AccountViewController::runFolderWizard()
{
    if (!_accountState || !_accountState->account() || !_view)
        return;


    FolderWizard *folderWizard = new FolderWizard(_accountState->account(), _view);

    connect(folderWizard, &QDialog::accepted, this, &AccountViewController::onFolderWizardAccepted);
    connect(folderWizard, &QDialog::rejected, this, [] { qCInfo(lcAccountViewController) << "Folder wizard cancelled"; });

    // ignore clang analyzer warning about potential memory leak, please.
    // the modal widget gets reparented to the stacked widget and is automatically deleted when the finished() signal is
    // received
    AccountModalWidget *widget = new AccountModalWidget({}, folderWizard, _view);
    addAccountModalWidget(widget);
}

void AccountViewController::onFolderWizardAccepted()
{
    if (!_accountState)
        return;

    FolderWizard *folderWizard = qobject_cast<FolderWizard *>(sender());
    if (!folderWizard)
        return;

    auto config = folderWizard->result();

    // The gui should not allow users to selectively choose any sync lists if vfs is enabled, but this kind of check was
    // originally in play here so...keep it just in case.
    if (config.useVirtualFiles && !config.selectiveSyncBlackList.empty())
        config.selectiveSyncBlackList.clear();


    // Refactoring todo: turn this into a signal/requestAddFolder
    FolderMan::instance()->addFolderFromGui(_accountState, config);
}

}
