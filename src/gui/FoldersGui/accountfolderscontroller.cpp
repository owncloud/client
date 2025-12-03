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


AccountFoldersController::AccountFoldersController(QObject *parent)
    : QObject{parent}
{
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
