/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
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


#include "accountsettings.h"
#include "ui_accountsettings.h"


#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "application.h"
#include "common/utility.h"
#include "commonstrings.h"
#include "configfile.h"
#include "creds/httpcredentialsgui.h"
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
#include "quotainfo.h"
#include "scheduling/syncscheduler.h"
#include "settingsdialog.h"
#include "theme.h"

#include <QAction>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QtQuickWidgets/QtQuickWidgets>

namespace {
constexpr auto modalWidgetStretchedMarginC = 50;
}

namespace OCC {

Q_LOGGING_CATEGORY(lcAccountSettings, "gui.account.settings", QtInfoMsg)

// Refactoring todo: devise a correct handling of null account state in this ctr.
// also move all this connect stuff to a "connect" method.
// also ditch the lambdas which should actually be functions (private if necessary)
// Also refactoring todo: split the controller behavior out into a controller. A widget should NOT contain
// business or controller logic!
AccountSettings::AccountSettings(const AccountStatePtr &accountState, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AccountSettings)
    , _wasDisabledBefore(false)
    , _accountState(accountState)
{
    ui->setupUi(this);

    // as usual we do too many things in the ctr and we need to eval all the code paths to make sure they handle
    // the QPointer properly, but as a stopgap to catch null states asap before they trickle down into other areas:
    Q_ASSERT(_accountState);


    _model = new FolderStatusModel(this);

    // see comments in this impl as it needs work
    _model->setAccountState(_accountState);

    auto weightedModel = new QSortFilterProxyModel(this);
    weightedModel->setSourceModel(_model);
    weightedModel->setSortRole(static_cast<int>(FolderStatusModel::Roles::Priority));
    weightedModel->sort(0, Qt::DescendingOrder);

    _sortModel = weightedModel;

    ui->quickWidget->engine()->addImageProvider(QStringLiteral("space"), new Spaces::SpaceImageProvider(_accountState->account()));
    ui->quickWidget->setOCContext(QUrl(QStringLiteral("qrc:/qt/qml/org/ownCloud/gui/qml/FolderDelegate.qml")), this);

    connect(FolderMan::instance(), &FolderMan::folderListChanged, _model, &FolderStatusModel::resetFolders);

    ui->connectionStatusLabel->clear();

    connect(_accountState.data(), &AccountState::stateChanged, this, &AccountSettings::slotAccountStateChanged);
    slotAccountStateChanged();

    // Refactoring todo: eval why this is created/destructed each time the user invokes the manageAccount button. 
    // normal impl creates actions as members of either gui, or better, a controller, and controller updates action states
    // in line with updates it receives.
    connect(ui->manageAccountButton, &QToolButton::clicked, this, [this] {
        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->setAccessibleName(tr("Account options menu"));
        menu->addAction(_accountState->isSignedOut() ? tr("Log in") : tr("Log out"), this, &AccountSettings::slotToggleSignInState);
        auto *reconnectAction = menu->addAction(tr("Reconnect"), this, [this] { _accountState->checkConnectivity(true); });
        reconnectAction->setEnabled(!_accountState->isConnected() && !_accountState->isSignedOut());
        menu->addAction(CommonStrings::showInWebBrowser(), this, &AccountSettings::slotOpenAccountInBrowser);
        menu->addAction(tr("Remove"), this, &AccountSettings::slotDeleteAccount);
        menu->popup(mapToGlobal(ui->manageAccountButton->pos()));

        // set the focus for accessability
        menu->setFocus();
    });

    connect(_accountState.get(), &AccountState::isSettingUpChanged, this, [this] {
        if (_accountState->isSettingUp()) {
            ui->spinner->startAnimation();
            ui->stackedWidget->setCurrentWidget(ui->loadingPage);
        } else {
            ui->spinner->stopAnimation();
            ui->stackedWidget->setCurrentWidget(ui->quickWidget);
        }
    });
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this,
        [this] { ui->manageAccountButton->setEnabled(ui->stackedWidget->currentWidget() == ui->quickWidget); });
    ui->stackedWidget->setCurrentWidget(ui->quickWidget);
}

void AccountSettings::slotOpenAccountInBrowser()
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

void AccountSettings::slotToggleSignInState()
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

