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
   QItemSelectionModel *selectionModel() const { return _selectionModel; }

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
   bool _multiLoad = false;
};
}
