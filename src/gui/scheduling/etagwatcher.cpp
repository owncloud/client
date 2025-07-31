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
#include "libsync/configfile.h"
#include "libsync/graphapi/spacesmanager.h"
#include "libsync/syncengine.h"


using namespace std::chrono_literals;

using namespace OCC;

namespace {
constexpr auto pollTimeoutC = 30s;
}

Q_LOGGING_CATEGORY(lcEtagWatcher, "gui.scheduler.etagwatcher", QtInfoMsg)

ETagWatcher::ETagWatcher(FolderMan *folderMan, QObject *parent)
    : QObject(parent)
    , _folderMan(folderMan)
{
    // Refactoring todo: where/when are info's for removed folders cleaned up? or adds? really only on full folder list changed?
    // this can/should be incrementally managed instead of constantly moving entries around like this
    connect(folderMan, &FolderMan::folderListChanged, this, [this] {
        decltype(_lastEtagJob) intersection;
        for (auto *f : _folderMan->folders()) {
            if (f->accountState() && f->isReady()) {
                auto it = _lastEtagJob.find(f);
                if (it != _lastEtagJob.cend()) {
                    intersection[f] = std::move(it->second);
                } else {
                    intersection[f] = {};
                    connect(&f->syncEngine(), &SyncEngine::rootEtag, this, [f, this](const QString &etag, const QDateTime &time) {
                        auto &info = _lastEtagJob[f];
                        info.etag = etag;
                        info.lastUpdate.reset();
                        if (f->accountState()) {
                            f->accountState()->tagLastSuccessfulETagRequest(time);
                        }
                    });
                }
            }
        }
        _lastEtagJob = std::move(intersection);
    });

    auto *pollTimer = new QTimer(this);
    pollTimer->setInterval(pollTimeoutC);
    // check wheter we need to query the etag for oc10 servers
    connect(pollTimer, &QTimer::timeout, this, [this] {
        for (auto &info : _lastEtagJob) {
            if (!info.first->accountState()) {
                continue;
            }

            // for spaces we use the etag provided by the SpaceManager
            Q_ASSERT(info.first->accountState()->supportsSpaces());

            // we could also connect to the spaceChanged signal but for now this will keep it closer to oc10
            // ensure we already know about the space (startup)
            if (auto *space = info.first->space()) {
                updateEtag(info.first, space->drive().getRoot().getETag());
            }
        }
    });
    pollTimer->start();
}

void ETagWatcher::updateEtag(Folder *f, const QString &etag)
{
    // the server must provide a valid etag but there might be bugs
    // https://github.com/owncloud/ocis/issues/7160
    if (OC_ENSURE_NOT(etag.isEmpty())) {
        auto &info = _lastEtagJob[f];
        if (f->canSync() && info.etag != etag) {
            qCDebug(lcEtagWatcher) << "Scheduling sync of" << f->displayName() << f->path() << "due to an etag change";
            info.etag = etag;
            _folderMan->scheduler()->enqueueFolder(f);
        }
        info.lastUpdate.reset();
    } else {
        qCWarning(lcEtagWatcher) << "Invalid empty etag received for" << f->displayName() << f->path();
    }
}
