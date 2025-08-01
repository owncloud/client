/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "gui/scheduling/etagwatcher.h"

#include "accountstate.h"
#include "gui/folderman.h"
#include "libsync/graphapi/spacesmanager.h"
#include "libsync/syncengine.h"


using namespace std::chrono_literals;

using namespace OCC;

Q_LOGGING_CATEGORY(lcEtagWatcher, "gui.scheduler.etagwatcher", QtInfoMsg)

ETagWatcher::ETagWatcher(FolderMan *folderMan, QObject *parent)
    : QObject(parent)
    , _folderMan(folderMan)
{
    // Refactoring todo: use folderAdded/folderAboutToBeRemoved signals when implemented on the FolderMan
    connect(folderMan, &FolderMan::folderListChanged, this, &ETagWatcher::slotFolderListChanged);
}

void ETagWatcher::slotSpaceChanged(GraphApi::Space *space)
{
    QString spaceId = space->id();
    auto it = _lastEtagJobForSpace.find(spaceId);
    if (it != _lastEtagJobForSpace.cend()) {
        QString etag = Utility::normalizeEtag(space->drive().getRoot().getETag());
        updateEtag(spaceId, etag);
    }
}

void ETagWatcher::slotFolderListChanged()
{
    decltype(_lastEtagJobForSpace) newMap;

    for (auto *f : _folderMan->folders()) {
        if (f->accountState() && f->isReady()) {
            connect(f->accountState()->account()->spacesManager(), &GraphApi::SpacesManager::spaceChanged, this, &ETagWatcher::slotSpaceChanged,
                Qt::UniqueConnection);

            QString spaceId = f->definition().spaceId();

            auto it = _lastEtagJobForSpace.find(spaceId);
            if (it != _lastEtagJobForSpace.cend()) {
                newMap[spaceId] = std::move(it->second);
            } else {
                newMap[spaceId] = {};
                newMap[spaceId].folder = f;
                connect(&f->syncEngine(), &SyncEngine::rootEtag, this, [f, this](const QString &etag, const QDateTime &time) {
                    QString spaceId = f->definition().spaceId();
                    auto &info = _lastEtagJobForSpace[spaceId];
                    info.etag = etag;
                    info.lastUpdate.reset();
                    if (f->accountState()) {
                        f->accountState()->tagLastSuccessfulETagRequest(time);
                    }
                });
            }
        }
    }

    _lastEtagJobForSpace = std::move(newMap);
}

void ETagWatcher::updateEtag(const QString &spaceId, const QString &etag)
{
    // the server must provide a valid etag but there might be bugs
    // https://github.com/owncloud/ocis/issues/7160
    if (OC_ENSURE_NOT(etag.isEmpty())) {
        auto &info = _lastEtagJobForSpace[spaceId];
        if (info.folder->canSync() && info.etag != etag) {
            const bool folderWasSyncedOnce = !info.etag.isEmpty();
            info.etag = etag;

            if (folderWasSyncedOnce) {
                // Only schedule a sync if the folder finished its initial sync.
                qCDebug(lcEtagWatcher) << "Scheduling sync of" << info.folder->displayName() << info.folder->path() << "due to an etag change";
                _folderMan->scheduler()->enqueueFolder(info.folder);
            }
        }
        info.lastUpdate.reset();
    } else {
        qCWarning(lcEtagWatcher) << "Invalid empty etag received for space" << spaceId;
    }
}
