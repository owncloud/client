/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "folder.h"

namespace OCC {

class SyncJournalDb;
class SyncEngine;
class Vfs;

class FolderBuilder : public QObject
{
    Q_OBJECT

public:
    FolderBuilder(const FolderDefinition &definition, QObject *parent = nullptr);

    Folder *buildFolder(AccountState *accountState, bool ignoreHiddenFiles, bool moveToTrash, QObject *parent);


private:
    SyncJournalDb *buildJournal();
    Vfs *buildVfs();
    SyncEngine *buildEngine(Account *account, SyncJournalDb *journal, bool ignoreHiddenFiles, bool moveToTrash);

    FolderDefinition _definition;
};
}
