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

#include "commonstrings.h"
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
    // first column is the real data, which is in a FolderItem, the second item is just a dumb QStandardItem that is basically
    // a placeholder that can host the ButtonDelegate.
    _model->setColumnCount(2);
    // we have to add something to the model header to allow us to pretend we can manipulate the logical indexes before real data has been added
    // to the model. Just setting the column count does nothing. Also note these get deleted on model clear so DON'T USE CLEAR
    _model->setHorizontalHeaderItem(0, new QStandardItem("bling"));
    _model->setHorizontalHeaderItem(1, new QStandardItem("boop"));
    Q_ASSERT(_model->columnCount() == 2);
    _model->setSortRole(FolderItemRoles::SortPriorityRole);

    // you can't get the selection model from the model. you have to get it from the view. this creates dependency ick so instead just create
    // our own selection model and set it on the view via setModels
    _selectionModel = new QItemSelectionModel(_model, this);
    connect(_selectionModel, &QItemSelectionModel::currentColumnChanged, this, &FolderModelController::onCurrentChanged);
    connect(_selectionModel, &QItemSelectionModel::currentRowChanged, this, &FolderModelController::onCurrentChanged);
}

void FolderModelController::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (current.parent().isValid()) {
        emit currentFolderChanged(nullptr);
        return;
    }
    if (current.row() == previous.row()) {
        _selectionModel->setCurrentIndex(_model->index(current.row(), 1), QItemSelectionModel::Current);
        return;
    }
    // todo: don't bother with all this casting nonsense, just make a new type for folder items
    QStandardItem *item = _model->item(current.row(), 0);
    FolderItem *folderItem = dynamic_cast<FolderItem *>(item);
    if (folderItem) {
        if (current.column() != 1)
            _selectionModel->setCurrentIndex(_model->index(current.row(), 1), QItemSelectionModel::Current);
        emit currentFolderChanged(folderItem->folder());
    } else {
        // at the moment we won't support a "current folder" value if a non-folder item is selected.
        emit currentFolderChanged(nullptr);
    }
}

void FolderModelController::onFolderListChanged(const QUuid &accountId, const QList<Folder *> folders)
{
    // first check is just the identify whether new folder set is associated with the current account, which should not be null
    if (!accountId.isNull() && _accountId != accountId)
        return;

    // if the accountId is null it should mean that ALL folders in the system, regardless of account id, were removed, eg on shutdown
    // so we still need to clear the model even if the accountId is null. In this case the whole model needs to be cleared
    if (accountId.isNull()) {
        Q_ASSERT(folders.isEmpty());
        _model->clear();
        return;
    }

    // otherwise we *can't* reset the model or we lose the headers and the associated column sizing >:|
    _model->invisibleRootItem()->removeRows(0, _model->rowCount());

    _multiLoad = true;
    for (const auto &f : folders)
        onFolderAdded(accountId, f);
    _multiLoad = false;

    _model->sort(0, Qt::DescendingOrder);
    // be sure nothing is selected on first load. see the kooky handling in AccountFoldersView handleEvent - it is "necessary" to
    // set and then clear the selection on ShowToParent so going in, selection should be empty. yes this needs improvement but
    // the nature of always having current row in edit mode is super complicated
    _selectionModel->clear();
}

void FolderModelController::onFolderAdded(const QUuid &accountId, Folder *folder)
{
    if (!folder || folder->id().isEmpty() || accountId != _accountId)
        return;

    FolderItem *item = new FolderItem(folder);
    QStandardItem *buttonItem = new QStandardItem();
    buttonItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    buttonItem->setAccessibleText(tr("%1 options").arg(CommonStrings::space()));
    buttonItem->setAccessibleDescription(tr("Menu button with %1 options. Hit the space key to pop the folder options menu").arg(CommonStrings::space()));

    // we need a dummy item for the second column to host the button delegate for the row
    _model->appendRow({item, buttonItem});
    _items.insert(folder->spaceId(), item);

    if (!_multiLoad) {
        _model->sort(0, Qt::DescendingOrder);
        _selectionModel->setCurrentIndex(item->index(), QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    }
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
        QModelIndex previousIndex = _model->sibling(index.row() - 1, 0, index);
        if (index.isValid())
            // note that removeRow(s) takes care of deleting the items in the row
            _model->removeRow(it->index().row());
        _items.remove(id);
        // should not need to sort on remove but it's helpful to update the selection to previous item:
        _selectionModel->setCurrentIndex(previousIndex, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    }   
}

void FolderModelController::connectSignals(FolderMan *folderMan)
{
    connect(folderMan, &FolderMan::folderListChanged, this, &FolderModelController::onFolderListChanged);
    connect(folderMan, &FolderMan::folderAdded, this, &FolderModelController::onFolderAdded);
    connect(folderMan, &FolderMan::folderRemoved, this, &FolderModelController::onFolderRemoved);

    // normally the folder list is updated after the views have been created but just in case it
    // happened sooner:
    // onFolderListChanged(_accountId, folderMan->foldersForAccount(_accountId));
    // so far I never see it happening sooner but may revisit this.
}
}
