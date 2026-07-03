/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

enum FolderItemRoles { DataRole = Qt::UserRole, StatusIconRole, StatusStringRole, StatusInfoRole, SortPriorityRole };

class FolderItem : public QStandardItem
{
    Q_DECLARE_TR_FUNCTIONS(FolderItem)

public:
    FolderItem(Folder *folder);
    ~FolderItem() override;

    QVariant data(int role) const override;

    void refresh();
    void setProgress(const ProgressInfo &progress);
    void updateImage();

    Folder *folder();

private:
    QPointer<Folder> _folder;
    FolderItemUpdater *_updater = nullptr;

    quint64 _totalSize = 0;
    quint64 _completedSize = 0;
    quint64 _estimatedUpBw = 0;
    quint64 _estimatedDownBw = 0;
    int _percentComplete = 0;

    QString _statusString;
    QIcon _image;
    // ProgressInfo _progress;
    //  void updateProgress(OCC::Folder *folder, const OCC::ProgressInfo &progress);

    QString statusIconName() const;
    QString statusAsString() const;
    void updateStatusString();
};
}