void AccountSettings::slotCustomContextMenuRequested(Folder *folder)
{
    // Refactoring todo: we need to eval defensive handling of the account state QPointer in more depth, and I
    // am not able to easily determine what should happen in this handler if the state is null. For now we just assert
    // to make the "source" of the nullptr obvious before it trickles down into sub-areas and causes a crash that's harder
    // to id
    Q_ASSERT(_accountState);

    // qpointer for async calls
    const auto isDeployed = folder->isDeployed();
    const auto addRemoveFolderAction = [isDeployed, folder, this](QMenu *menu) {
        Q_ASSERT(!isDeployed);
        return menu->addAction(tr("Remove folder sync connection"), this, [folder, this] { slotRemoveCurrentFolder(folder); });
    };


    auto *menu = new QMenu(ui->quickWidget);
    menu->setAccessibleName(tr("Sync options menu"));
    menu->setAttribute(Qt::WA_DeleteOnClose);
    connect(folder, &OCC::Folder::destroyed, menu, &QMenu::close);
    // Only allow removal if the item isn't in "ready" state.
    if (!folder->isReady() && !isDeployed) {
        if (Theme::instance()->syncNewlyDiscoveredSpaces()) {
            menu->addAction(tr("Folder is not ready yet"))->setEnabled(false);
        } else {
            addRemoveFolderAction(menu);
        }
        menu->popup(QCursor::pos());
        // accassebility
        menu->setFocus();
        return;
    }
    // Add an action to open the folder in the system's file browser:
    QAction *showInFileManagerAction = menu->addAction(CommonStrings::showInFileBrowser(), [folder]() {
        qCInfo(lcAccountSettings) << "Opening local folder" << folder->path();
        if (QFileInfo::exists(folder->path())) {
            showInFileManager(folder->path());
        }
    });

    if (!QFile::exists(folder->path())) {
        showInFileManagerAction->setEnabled(false);
    }

    // Add an action to open the folder on the server in a webbrowser:
    // Refactoring todo: why are we using the folder accountState AND the local member? shouldn't the folder have the same account state
    // as this settings panel?!
    if (folder->accountState()->account()->capabilities().privateLinkPropertyAvailable()) {
        QString path = folder->remotePathTrailingSlash();
        menu->addAction(CommonStrings::showInWebBrowser(), [path, davUrl = folder->webDavUrl(), this] {
            fetchPrivateLinkUrl(_accountState->account(), davUrl, path, this, [](const QUrl &url) { Utility::openBrowser(url, nullptr); });
        });
    }


    // Root-folder specific actions:
    menu->addSeparator();

    // qpointer for the async context menu
    if (OC_ENSURE(folder->isReady())) {
        const bool folderPaused = folder->syncPaused();

        if (!folderPaused) {
            QAction *forceSyncAction = menu->addAction(tr("Force sync now"));
            if (folder->isSyncRunning()) {
                forceSyncAction->setText(tr("Restart sync"));
            }
            forceSyncAction->setEnabled(folder->accountState()->isConnected());
            connect(forceSyncAction, &QAction::triggered, this, [folder, this] { slotForceSyncCurrentFolder(folder); });
        }

        QAction *resumeAction = menu->addAction(folderPaused ? tr("Resume sync") : tr("Pause sync"));
        connect(resumeAction, &QAction::triggered, this, [folder, this] { slotEnableCurrentFolder(folder, true); });

        if (!isDeployed) {
            if (!Theme::instance()->syncNewlyDiscoveredSpaces()) {
                addRemoveFolderAction(menu);
            }

            auto maybeShowEnableVfs = [folder, menu, this]() {
                // Only show "Enable VFS" if a VFS mode is available
                const auto mode = VfsPluginManager::instance().bestAvailableVfsMode();
                if (FolderMan::instance()->checkVfsAvailability(folder->path(), mode)) {
                    if (mode == Vfs::WindowsCfApi) {
                        QAction *enableVfsAction = menu->addAction(tr("Enable virtual file support"));
                        connect(enableVfsAction, &QAction::triggered, this, [folder, this] { slotEnableVfsCurrentFolder(folder); });
                    }
                }
            };

            if (Theme::instance()->showVirtualFilesOption()) {
                if (Theme::instance()->forceVirtualFilesOption()) {
                    if (!folder->virtualFilesEnabled()) {
                        // VFS is currently disabled, but is forced on by theming (e.g. due to a theme change)
                        maybeShowEnableVfs();
                    }
                } else {
                    if (folder->virtualFilesEnabled()) {
                        menu->addAction(tr("Disable virtual file support"), this, [folder, this] { slotDisableVfsCurrentFolder(folder); });
                    } else {
                        maybeShowEnableVfs();
                    }
                }
            }
            if (!folder->virtualFilesEnabled()) {
                menu->addAction(tr("Choose what to sync"), this, [folder, this] { showSelectiveSyncDialog(folder); });
            }
            menu->popup(QCursor::pos());
            menu->setFocus(); // for accassebility (keyboard navigation)
        } else {
            menu->deleteLater();
        }
    }
}

