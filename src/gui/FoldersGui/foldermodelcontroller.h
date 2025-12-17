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
class QItemSelectionModel;


namespace OCC {
//class FolderData;
class Folder;
class FolderMan;
class FolderItem;

class FolderModelController : public QObject
{
    Q_OBJECT

public:
   explicit FolderModelController(const QUuid &accountId, QObject *parent);

   QStandardItemModel *itemModel() const { return _model; }

   // getting creative to allow connecting to folderman via dependency injection. its a bit weird but let's see how it is
   // note we don't want to pass folderman to the ctr as we only need to connect to it, don't want it to be a member,
   // and this separate connect function makes more sense it that respect
   void connectSignals(FolderMan *folderman);

   signals:
   void currentFolderChanged(OCC::Folder *folder);

   protected slots:
   void onFolderListChanged(const QUuid &accountId, const QList<OCC::Folder *> folders);
   void onFolderAdded(const QUuid &accountId, OCC::Folder *folder);
   void onFolderRemoved(const QUuid &accountId, OCC::Folder *folder);
   void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

   private:
   QUuid _accountId;
   QHash<QString, QStandardItem *> _items;
   QStandardItemModel *_model = nullptr;
   QItemSelectionModel *_selectionModel = nullptr;
};
}
