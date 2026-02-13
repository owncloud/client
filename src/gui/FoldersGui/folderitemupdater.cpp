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
#include "folderitemupdater.h"

#include "folder.h"
#include "folderitem.h"
#include "resources.h"

namespace OCC {
FolderItemUpdater::FolderItemUpdater(FolderItem *item)
    : QObject{nullptr}
    , _item(item)
{
    if (_item && _item->folder()) {
        connect(_item->folder(), &Folder::spaceChanged, this, &FolderItemUpdater::onFolderChanged);
        connect(_item->folder(), &Folder::syncStateChange, this, &FolderItemUpdater::onSyncStateChanged);
        if (item->folder()->accountState())
            connect(item->folder()->accountState(), &AccountState::stateChanged, this, &FolderItemUpdater::onFolderChanged);
    }
}

// space changed is stuff like...display name, should not be triggered so often?
void FolderItemUpdater::onFolderChanged()
{
    _item->refresh();
}

void FolderItemUpdater::onSyncStateChanged()
{
    if (!_item->folder())
        return;

    SyncResult::Status status = _item->folder()->syncResult().status();
    if (status == SyncResult::SyncRunning) {
        Q_ASSERT(!_progressInfoConnection); // ie it should not be connected already
        _progressInfoConnection = connect(_item->folder(), &Folder::progressUpdate, this, &FolderItemUpdater::onProgressUpdated);
    } else {
        disconnect(_progressInfoConnection);
    }

    _item->refresh();

    if (status == SyncResult::Error || status == SyncResult::Problem || status == SyncResult::SetupError) {
        QStringList errors = _item->folder()->syncResult().errorStrings();

        if (_item->folder()->syncResult().hasUnresolvedConflicts())
            errors.append(tr("There are unresolved conflicts."));

        for (const QString &error : std::as_const(errors)) {
            QIcon errorIcon = Resources::getCoreIcon("states/warning");
            QStandardItem *errorItem = new QStandardItem(errorIcon, error);
            errorItem->setData(error, Qt::AccessibleTextRole);
            _item->appendRow(errorItem);
        }

    } else if (status == SyncResult::SyncPrepare && _item->hasChildren()) {
        // I expect this check needs refinement - may want to wait until the sync has actually started before removing previous errors
        _item->removeRows(0, _item->rowCount());
    }
}


void FolderItemUpdater::onProgressUpdated(const ProgressInfo &progress)
{
    // I think trying to track overall total sizes using progress is just hopeless because
    // a) when stuff is removed sync totals = 0. This means we can't update a "known total size" using these progress values because
    // they are never negative, to indicate a removal
    // b) when discovery finds that nothing needs to be uploaded/downloaded, the totals are zero. I am not sure why a progress is even reported in this
    // case as nothing is going to "progress" or happen at all! Maybe there is some other value in the progress that let's us identify that
    // c) if sync totals are zero we don't know if it's because something was removed or there was no diff. I just don't see how this can
    // work even if we try to maintain "last known totals" as these dual meaning "zeros" are a big problem.
    // kw does not know total space size so we can't get it there
    // calculating sizes "on the fly" by recursing a directory is expensive and would have to happen constantly, as far as I can tell,
    // due to the problem of not knowing about deletion sizes (without vfs) and not knowing when user hydrates/dehydrates files (with vfs)

    // I don't think we need to worry about the other states - propagation is the state where progress data seems to be provided, but double check this
    if (progress.status() == ProgressInfo::Propagation) {
        // slow down the updates just a bit - the default is too incremental
        if (std::chrono::steady_clock::now() - _lastProgressUpdated > ProgressUpdateTimeout) {
            _item->setProgress(progress);
            _lastProgressUpdated = std::chrono::steady_clock::now();
        }
    } else
        _lastProgressUpdated = {};
}
}