void AccountSettings::showSelectiveSyncDialog(Folder *folder)
{
    if (!_accountState) {
        return;
    }

    auto *selectiveSync = new SelectiveSyncWidget(_accountState->account(), this);
    selectiveSync->setDavUrl(folder->webDavUrl());
    bool ok;
    selectiveSync->setFolderInfo(
        folder->remotePath(), folder->displayName(), folder->journalDb()->getSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, ok));
    Q_ASSERT(ok);

    auto *modalWidget = new AccountModalWidget(tr("Choose what to sync"), selectiveSync, this);
    modalWidget->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(modalWidget, &AccountModalWidget::accepted, this, [selectiveSync, folder, this] {
        folder->journalDb()->setSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, selectiveSync->createBlackList());
        doForceSyncCurrentFolder(folder);
    });
    addModalWidget(modalWidget);
}

void AccountSettings::slotAddFolder()
{
    if (!_accountState) {
        return;
    }

    FolderWizard *folderWizard = new FolderWizard(_accountState, this);
    folderWizard->setAttribute(Qt::WA_DeleteOnClose);

    connect(folderWizard, &QDialog::accepted, this, &AccountSettings::slotFolderWizardAccepted);
    connect(folderWizard, &QDialog::rejected, this, [] {
        qCInfo(lcAccountSettings) << "Folder wizard cancelled";
    });

    addModalLegacyDialog(folderWizard, AccountSettings::ModalWidgetSizePolicy::Expanding);
}


void AccountSettings::slotFolderWizardAccepted()
{
    if (!_accountState) {
        return;
    }

    FolderWizard *folderWizard = qobject_cast<FolderWizard *>(sender());
    if (!folderWizard)
        return;

    qCInfo(lcAccountSettings) << "Folder wizard completed";

    auto config = folderWizard->result();

    // The gui should not allow users to selectively choose any sync lists if vfs is enabled, but this kind of check was
    // originally in play here so...keep it just in case.
    if (config.useVirtualFiles && !config.selectiveSyncBlackList.empty()) {
        config.selectiveSyncBlackList.clear();
    }

    // Refactoring todo: turn this into a signal/requestAddFolder
    FolderMan::instance()->addFolderFromGui(_accountState, config);
}

void AccountSettings::slotRemoveCurrentFolder(Folder *folder)
{
    qCInfo(lcAccountSettings) << "Remove Folder " << folder->path();
    QString shortGuiLocalPath = folder->shortGuiLocalPath();

    auto messageBox = new QMessageBox(QMessageBox::Question, tr("Confirm Folder Sync Connection Removal"),
        tr("<p>Do you really want to stop syncing the folder <i>%1</i>?</p>"
           "<p><b>Note:</b> This will <b>not</b> delete any files.</p>")
            .arg(shortGuiLocalPath),
        QMessageBox::NoButton, ocApp()->gui()->settingsDialog());
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    QPushButton *yesButton = messageBox->addButton(tr("Remove Folder Sync Connection"), QMessageBox::YesRole);
    messageBox->addButton(tr("Cancel"), QMessageBox::NoRole);
    connect(messageBox, &QMessageBox::finished, this, [messageBox, yesButton, folder, this] {
        if (messageBox->clickedButton() == yesButton) {
            // Refactoring todo: this should just emit a signal to request removing the folder - let the FolderMan do the right stuff
            FolderMan::instance()->removeFolderSettings(folder);
            FolderMan::instance()->removeFolderSync(folder);
            // Refactoring todo: I don't understand why this is called when a folder is removed as the slot seems to be more
            // geared to loading/adding newly discovered spaces *when the spaces manager notifies something has changed*
            // instead we seem to "conveniently" recycle the meaning of the handler to cover other changes as well.
            // also, I *really* don't understand why we are using a single shot timer to "schedule" this in the main event
            // loop when we are already in the main event loop aren't we (ie we are IN a gui slot already)? I am seeing this
            // a lot and so far have no explanation as to what the reason or intent is.
            QTimer::singleShot(0, this, &AccountSettings::slotSpacesUpdated);
        }
    });
    messageBox->open();
}

