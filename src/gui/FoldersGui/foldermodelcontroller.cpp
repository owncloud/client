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
#include "folderman.h"

#include <QItemSelectionModel>
#include <QStandardItemModel>

namespace OCC {


FolderModelController::FolderModelController(const QUuid &accountId, QObject *parent)
    : QObject(parent)
    , _accountId(accountId)
{
    _model = new QStandardItemModel(this);
    _model->setSortRole(FolderItemRoles::SortPriorityRole);
    _selectionModel = new QItemSelectionModel(_model, this);
    connect(_selectionModel, &QItemSelectionModel::currentChanged, this, &FolderModelController::onCurrentChanged);
}

void FolderModelController::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    QStandardItem *item = _model->itemFromIndex(current);
    FolderItem *folderItem = dynamic_cast<FolderItem *>(item);
    if (folderItem)
        emit currentFolderChanged(folderItem->folder());
    else
        // at the moment we won't support a "current folder" value if a child error is current/focused -> this will ensure no context menu can pop outside of a
        // normal folder row. revisit if we need to expand this concept
        emit currentFolderChanged(nullptr);
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

    _model->sort(0, Qt::DescendingOrder);
}

void FolderModelController::onFolderAdded(const QUuid &accountId, Folder *folder)
{
    if (!folder || folder->id().isEmpty() || accountId != _accountId)
        return;

    FolderItem *item = new FolderItem(folder);
    _model->appendRow(item);
    _items.insert(folder->spaceId(), item);

    _model->sort(0, Qt::DescendingOrder);
    _selectionModel->setCurrentIndex(item->index(), QItemSelectionModel::SelectCurrent);
}

void FolderModelController::onFolderRemoved(const QUuid &accountId, Folder *folder)
{
    if (!folder || accountId != _accountId)
        return;

    QString id = folder->spaceId();
    if (id.isEmpty())
        return;

    if (_items.contains(id)) {
        QStandardItem *it = _items.value(folder->spaceId());
        QModelIndex index = it->index();
        if (index.isValid())
            _model->removeRow(it->index().row());
        _items.remove(id);
    }
    // should not need to sort on remove
}

void FolderModelController::connectSignals(FolderMan *folderMan)
{
    connect(folderMan, &FolderMan::folderListChanged, this, &FolderModelController::onFolderListChanged);
    connect(folderMan, &FolderMan::folderAdded, this, &FolderModelController::onFolderAdded);
    connect(folderMan, &FolderMan::folderRemoved, this, &FolderModelController::onFolderRemoved);

    onFolderListChanged(_accountId, folderMan->foldersForAccount(_accountId));
}
}
