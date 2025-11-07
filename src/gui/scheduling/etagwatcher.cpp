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
#include "folder.h"
#include "libsync/graphapi/spacesmanager.h"
#include "libsync/syncengine.h"


using namespace std::chrono_literals;

using namespace OCC;

Q_LOGGING_CATEGORY(lcEtagWatcher, "gui.scheduler.etagwatcher", QtInfoMsg)

ETagWatcher::ETagWatcher(QObject *parent)
    : QObject(parent)
{
}

void ETagWatcher::slotSpaceChanged(GraphApi::Space *space)
{
    QString spaceId = space->id();
    if (_lastEtagJobForSpace.contains(spaceId)) {
        QString etag = Utility::normalizeEtag(space->drive().getRoot().getETag());
        updateEtag(spaceId, etag);
    }
}

void ETagWatcher::slotFolderListChanged(const QUuid &accountId, const QList<Folder *> folders)
{
    if (folders.isEmpty()) {
        // if the list is empty and there is no accountId, it means ALL folders have been removed, eg on app shutdown
        if (accountId.isNull())
            _lastEtagJobForSpace.clear();
        else
        // remove all folders associated with the account id because there are no longer any active folders here
        {
            for (const ETagInfo &info : std::as_const(_lastEtagJobForSpace)) {
                Folder *f = info.folder;
                if (f->accountState() && f->accountState()->account() && f->accountState()->account()->uuid() == accountId)
                    _lastEtagJobForSpace.remove(f->definition().spaceId());
            }
        }
        return;
    }

    for (Folder *f : folders)
        onFolderAdded({}, f);
}

void ETagWatcher::onFolderAdded(const QUuid &accountId, Folder *folder)
{
    Q_UNUSED(accountId);

    QString spaceId = folder->definition().spaceId();
    if (_lastEtagJobForSpace.contains(spaceId))
        return;

    if (folder->accountState() && folder->accountState()->account() && folder->isReady()) {
        _lastEtagJobForSpace.insert(spaceId, ETagInfo{{}, folder});

        connect(&folder->syncEngine(), &SyncEngine::rootEtag, this, [folder, this](const QString &etag, const QDateTime &time) {
            QString spaceId = folder->definition().spaceId();
            auto &info = _lastEtagJobForSpace[spaceId];
            info.etag = etag;
            if (folder->accountState()) {
                folder->accountState()->tagLastSuccessfulETagRequest(time);
            }
        });
    }
}

void ETagWatcher::onFolderRemoved(const QUuid &accountId, Folder *folder)
{
    Q_UNUSED(accountId);
    if (folder)
        _lastEtagJobForSpace.remove(folder->definition().spaceId());
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
                emit requestEnqueueFolder(info.folder);
            }
        }
    } else {
        qCWarning(lcEtagWatcher) << "Invalid empty etag received for space" << spaceId;
    }
}
