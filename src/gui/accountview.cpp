/*
 * Copyright (C) by Lisa Reese <lisa.reese@kiteworks.com>
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


#include "accountview.h"
#include "FoldersGui/accountfoldersview.h"
#include "FoldersGui/foldermodelcontroller.h"
#include "ui_accountview.h"


#include "FoldersGui/accountfolderscontroller.h"
#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "application.h"
#include "common/utility.h"
#include "commonstrings.h"
#include "configfile.h"
#include "folderman.h"
#include "folderstatusmodel.h"
#include "folderwizard/folderwizard.h"
#include "gui/accountmodalwidget.h"
#include "gui/models/models.h"
#include "gui/networkinformation.h"
#include "gui/qmlutils.h"
#include "gui/selectivesyncwidget.h"
#include "gui/spaces/spaceimageprovider.h"
#include "guiutility.h"
#include "libsync/graphapi/spacesmanager.h"
#include "openfilemanager.h"
#include "scheduling/syncscheduler.h"
#include "settingsdialog.h"
#include "theme.h"

#include <QAction>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QtQuickWidgets/QtQuickWidgets>

namespace {
constexpr auto modalWidgetStretchedMarginC = 50;
}

namespace OCC {

Q_LOGGING_CATEGORY(lcAccountView, "gui.account.view", QtInfoMsg)

// Refactoring todo: devise a correct handling of null account state in this ctr.
// also move all this connect stuff to a "connect" method.
// also ditch the lambdas which should actually be functions (private if necessary)
// Also refactoring todo: split the controller behavior out into a controller. A widget should NOT contain
// business or controller logic!
AccountView::AccountView(AccountState *accountState, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AccountView)
    , _wasDisabledBefore(false)
    , _accountState(accountState)
{
    ui->setupUi(this);

    // as usual we do too many things in the ctr and we need to eval all the code paths to make sure they handle
    // the QPointer properly, but as a stopgap to catch null states asap before they trickle down into other areas:
    if (!_accountState || !_accountState->account())
        return;

    AccountFoldersController *foldersController = new AccountFoldersController(_accountState, ui->accountFoldersView, this);
    connect(foldersController, &AccountFoldersController::requestShowModalWidget, this, &AccountView::onRequestShowModalWidget);

    //   _model = new FolderStatusModel(_accountState, this);

    //  connect(folderMan, &FolderMan::folderSyncStateChange, _model, &FolderStatusModel::slotFolderSyncStateChange);

    ui->connectionStatusLabel->clear();

    connect(_accountState, &AccountState::stateChanged, this, &AccountView::slotAccountStateChanged);
    slotAccountStateChanged(_accountState->state());

    buildManageAccountMenu();

    connect(_accountState, &AccountState::isSettingUpChanged, this, &AccountView::accountSettingUpChanged);

    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this,
        [this] { ui->manageAccountButton->setEnabled(ui->stackedWidget->currentWidget() == ui->accountFoldersView); });
    ui->stackedWidget->setCurrentWidget(ui->accountFoldersView);
}

void AccountView::accountSettingUpChanged(bool settingUp)
{
    if (settingUp) {
        ui->spinner->startAnimation();
        ui->stackedWidget->setCurrentWidget(ui->loadingPage);
    } else {
        ui->spinner->stopAnimation();
        ui->stackedWidget->setCurrentWidget(ui->accountFoldersView);
    }
}

void AccountView::slotOpenAccountInBrowser()
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

void AccountView::slotToggleSignInState()
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



void AccountView::showConnectionLabel(const QString &message, StatusIcon statusIcon, QStringList errors)
{
    if (errors.isEmpty()) {
        ui->connectionStatusLabel->setText(message);
        ui->connectionStatusLabel->setToolTip(QString());
    } else {
        errors.prepend(message);
        const QString msg = errors.join(QLatin1String("\n"));
        qCDebug(lcAccountView) << msg;
        ui->connectionStatusLabel->setText(msg);
        ui->connectionStatusLabel->setToolTip(QString());
    }
    ui->accountStatus->setVisible(!message.isEmpty());

    QIcon icon;
    switch (statusIcon) {
    case StatusIcon::None:
        break;
    case StatusIcon::Connected:
        icon = Resources::getCoreIcon(QStringLiteral("states/ok"));
        break;
    case StatusIcon::Disconnected:
        icon = Resources::getCoreIcon(QStringLiteral("states/offline"));
        break;
    case StatusIcon::Info:
        icon = Resources::getCoreIcon(QStringLiteral("states/information"));
        break;
    case StatusIcon::Warning:
        icon = Resources::getCoreIcon(QStringLiteral("states/warning"));
        break;
    }

    if (!icon.isNull()) {
        ui->warningLabel->setPixmap(icon.pixmap(ui->warningLabel->size()));
    }
    ui->warningLabel->setVisible(statusIcon != StatusIcon::None);
}


void AccountView::buildManageAccountMenu()
{
    QMenu *menu = new QMenu(this);
    menu->setAccessibleName(tr("Account options menu"));

    auto *logInOutAction = menu->addAction(tr("Log in"), this, &AccountView::slotToggleSignInState);
    auto *reconnectAction = menu->addAction(tr("Reconnect"), this, [this] { _accountState->checkConnectivity(true); });
    reconnectAction->setEnabled(!_accountState->isConnected() && !_accountState->isSignedOut());
    connect(_accountState, &AccountState::stateChanged, this, [logInOutAction, reconnectAction, this]() {
        logInOutAction->setText(_accountState->isSignedOut() ? tr("Log in") : tr("Log out"));
        reconnectAction->setEnabled(!_accountState->isConnected() && !_accountState->isSignedOut());
    });

    menu->addAction(CommonStrings::showInWebBrowser(), this, &AccountView::slotOpenAccountInBrowser);
    menu->addAction(tr("Remove"), this, &AccountView::slotDeleteAccount);

    ui->manageAccountButton->setMenu(menu);
}

// Refactoring todo: the signal sends the new account state, refactor this to use that param
void AccountView::slotAccountStateChanged(AccountState::State state)
{
    if (!_accountState || !_accountState->account()) {
        return;
    }

    Account *account = _accountState->account();
    qCDebug(lcAccountView) << "Account state changed to" << state << "for account" << account;

    // FolderMan *folderMan = FolderMan::instance();
    /*  I think the controller should be able to listen for folder state changes to determine if eg it went from paused to running, oder?
     *  // anyway this doesn't belong in any gui, just a controller
     *   for (auto *folder : folderMan->folders()) {
           _model->slotUpdateFolderState(folder);
       }*/

    switch (state) {
    case AccountState::Connected: {
        QStringList errors;
        StatusIcon icon = StatusIcon::Connected;
        if (account->serverSupportLevel() != Account::ServerSupportLevel::Supported) {
            errors << tr("The server version %1 is unsupported! Proceed at your own risk.").arg(account->capabilities().status().versionString());
            icon = StatusIcon::Warning;
        }
        showConnectionLabel(tr("Connected"), icon, errors);
        // connect(_accountState->account()->spacesManager(), &GraphApi::SpacesManager::updated, this, &AccountView::slotSpacesUpdated, Qt::UniqueConnection);

        // Refactoring todo: won't this get called every time the state changes to connected even if the spaces manager is already
        // triggering the slot? ie duplicate call to slotSpacesUpdated?
        // slotSpacesUpdated();

        break;
    }
    case AccountState::ServiceUnavailable:
        showConnectionLabel(tr("Server is temporarily unavailable"), StatusIcon::Disconnected);
        break;
    case AccountState::MaintenanceMode:
        showConnectionLabel(tr("Server is currently in maintenance mode"), StatusIcon::Disconnected);
        break;
    case AccountState::SignedOut:
        showConnectionLabel(tr("Signed out"), StatusIcon::Disconnected);
        break;
    case AccountState::AskingCredentials: {
        showConnectionLabel(tr("Updating credentials…"), StatusIcon::Info);
        break;
    }
    case AccountState::Connecting:
        if (NetworkInformation::instance()->isBehindCaptivePortal()) {
            showConnectionLabel(tr("Captive portal prevents connections to the server."), StatusIcon::Disconnected);
        } else if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
            showConnectionLabel(tr("Sync is paused due to metered internet connection"), StatusIcon::Disconnected);
        } else {
            showConnectionLabel(tr("Connecting…"), StatusIcon::Info);
        }
        break;
    case AccountState::ConfigurationError:
        showConnectionLabel(tr("Server configuration error"), StatusIcon::Warning, _accountState->connectionErrors());
        break;
    case AccountState::NetworkError:
        // don't display the error to the user, https://github.com/owncloud/client/issues/9790
        [[fallthrough]];
    case AccountState::Disconnected:
        showConnectionLabel(tr("Disconnected"), StatusIcon::Disconnected);
        break;
    }
}


