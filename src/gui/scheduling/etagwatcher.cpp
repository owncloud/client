/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "gui/scheduling/etagwatcher.h"

#include "accountstate.h"
#include "folder.h"
#include "libsync/syncengine.h"


using namespace std::chrono_literals;

using namespace OCC;

Q_LOGGING_CATEGORY(lcEtagWatcher, "gui.scheduler.etagwatcher", QtInfoMsg)

ETagWatcher::ETagWatcher(QObject *parent)
    : QObject(parent)
{
}

// todo: #48 - this seems to be triggered on every timer driven sync check for each folder. Investigate.
void ETagWatcher::slotSpaceChanged(GraphApi::Space *space)
{
    QString spaceId = space->id();
    QUuid account = space->accountId();
    if (_lastEtagJobForSpace.contains(account) && _lastEtagJobForSpace[account].contains(spaceId)) {
        QString etag = Utility::normalizeEtag(space->eTag());
        updateEtag(account, spaceId, etag);
    }
}

void ETagWatcher::slotFolderListChanged(const QUuid &accountId, const QList<Folder *> folders)
{
    if (folders.isEmpty()) {
        // if the list is empty and there is no accountId, it means ALL folders have been removed, eg on app shutdown
        if (accountId.isNull())
            _lastEtagJobForSpace.clear();
        else if (_lastEtagJobForSpace.contains(accountId))
            _lastEtagJobForSpace.remove(accountId);
        return;
    }

    for (Folder *f : folders)
        onFolderAdded(accountId, f);
}

void ETagWatcher::onFolderAdded(const QUuid &accountId, Folder *folder)
{
    Q_UNUSED(accountId);

    QString spaceId = folder->spaceId();
    if (_lastEtagJobForSpace.contains(accountId) && _lastEtagJobForSpace[accountId].contains(spaceId))
        return;

    _lastEtagJobForSpace[accountId].insert(spaceId, ETagInfo{{}, folder});

    connect(folder->syncEngine(), &SyncEngine::rootEtagDiscovered, this, [accountId, folder, this](const QString &etag, const QDateTime &time) {
        QString spaceId = folder->spaceId();
        if (_lastEtagJobForSpace.contains(accountId) && _lastEtagJobForSpace[accountId].contains(spaceId)) {
            auto &info = _lastEtagJobForSpace[accountId][spaceId];
            info.etag = etag;
            if (folder->accountState()) {
                folder->accountState()->tagLastSuccessfulETagRequest(time);
            }
        }
    });
}

void ETagWatcher::onFolderRemoved(const QUuid &accountId, Folder *folder)
{
    Q_UNUSED(accountId);
    if (folder && _lastEtagJobForSpace.contains(accountId))
        _lastEtagJobForSpace[accountId].remove(folder->spaceId());
}

void ETagWatcher::updateEtag(const QUuid &accountId, const QString &spaceId, const QString &etag)
{
    // the server must provide a valid etag but there might be bugs
    // https://github.com/owncloud/ocis/issues/7160
    if (!etag.isEmpty() && _lastEtagJobForSpace.contains(accountId) && _lastEtagJobForSpace[accountId].contains(spaceId)) {
        auto &info = _lastEtagJobForSpace[accountId][spaceId];
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
