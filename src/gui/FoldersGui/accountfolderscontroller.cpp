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
#include "accountmodalwidget.h"
#include "application.h"
#include "commonstrings.h"
#include "configfile.h"
#include "foldermodelcontroller.h"
#include "folderwizard.h"
#include "selectivesyncwidget.h"
#include "settingsdialog.h"

#include "guiutility.h"
#include "networkjobs.h"
#include "openfilemanager.h"

#include <QMessageBox>
#include <QtWidgets/qabstractbutton.h>

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
    connect(modelController, &FolderModelController::currentFolderChanged, this, &AccountFoldersController::onFolderChanged);

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

    _view->setItemModels(model, modelController->selectionModel());

    buildMenuActions();

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

void AccountFoldersController::onFolderChanged(OCC::Folder *folder)
{
    _currentFolder = folder;
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

void AccountFoldersController::slotFolderWizardAccepted(OCC::FolderMan::SyncConnectionDescription result)
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

void AccountFoldersController::buildMenuActions()
{
    QList<QAction *> itemActions;

    // show in finder
    _showInSystemFolder = new QAction(CommonStrings::showInFileBrowser(), this);
    itemActions.push_back(_showInSystemFolder);
    connect(_showInSystemFolder, &QAction::triggered, this, &AccountFoldersController::onShowInSystemFolder);

    if (_accountState->account()->capabilities().privateLinkPropertyAvailable()) {
        _showInBrowser = new QAction(CommonStrings::showInWebBrowser(), this);
        itemActions.push_back(_showInBrowser);
        connect(_showInBrowser, &QAction::triggered, this, &AccountFoldersController::onShowInBrowser);
    }

    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    itemActions.push_back(separator);

    _forceSync = new QAction(tr("Force sync now"), this);
    itemActions.push_back(_forceSync);
    connect(_forceSync, &QAction::triggered, this, &AccountFoldersController::onForceSync);

    _pauseSync = new QAction(this);
    itemActions.push_back(_pauseSync);
    connect(_pauseSync, &QAction::triggered, this, &AccountFoldersController::onTogglePauseSync);

    _removeSync = new QAction(tr("Remove folder sync connection"), this);
    itemActions.push_back(_removeSync);
    connect(_removeSync, &QAction::triggered, this, &AccountFoldersController::onRemoveSync);

    // we may want to treat this similar to the enable vfs option, because choose sync can only be activated
    // if the folder is not using vfs. So the idea is if force vfs is on the user will never be able to choose
    // so showing the option at all is probably not great
    _chooseSync = new QAction(tr("Choose what to sync"), this);
    itemActions.push_back(_chooseSync);
    connect(_chooseSync, &QAction::triggered, this, &AccountFoldersController::onChooseSync);


    // Only show "Enable VFS" if a VFS mode is available
    /*  const auto mode = VfsPluginManager::instance().bestAvailableVfsMode();
      if (mode == Vfs::WindowsCfApi) {
          _enableVfs = new QAction(tr("Enable virtual file support"), this);

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
      _enableVfs = new QAction(this);
    */

    connect(_view, &AccountFoldersView::requestActionsUpdate, this, &AccountFoldersController::updateActions);
    _view->setFolderActions(itemActions);
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


void AccountFoldersController::updateActions()
{
    if (!_currentFolder) // probably disable all actions here
        return;

    _showInSystemFolder->setEnabled(QFileInfo::exists(_currentFolder->path()));

    if (_showInBrowser)
        _showInBrowser->setEnabled(_currentFolder->isAvailable());

    _forceSync->setEnabled(!_currentFolder->isSyncRunning() && _currentFolder->canSync());

    _pauseSync->setText(_currentFolder->syncPaused() ? tr("Resume sync") : tr("Pause sync"));
    // is this enough? I think we need to keep it enabled even if sync is running so the user can effectively cancel it + pause. as who knows. maybe
    // they made a mistake when adding the folder or whatever
    _pauseSync->setEnabled(_currentFolder->isAvailable());

    _chooseSync->setEnabled(!_currentFolder->virtualFilesEnabled());
}

void OCC::AccountFoldersController::onShowInSystemFolder()
{
    if (!_currentFolder)
        return;

    if (QFileInfo::exists(_currentFolder->path()))
        showInFileManager(_currentFolder->path());
}

void AccountFoldersController::onShowInBrowser()
{
    if (!_currentFolder)
        return;

    QString path = _currentFolder->path();
    QUrl davUrl = _currentFolder->webDavUrl();
    fetchPrivateLinkUrl(_accountState->account(), davUrl, path, this, [](const QUrl &url) { Utility::openBrowser(url, nullptr); });
}

void AccountFoldersController::onForceSync()
{
    if (!_currentFolder)
        return;

    if (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered()) {
        QMessageBox messageBox(QMessageBox::Question, tr("Internet connection is metered"),
            tr("Synchronization is paused because the Internet connection is a metered connection"
               "<p>Do you really want to force a Synchronization now?"),
            QMessageBox::Yes | QMessageBox::No, ocApp()->gui()->settingsDialog());
        messageBox.setAttribute(Qt::WA_DeleteOnClose);
        int result = messageBox.exec();
        if (result == QDialog::Rejected)
            return;
    }

    FolderMan::instance()->forceFolderSync(_currentFolder);
}

void AccountFoldersController::onTogglePauseSync()
{
    if (!_currentFolder)
        return;

    bool currentlyPaused = _currentFolder->syncPaused();

    if (!currentlyPaused) {
        if (_currentFolder->isSyncRunning()) {
            QMessageBox msgbox(QMessageBox::Question, tr("Sync Running"), tr("The sync operation is running.<br/>Do you want to stop it?"),
                QMessageBox::Yes | QMessageBox::No, ocApp()->gui()->settingsDialog());
            msgbox.setDefaultButton(QMessageBox::No);
            int result = msgbox.exec();
            if (result == QDialog::Rejected)
                return;
            _currentFolder->slotTerminateSync(tr("Sync paused by user"));
        }
    }

    // todo: what is this really for? if we are pausing do we actually need it or only on resume?
    // if we do need it, it should probably move to the setSyncPaused function so it never gets missed
    _currentFolder->slotNextSyncFullLocalDiscovery(); // ensure we don't forget about local errors
    _currentFolder->setSyncPaused(!currentlyPaused);
}

void AccountFoldersController::onRemoveSync()
{
    if (!_currentFolder)
        return;

    qCInfo(lcAccountFoldersController) << "Remove Folder " << _currentFolder->path();
    QString shortGuiLocalPath = _currentFolder->shortGuiLocalPath();

    QMessageBox msgBox(QMessageBox::Question, tr("Confirm Folder Sync Connection Removal"),
        tr("<p>Do you really want to stop syncing the folder <i>%1</i>?</p>"
           "<p><b>Note:</b> This will <b>not</b> delete any files.</p>")
            .arg(shortGuiLocalPath),
        QMessageBox::Yes | QMessageBox::No, ocApp()->gui()->settingsDialog());
    msgBox.button(QMessageBox::Yes)->setText(tr("Remove Folder Sync Connection"));
    msgBox.button(QMessageBox::No)->setText(tr("Cancel"));

    int result = msgBox.exec();
    if (result == QDialog::Rejected)
        return;
    // todo: make it a signal
    FolderMan::instance()->removeFolderFromGui(_currentFolder);
}

void AccountFoldersController::onChooseSync()
{
    if (!_currentFolder || _accountState || !_accountState->account()) {
        return;
    }

    bool ok;
    QSet<QString> selectiveSyncList = _currentFolder->journalDb()->getSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, ok);
    if (!ok)
        return;

    // this widget gets reparented to the layout in the AccountModalWidget so should be cleaned up there
    auto *selectiveSync = new SelectiveSyncWidget(_accountState->account(), nullptr);
    selectiveSync->setDavUrl(_currentFolder->webDavUrl());
    selectiveSync->setFolderInfo(_currentFolder->remotePath(), _currentFolder->displayName(), selectiveSyncList);

    // the account modal widget is auto-deleted by the handler, currently the AccountView, when it emits "finished"
    // as with so many of these async impls the details are way too complicated but that's a topic for a future refactoring
    auto *modalWidget = new AccountModalWidget(tr("Choose what to sync"), selectiveSync, nullptr);
    modalWidget->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(modalWidget, &AccountModalWidget::accepted, this, [selectiveSync, this] {
        if (!_currentFolder)
            return;
        _currentFolder->journalDb()->setSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, selectiveSync->createBlackList());
        FolderMan::instance()->forceFolderSync(_currentFolder);
    });

    emit requestAccountModalWidget(modalWidget);
}
}