AccountView::~AccountView()
{
    _goingDown = true;
    delete ui;
}

void AccountView::addModalLegacyDialog(QWidget *widget, ModalWidgetSizePolicy sizePolicy)
{
    if (!widget->testAttribute(Qt::WA_DeleteOnClose)) { // DEBUG CODE! See https://github.com/owncloud/client/issues/11673
        // Early check to see if the attribute gets unset before the second/real check below
        qCWarning(lcAccountView) << "Missing WA_DeleteOnClose! (1)" << widget->metaObject() << widget;
    }

    // create a widget filling the stacked widget
    // this widget contains a wrapping group box with widget as content
    auto *outerWidget = new QWidget;
    auto *groupBox = new QGroupBox;

    switch (sizePolicy) {
    case ModalWidgetSizePolicy::Expanding: {
        auto *outerLayout = new QHBoxLayout(outerWidget);
        outerLayout->setContentsMargins(modalWidgetStretchedMarginC, modalWidgetStretchedMarginC, modalWidgetStretchedMarginC, modalWidgetStretchedMarginC);
        outerLayout->addWidget(groupBox);
        auto *layout = new QHBoxLayout(groupBox);
        layout->addWidget(widget);
    } break;
    case ModalWidgetSizePolicy::Minimum: {
        auto *outerLayout = new QGridLayout(outerWidget);
        outerLayout->addWidget(groupBox, 0, 0, Qt::AlignCenter);
        auto *layout = new QHBoxLayout(groupBox);
        layout->addWidget(widget);
    } break;
    }
    groupBox->setTitle(widget->windowTitle());

    ui->stackedWidget->addWidget(outerWidget);
    ui->stackedWidget->setCurrentWidget(outerWidget);

    // the widget is supposed to behave like a dialog and we connect to its destuction
    if (!widget->testAttribute(Qt::WA_DeleteOnClose)) { // DEBUG CODE! See https://github.com/owncloud/client/issues/11673
        qCWarning(lcAccountView) << "Missing WA_DeleteOnClose! (2)" << widget->metaObject() << widget;
    }
    Q_ASSERT(widget->testAttribute(Qt::WA_DeleteOnClose));

    // Refactoring todo: eval this more completely
    Q_ASSERT(_accountState && _accountState->account());

    connect(widget, &QWidget::destroyed, this, [this, outerWidget] {
        outerWidget->deleteLater();
        if (!_goingDown) {
            ocApp()->gui()->settingsDialog()->ceaseModality(_accountState->account());
        }
    });
    widget->setVisible(true);
    ocApp()->gui()->settingsDialog()->requestModality(_accountState->account());
}

