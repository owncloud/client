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

#include "folder.h"
#include "folderman.h"

#include <QObject>
#include <QPointer>
#include <QUuid>

class QAction;

namespace OCC {

class AccountFoldersView;
class AccountState;
class FolderModelController;
class AccountModalWidget;

class AccountFoldersController : public QObject
{
    Q_OBJECT
public:
    explicit AccountFoldersController(AccountState *state, AccountFoldersView *view, QObject *parent);

    // void onUnsyncedSpacesChanged(const QUuid &accountId, QSet<GraphApi::Space *> unsyncedSpaces, int totalSpaceCount);
    void onUnsyncedSpaceCountChanged(const QUuid &accountId, int unsyncedSpaceCount, int totalSpaceCount);

signals:
    void removeFolderFromGui(OCC::Folder *f);
    void requestShowModalWidget(QWidget *widget);
    void requestAccountModalWidget(OCC::AccountModalWidget *widget);

protected slots:
    void slotAddFolder();
    void slotFolderWizardAccepted(OCC::FolderMan::SyncConnectionDescription result);

private:
    AccountState *_accountState = nullptr;
    QUuid _accountId;
    QPointer<Folder> _currentFolder = nullptr;
    AccountFoldersView *_view = nullptr;
    FolderModelController *_modelController = nullptr;

    void buildMenuActions();

    void updateActions();

    // menu actions:
    QAction *_showInSystemFolder = nullptr;
    QAction *_showInBrowser = nullptr;
    QAction *_forceSync = nullptr;
    QAction *_pauseSync = nullptr;
    QAction *_removeSync = nullptr;
    QAction *_chooseSync = nullptr;
    QAction *_enableVfs = nullptr;

    void onFolderChanged(OCC::Folder *folder);

    void onShowInSystemFolder();
    void onShowInBrowser();
    void onForceSync();
    void onTogglePauseSync();
    void onRemoveSync();
    void onChooseSync();
    void onEnableVfs();
};
}
