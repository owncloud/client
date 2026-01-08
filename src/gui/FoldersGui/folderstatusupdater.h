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

#include "folder.h"
#include "progressdispatcher.h"
#include <QObject>
#include <QPointer>


namespace OCC {

class FolderItem;
/// class ProgressDispatcher;

struct SyncProgress
{
    bool isNull() const { return _progressString.isEmpty() && _overallSyncString.isEmpty(); }
    QString _progressString;
    QString _overallSyncString;
    float _overallPercent = 0;

    std::chrono::steady_clock::time_point _lastProgressUpdated = std::chrono::steady_clock::now();
    ProgressInfo::Status _lastProgressUpdateStatus = ProgressInfo::None;
};

class FolderStatusUpdater : public QObject
{
    Q_OBJECT
public:
    explicit FolderStatusUpdater(FolderItem *item, Folder *folder, QObject *parent);

protected slots:
    void updateProgress(OCC::Folder *folder, const ProgressInfo &progress);

private:
    FolderItem *_item;
    QPointer<Folder> _folder;
};

}
