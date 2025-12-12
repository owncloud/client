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

#include "accountfolderscontroller.h"

#include "accountfoldersview.h"
#include "folder.h"
#include "folderman.h"
#include "foldermodelcontroller.h"
#include "folderwizard.h"

namespace OCC {

Q_LOGGING_CATEGORY(lcAccountFoldersController, "gui.accountfolders.controller", QtInfoMsg)

AccountFoldersController::AccountFoldersController(AccountState *state, AccountFoldersView *view, QObject *parent)
    : QObject{parent}
    , _accountState(state)
    , _view(view)
{
    if (!_view || !_accountState || !_accountState->account())
        return;

    _accountId = _accountState->account()->uuid();
    FolderModelController *modelController = new FolderModelController(_accountId, this);
    QStandardItemModel *model = modelController->itemModel();

    /*   we should probably simplify to use QStandardItemModel::sort with the sort
     *   data role on the folder item specialized to do the calc.
     *
     *   auto weightedModel = new QSortFilterProxyModel(this);
       weightedModel->setSourceModel(model);
       weightedModel->setSortRole(static_cast<int>(FolderStatusModel::Roles::Priority));
       weightedModel->sort(0, Qt::DescendingOrder);

         _sortModel = weightedModel;
     */

    _view->setItemModel(model);

    connect(_view, &AccountFoldersView::addFolderTriggered, this, &AccountFoldersController::slotAddFolder);
    // ui->quickWidget->engine()->addImageProvider(QStringLiteral("space"), new Spaces::SpaceImageProvider(_accountState->account()->spacesManager()));
    // ui->quickWidget->setOCContext(QUrl(QStringLiteral("qrc:/qt/qml/org/ownCloud/gui/qml/FolderDelegate.qml")), this);


    FolderMan *folderMan = FolderMan::instance();
    modelController->connectSignals(folderMan);
    // next step: move the synced folder watching out of folder man and put it in a separate class. this can be used
    // for updating the synced folder count and for managing the unsynced list in the folder wizard
    connect(folderMan, &FolderMan::unsyncedSpaceCountChanged, this, &AccountFoldersController::onUnsyncedSpaceCountChanged);
}

void AccountFoldersController::onUnsyncedSpaceCountChanged(const QUuid &accountId, int unsyncedSpaceCount, int totalSpaceCount)
{
    if (accountId != _accountId)
        return;

    _view->enableAddFolder(unsyncedSpaceCount > 0);
    // sometimes we have folders that are related to a space which has been removed server side. Because of this,
    // sometimes the total "synced" count can be larger than the space count, so min the diff to get something reasonable
    int reasonableCount = qMin(totalSpaceCount - unsyncedSpaceCount, totalSpaceCount);
    _view->setSyncedFolderCount(reasonableCount, totalSpaceCount);
}


void AccountFoldersController::slotAddFolder()
{
    if (!_accountState || !_accountState->account()) {
        return;
    }

    FolderWizard *folderWizard = new FolderWizard(_accountState->account(), nullptr);
    folderWizard->setAttribute(Qt::WA_DeleteOnClose);

    connect(folderWizard, &FolderWizard::folderWizardAccepted, this, &AccountFoldersController::slotFolderWizardAccepted);

    emit requestShowModalWidget(folderWizard);
}

void AccountFoldersController::slotFolderWizardAccepted(FolderMan::SyncConnectionDescription result)
{
    if (!_accountState) {
        return;
    }


    // The gui should not allow users to selectively choose any sync lists if vfs is enabled, but this kind of check was
    // originally in play here so...keep it just in case.
    if (result.useVirtualFiles && !result.selectiveSyncBlackList.empty()) {
        result.selectiveSyncBlackList.clear();
    }

    // Refactoring todo: turn this into a signal/requestAddFolder
    FolderMan::instance()->addFolderFromGui(_accountState, result);
}


/*void AccountView::slotCustomContextMenuRequested(Folder *folder)
{
    // Refactoring todo: we need to eval defensive handling of the account state QPointer in more depth, and I
    // am not able to easily determine what should happen in this handler if the state is null. For now we just assert
    // to make the "source" of the nullptr obvious before it trickles down into sub-areas and causes a crash that's harder
    // to id
    Q_ASSERT(_accountState && _accountState->account());

    // qpointer for async calls
    const auto isDeployed = folder->isDeployed();
    const auto addRemoveFolderAction = [isDeployed, folder, this](QMenu *menu) {
        Q_ASSERT(!isDeployed);
        return menu->addAction(tr("Remove folder sync connection"), this, [folder, this] { slotRemoveCurrentFolder(folder); });
    };


    auto *menu = new QMenu(ui->accountFoldersView);
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
        // accessibility
        menu->setFocus();
        return;
    }
    // Add an action to open the folder in the system's file browser:
    QAction *showInFileManagerAction = menu->addAction(CommonStrings::showInFileBrowser(), [folder]() {
        qCInfo(lcAccountView) << "Opening local folder" << folder->path();
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

uint AccountView::unsyncedSpaces() const
{
    return _unsyncedSpaces;
}

uint AccountView::syncedSpaces() const
{
    return _syncedSpaces;
}*/


/*void AccountFoldersController::showSelectiveSyncDialog(Folder *folder)
{
    if (!_accountState || !_accountState->account()) {
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
    addModalAccountWidget(modalWidget);
} */

/*void AccountFoldersController::slotTogglePauseFolder(Folder *folder, bool terminate)
{
    if (!folder)
        return;

    qCInfo(lcAccountView) << "Application: enable folder with alias " << folder->path();
    bool currentlyPaused = folder->syncPaused();

    // this sets the folder status to disabled but does not interrupt it.
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

    //_model->slotUpdateFolderState(folder);
    // original handler:
    /*void FolderStatusModel::slotUpdateFolderState(Folder *folder)
{
    if (!folder)
        return;
    for (size_t i = 0; i < _folders.size(); ++i) {
        if (_folders.at(i)->_folder == folder) {
            Q_EMIT dataChanged(index(i, 0), index(i, 0));
        }
    }
}
}*/

/*void AccountFoldersController::slotForceSyncFolder(Folder *folder)
{
    if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
        auto messageBox = new QMessageBox(QMessageBox::Question, tr("Internet connection is metered"),
            tr("Synchronization is paused because the Internet connection is a metered connection"
               "<p>Do you really want to force a Synchronization now?"),
            QMessageBox::Yes | QMessageBox::No, ocApp()->gui()->settingsDialog());
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        connect(messageBox, &QMessageBox::accepted, this, [folder = QPointer<Folder>(folder), this] {
            if (folder) {
                doForceSyncFolder(folder);
            }
        });
        ownCloudGui::raise();
        messageBox->open();
    } else {
        doForceSyncCurrentFolder(folder);
    }
}*/

/*void AccountFoldersController::slotRemoveCurrentFolder(Folder *folder)
{
    qCInfo(lcAccountView) << "Remove Folder " << folder->path();
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
            // todo: #3, this should be a signal to folderman
            FolderMan::instance()->removeFolderFromGui(folder);
            // todo:#4
            // QTimer::singleShot(0, this, &AccountView::slotSpacesUpdated);
        }
    });
    messageBox->open();
}*/

/* void AccountFoldersController::doForceSyncFolder(Folder *selectedFolder)
{
    FolderMan::instance()->forceFolderSync(selectedFolder);
}*/

/*void AccountFoldersController::slotEnableVfsCurrentFolder(Folder *folder)
{
    if (OC_ENSURE(VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::WindowsCfApi)) {
        if (!folder) {
            return;
        }
        qCInfo(lcAccountView) << "Enabling vfs support for folder" << folder->path();

        // Change the folder vfs mode and load the plugin
        folder->setVirtualFilesEnabled(true);
    }
}*/

/*void AccountFoldersController::slotDisableVfsCurrentFolder(Folder *folder)
{
    auto msgBox = new QMessageBox(QMessageBox::Question, tr("Disable virtual file support?"),
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

        qCInfo(lcAccountView) << "Disabling vfs support for folder" << folder->path();

        // Also wipes virtual files, schedules remote discovery
        folder->setVirtualFilesEnabled(false);
    });
    msgBox->open();
}*/
}
