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

#include "folderstatusupdater.h"

#include "folderitem.h"
#include "progressdispatcher.h"

using namespace std::chrono_literals;

namespace {
constexpr auto progressUpdateTimeOutC = 1s;
}
namespace OCC {

FolderStatusUpdater::FolderStatusUpdater(FolderItem *item, Folder *folder, QObject *parent)
    : QObject{parent}
    , _item(item)
    , _folder(folder)
{
}

// I think this should just trigger a data changed and we should keep the info in some struct for reference if/when we need it
// that is how the update should be prevented, not by checking to see if the gui is visible :/
void FolderStatusUpdater::updateProgress(OCC::Folder *folder, const ProgressInfo &progress)
{
    if (!_folder || !folder || folder != _folder)
        return;

    // ????? icky
    /*if (!qobject_cast<QWidget *>(QObject::parent())->isVisible()) {
        return; // for https://github.com/owncloud/client/issues/2648#issuecomment-71377909
    }*/

    /* const int folderIndex = indexOf(f);
     if (folderIndex < 0) {
         return;
     }h
     const auto &folder = _folders.at(folderIndex);
 */
    SyncProgress pr = _item->_progress;
    // depending on the use of virtual files or small files this slot might be called very often.
    // throttle the model updates to prevent a needlessly high cpu usage used on ui updates.
    if (pr._lastProgressUpdateStatus != progress.status() || (std::chrono::steady_clock::now() - pr._lastProgressUpdated > progressUpdateTimeOutC)) {
        pr._lastProgressUpdateStatus = progress.status();

        switch (progress.status()) {
        case ProgressInfo::None:
            Q_UNREACHABLE();
            break;
        case ProgressInfo::Discovery:
            if (!progress._currentDiscoveredRemoteFolder.isEmpty()) {
                pr._overallSyncString = tr("Checking for changes in remote '%1'").arg(progress._currentDiscoveredRemoteFolder);
            } else if (!progress._currentDiscoveredLocalFolder.isEmpty()) {
                pr._overallSyncString = tr("Checking for changes in local '%1'").arg(progress._currentDiscoveredLocalFolder);
            }
            break;
        case ProgressInfo::Reconcile:
            pr._overallSyncString = tr("Reconciling changes");
            break;
        case ProgressInfo::Propagation:
            Q_FALLTHROUGH();
        case ProgressInfo::Done: {
            //  computeProgress(progress, pr);
        }
        }
        // Q_EMIT dataChanged(index(folderIndex, 0), index(folderIndex, 0));
        pr._lastProgressUpdated = std::chrono::steady_clock::now();
    }
}

}
