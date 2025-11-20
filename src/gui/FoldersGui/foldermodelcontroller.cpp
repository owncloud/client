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

#include "foldermodelcontroller.h"

#include "folder.h"
#include "folderitem.h"

#include <QStandardItemModel>

namespace OCC {


FolderModelController::FolderModelController(const QUuid &accountId, QObject *parent)
    : QObject(parent)
    , _accountId(accountId)
{
    _model = new QStandardItemModel(this);
}

void FolderModelController::onFolderListChanged(const QUuid &accountId, const QList<Folder *> folders)
{
    if (!accountId.isNull() && _accountId != accountId)
        return;
    // important to understand: the first check is just the identify whether we are associated with a non-null account
    // the change was related it.

    // if the accountId is null it means that ALL folders, regardless of account, were removed, eg on shutdown
    // so we still need to reset the model even if the accountId is null
    _model->clear();

    if (folders.isEmpty())
        return;

    for (const auto &f : folders)
        onFolderAdded(accountId, f);
}

void FolderModelController::onFolderAdded(const QUuid &accountId, Folder *folder)
{
    if (!folder || folder->id().isEmpty() || accountId != _accountId)
        return;

    FolderItem *item = new FolderItem(folder);
    _model->appendRow(item);
    _items.insert(folder->id(), item);
}

void FolderModelController::onFolderRemoved(const QUuid &accountId, Folder *folder)
{
    if (!folder || accountId != _accountId)
        return;

    QByteArray id = folder->id();
    if (id.isEmpty())
        return;

    if (_items.contains(id)) {
        QStandardItem *it = _items.value(folder->id());
        QModelIndex index = it->index();
        if (index.isValid())
            _model->removeRow(it->index().row());
        _items.remove(id);
    }
    // _folders.remove(id);
}

}