void AccountView::addModalAccountWidget(AccountModalWidget *widget)
{
    if (!_accountState || !_accountState->account()) {
        return;
    }

    ui->stackedWidget->addWidget(widget);
    ui->stackedWidget->setCurrentWidget(widget);

    connect(widget, &AccountModalWidget::finished, this, [widget, this] {
        widget->deleteLater();
        ocApp()->gui()->settingsDialog()->ceaseModality(_accountState->account());
    });
    ocApp()->gui()->settingsDialog()->requestModality(_accountState->account());
}


void AccountView::slotDeleteAccount()
{
    if (!_accountState) {
        return;
    }

    // Deleting the account potentially deletes 'this', so
    // the QMessageBox should be destroyed before that happens.
    auto messageBox = new QMessageBox(QMessageBox::Question, tr("Confirm Account Removal"),
        tr("<p>Do you really want to remove the connection to the account <i>%1</i>?</p>"
           "<p><b>Note:</b> This will <b>not</b> delete any files.</p>")
            .arg(_accountState->account()->displayNameWithHost()),
        QMessageBox::NoButton, this);
    auto yesButton = messageBox->addButton(tr("Remove connection"), QMessageBox::YesRole);
    messageBox->addButton(tr("Cancel"), QMessageBox::NoRole);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    connect(messageBox, &QMessageBox::finished, this, [this, messageBox, yesButton]{
        if (messageBox->clickedButton() == yesButton) {
            auto manager = AccountManager::instance();
            manager->deleteAccount(_accountState);
            manager->save();
        }
    });
    messageBox->open();
}

} // namespace OCC