void AccountSettings::slotEnableVfsCurrentFolder(Folder *folder)
{
    if (OC_ENSURE(VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::WindowsCfApi)) {
        if (!folder) {
            return;
        }
        qCInfo(lcAccountSettings) << "Enabling vfs support for folder" << folder->path();

        // Change the folder vfs mode and load the plugin
        folder->setVirtualFilesEnabled(true);
    }
}

void AccountSettings::slotDisableVfsCurrentFolder(Folder *folder)
{
    auto msgBox = new QMessageBox(
        QMessageBox::Question,
        tr("Disable virtual file support?"),
        tr("This action will disable virtual file support. As a consequence contents of folders that "
           "are currently marked as 'available online only' will be downloaded."
           "\n\n"
           "The only advantage of disabling virtual file support is that the selective sync feature "
           "will become available again."
           "\n\n"
           "This action will abort any currently running synchronization."));
    auto acceptButton = msgBox->addButton(tr("Disable support"), QMessageBox::AcceptRole);
    msgBox->addButton(tr("Cancel"), QMessageBox::RejectRole);
    connect(msgBox, &QMessageBox::finished, msgBox, [msgBox, folder, acceptButton] {
        msgBox->deleteLater();
        if (msgBox->clickedButton() != acceptButton || !folder) {
            return;
        }

        qCInfo(lcAccountSettings) << "Disabling vfs support for folder" << folder->path();

        // Also wipes virtual files, schedules remote discovery
        folder->setVirtualFilesEnabled(false);
    });
    msgBox->open();
}

