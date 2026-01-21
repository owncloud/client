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

#include "QStandardItem"

// #include "foldererroritem.h"
#include "folder.h"
#include "progressdispatcher.h"

#include <QList>
#include <QPointer>

// #include "folderstatusupdater.h"

namespace OCC {

class FolderItemUpdater;

enum FolderItemRoles { DataRole = Qt::UserRole, StatusIconRole, StatusStringRole, StatusInfoRole };

class FolderItem : public QStandardItem
{
public:
    FolderItem(Folder *folder);
    ~FolderItem() override;

    QVariant data(int role) const override;

    void refresh();

    void setProgress(const ProgressInfo &progress);

    // SyncProgress _progress;

    Folder *folder();

private:
    QPointer<Folder> _folder;
    FolderItemUpdater *_updater = nullptr;

    quint64 _totalSize = 0;
    quint64 _completedSize = 0;
    QString _statusString;
    // ProgressInfo _progress;
    //  void updateProgress(OCC::Folder *folder, const OCC::ProgressInfo &progress);

    QString statusIconName() const;
    QString statusAsString() const;
    void updateStatusString();
};
}
