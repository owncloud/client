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

#include "folderman.h"

#include <QObject>
#include <QUuid>

namespace OCC {

class AccountFoldersView;
class AccountState;
class FolderModelController;
class Folder;

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

protected slots:
    void slotAddFolder();
    void slotFolderWizardAccepted(FolderMan::SyncConnectionDescription result);

private:
    AccountState *_accountState = nullptr;
    QUuid _accountId;
    AccountFoldersView *_view = nullptr;
    FolderModelController *_modelController = nullptr;
};
}