void AccountSettings::showConnectionLabel(const QString &message, StatusIcon statusIcon, QStringList errors)
{
    if (errors.isEmpty()) {
        ui->connectionStatusLabel->setText(message);
        ui->connectionStatusLabel->setToolTip(QString());
    } else {
        errors.prepend(message);
        const QString msg = errors.join(QLatin1String("\n"));
        qCDebug(lcAccountSettings) << msg;
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

void AccountSettings::slotEnableCurrentFolder(Folder *folder, bool terminate)
{
    Q_ASSERT(folder);
    qCInfo(lcAccountSettings) << "Application: enable folder with alias " << folder->path();
    bool currentlyPaused = false;

    // this sets the folder status to disabled but does not interrupt it.
    currentlyPaused = folder->syncPaused();
    if (!currentlyPaused && !terminate) {
        // check if a sync is still running and if so, ask if we should terminate.
        if (folder->isSyncRunning()) { // its still running
            auto msgbox = new QMessageBox(QMessageBox::Question, tr("Sync Running"), tr("The sync operation is running.<br/>Do you want to stop it?"),
                QMessageBox::Yes | QMessageBox::No, this);
            msgbox->setAttribute(Qt::WA_DeleteOnClose);
            msgbox->setDefaultButton(QMessageBox::Yes);
            connect(msgbox, &QMessageBox::accepted, this, [folder = QPointer<Folder>(folder), this] {
                if (folder) {
                    slotEnableCurrentFolder(folder, true);
                }
            });
            msgbox->open();
            return;
        }
    }

    // message box can return at any time while the thread keeps running,
    // so better check again after the user has responded.
    if (folder->isSyncRunning() && terminate) {
        folder->slotTerminateSync(tr("Sync paused by user"));
    }
    folder->slotNextSyncFullLocalDiscovery(); // ensure we don't forget about local errors
    folder->setSyncPaused(!currentlyPaused);

    // keep state for the icon setting.
    if (currentlyPaused)
        _wasDisabledBefore = true;

    _model->slotUpdateFolderState(folder);
}

void AccountSettings::slotForceSyncCurrentFolder(Folder *folder)
{
    if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
        auto messageBox = new QMessageBox(QMessageBox::Question, tr("Internet connection is metered"),
            tr("Synchronization is paused because the Internet connection is a metered connection"
               "<p>Do you really want to force a Synchronization now?"),
            QMessageBox::Yes | QMessageBox::No, ocApp()->gui()->settingsDialog());
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        connect(messageBox, &QMessageBox::accepted, this, [folder = QPointer<Folder>(folder), this] {
            if (folder) {
                doForceSyncCurrentFolder(folder);
            }
        });
        ownCloudGui::raise();
        messageBox->open();
    } else {
        doForceSyncCurrentFolder(folder);
    }
}

void AccountSettings::doForceSyncCurrentFolder(Folder *selectedFolder)
{
    // Prevent new sync starts
    FolderMan::instance()->scheduler()->stop();

    // Terminate and reschedule any running sync
    for (auto *folder : FolderMan::instance()->folders()) {
        if (folder->isSyncRunning()) {
            folder->slotTerminateSync(tr("User triggered force sync"));
            FolderMan::instance()->scheduler()->enqueueFolder(folder);
        }
    }

    selectedFolder->slotWipeErrorBlacklist(); // issue #6757
    selectedFolder->slotNextSyncFullLocalDiscovery(); // ensure we don't forget about local errors

    // Insert the selected folder at the front of the queue
    FolderMan::instance()->scheduler()->enqueueFolder(selectedFolder, SyncScheduler::Priority::High);

    // Restart scheduler
    FolderMan::instance()->scheduler()->start();
}

// Refactoring todo: the signal sends the new account state, refactor this to use that param
void AccountSettings::slotAccountStateChanged()
{
    if (!_accountState) {
        return;
    }

    const AccountState::State state = _accountState->state();
    const AccountPtr account = _accountState->account();
    qCDebug(lcAccountSettings) << "Account state changed to" << state << "for account" << account;

    FolderMan *folderMan = FolderMan::instance();
    for (auto *folder : folderMan->folders()) {
        _model->slotUpdateFolderState(folder);
    }

    switch (state) {
    case AccountState::Connected: {
        QStringList errors;
        StatusIcon icon = StatusIcon::Connected;
        if (account->serverSupportLevel() != Account::ServerSupportLevel::Supported) {
            errors << tr("The server version %1 is unsupported! Proceed at your own risk.").arg(account->capabilities().status().versionString());
            icon = StatusIcon::Warning;
        }
        showConnectionLabel(tr("Connected"), icon, errors);
        if (_accountState->supportsSpaces()) {
            connect(_accountState->account()->spacesManager(), &GraphApi::SpacesManager::updated, this, &AccountSettings::slotSpacesUpdated,
                Qt::UniqueConnection);
            // Refactoring todo: won't this get called every time the state changes to connected even if the spaces manager is already
            // triggering the slot? ie duplicate call to slotSpacesUpdated?
            slotSpacesUpdated();
        }
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
        showConnectionLabel(tr("Updating credentials..."), StatusIcon::Info);
        break;
    }
    case AccountState::Connecting:
        if (NetworkInformation::instance()->isBehindCaptivePortal()) {
            showConnectionLabel(tr("Captive portal prevents connections to the server."), StatusIcon::Disconnected);
        } else if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
            showConnectionLabel(tr("Sync is paused due to metered internet connection"), StatusIcon::Disconnected);
        } else {
            showConnectionLabel(tr("Connecting..."), StatusIcon::Info);
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

void AccountSettings::slotSpacesUpdated()
{
    if (!_accountState || !_accountState->supportsSpaces()) {
        // oC10 does not support spaces, and there is no `SpacesManager` available.
        return;
    }

    auto spaces = _accountState->account()->spacesManager()->spaces();
    auto unsycnedSpaces = std::set<GraphApi::Space *>(spaces.cbegin(), spaces.cend());
    for (const auto &f : std::as_const(FolderMan::instance()->folders())) {
        unsycnedSpaces.erase(f->space());
    }

    // Check if we should add new spaces automagically, or only signal that there are unsynced spaces.
    // Refactoring todo answer to above: we should not be loading spaces in any gui. Instead I would expect that the gui merely updates
    // it's view/state in response to new or removed spaces. The clue is that the main actor here is FolderMan - connect FolderMan to whoever
    // emits the newly discovered spaces (or whatever this is) and let it deal with it.
    // A wrinkle here is that this slot is called in several places in the gui, apparently just to trigger refresh or similar? Not good!
    if (Theme::instance()->syncNewlyDiscoveredSpaces()) {
        // Refactoring todo: why is this scheduled for "later" on the main event loop? aren't we already there?
        // where does this slot run if not on the main thread?
        // what needs to be processed "before" this loading routine that requires scheduling it for later?
        // if anything we should consider running the loading routines in a worker thread to avoid *blocking* the main
        // event loop.
        QTimer::singleShot(0, this, [this, unsycnedSpaces]() {
            for (GraphApi::Space *newSpace : unsycnedSpaces) {
                // TODO: Problem: when a space is manually removed, this will re-add it!
                qCInfo(lcAccountSettings) << "Adding sync connection for newly discovered space" << newSpace->displayName();

                const QString localDir(_accountState->account()->defaultSyncRoot());
                const QString folderName = FolderMan::instance()->findGoodPathForNewSyncFolder(
                    localDir, newSpace->displayName(), FolderMan::NewFolderType::SpacesFolder, _accountState->account()->uuid());

                FolderMan::SyncConnectionDescription fwr;
                fwr.davUrl = QUrl(newSpace->drive().getRoot().getWebDavUrl());
                fwr.spaceId = newSpace->drive().getRoot().getId();
                fwr.localPath = folderName;
                fwr.displayName = newSpace->displayName();
                fwr.useVirtualFiles = Utility::isWindows() ? Theme::instance()->showVirtualFilesOption() : false;
                fwr.priority = newSpace->priority();
                FolderMan::instance()->addFolderFromGui(_accountState, fwr);
            }

            _unsyncedSpaces = 0;
            _syncedSpaces = _accountState->account()->spacesManager()->spaces().size();
            Q_EMIT unsyncedSpacesChanged();
            Q_EMIT syncedSpacesChanged();
        });
    } else {
        if (_unsyncedSpaces != unsycnedSpaces.size()) {
            _unsyncedSpaces = static_cast<uint>(unsycnedSpaces.size());
            Q_EMIT unsyncedSpacesChanged();
        }
        uint syncedSpaces = spaces.size() - _unsyncedSpaces;
        if (_syncedSpaces != syncedSpaces) {
            _syncedSpaces = syncedSpaces;
            Q_EMIT syncedSpacesChanged();
        }
    }
}

AccountSettings::~AccountSettings()
{
    _goingDown = true;
    delete ui;
}

void AccountSettings::addModalLegacyDialog(QWidget *widget, ModalWidgetSizePolicy sizePolicy)
{
    if (!widget->testAttribute(Qt::WA_DeleteOnClose)) { // DEBUG CODE! See https://github.com/owncloud/client/issues/11673
        // Early check to see if the attribute gets unset before the second/real check below
        qCWarning(lcAccountSettings) << "Missing WA_DeleteOnClose! (1)" << widget->metaObject() << widget;
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
        qCWarning(lcAccountSettings) << "Missing WA_DeleteOnClose! (2)" << widget->metaObject() << widget;
    }
    Q_ASSERT(widget->testAttribute(Qt::WA_DeleteOnClose));

    // Refactoring todo: eval this more completely
    Q_ASSERT(_accountState);

    connect(widget, &QWidget::destroyed, this, [this, outerWidget] {
        outerWidget->deleteLater();
        if (!_goingDown) {
            ocApp()->gui()->settingsDialog()->ceaseModality(_accountState->account().get());
        }
    });
    widget->setVisible(true);
    ocApp()->gui()->settingsDialog()->requestModality(_accountState->account().get());
}

void AccountSettings::addModalWidget(AccountModalWidget *widget)
{
    if (!_accountState) {
        return;
    }

    ui->stackedWidget->addWidget(widget);
    ui->stackedWidget->setCurrentWidget(widget);

    connect(widget, &AccountModalWidget::finished, this, [widget, this] {
        widget->deleteLater();
        ocApp()->gui()->settingsDialog()->ceaseModality(_accountState->account().get());
    });
    ocApp()->gui()->settingsDialog()->requestModality(_accountState->account().get());
}

uint AccountSettings::unsyncedSpaces() const
{
    return _unsyncedSpaces;
}

uint AccountSettings::syncedSpaces() const
{
    return _syncedSpaces;
}

void AccountSettings::slotDeleteAccount()
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

bool AccountSettings::event(QEvent *e)
{
    if (e->type() == QEvent::Hide || e->type() == QEvent::Show) {
        if (_accountState && !_accountState->supportsSpaces()) {
            _accountState->quotaInfo()->setActive(isVisible());
        }
    }
    return QWidget::event(e);
}

} // namespace OCC
