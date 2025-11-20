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

#include <QObject>
#include <QHash>
#include <QByteArray>
#include <QPointer>
#include <QUuid>

class QStandardItem;
class QStandardItemModel;


namespace OCC {
//class FolderData;
class Folder;

class FolderModelController : public QObject
{
public:
   explicit FolderModelController(const QUuid &accountId, QObject *parent);

    QStandardItemModel *itemModel();

protected slots:
    void onFolderListChanged(const QUuid &accountId, const QList<OCC::Folder *> folders);
    void onFolderAdded(const QUuid &accountId, OCC::Folder *folder);
    void onFolderRemoved(const QUuid &accountId, OCC::Folder *folder);

private:

    QUuid _accountId;
   // QHash<QByteArray, QPointer<Folder>> _folders;
    QHash<QByteArray, QStandardItem *> _items;
    QStandardItemModel *_model;
};
}
