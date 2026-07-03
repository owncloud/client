/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

signals:
    void removeFolderFromGui(OCC::Folder *f);
    void requestAddFolder(QUuid accountId);

    void requestAccountModalWidget(OCC::AccountModalWidget *widget);

protected:
    void onUnsyncedSpaceCountChanged(const QUuid &accountId, int unsyncedSpaceCount, int totalSpaceCount);
    void onAddFolder();

private:
    QPointer<AccountState> _accountState;
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
