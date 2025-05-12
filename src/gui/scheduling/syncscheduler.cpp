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

#include "gui/scheduling/syncscheduler.h"

#include "gui/folderman.h"
#include "gui/networkinformation.h"
#include "gui/scheduling/etagwatcher.h"
#include "libsync/configfile.h"
#include "libsync/syncengine.h"

using namespace std::chrono_literals;

using namespace OCC;

Q_LOGGING_CATEGORY(lcSyncScheduler, "gui.scheduler.syncscheduler", QtInfoMsg)

class FolderPriorityQueue
{
private:
    struct Element
    {
        Element() { }

        Element(Folder *f, SyncScheduler::Priority p)
            : folder(f)
            , rawFolder(f)
            , priority(p)
        {
        }

        // We don't own the folder, so it might get deleted
        QPointer<Folder> folder = nullptr;
        // raw pointer for lookup in _scheduledFolders
        Folder *rawFolder = nullptr;
        SyncScheduler::Priority priority = SyncScheduler::Priority::Low;

        friend bool operator<(const Element &lhs, const Element &rhs) { return lhs.priority < rhs.priority; }
    };


public:
    FolderPriorityQueue() = default;

    void enqueueFolder(Folder *folder, SyncScheduler::Priority priority)
    {
        const auto [it, inserted] = _scheduledFolders.emplace(folder, priority);
        if (inserted) {
            // the folder is not yet scheduled
            _queue.emplace(folder, priority);
        } else {
            // if the new priority is higher we need to rebuild the queue
            if (priority > it->second) {
                // we need to reorder the queue
                // this is expensive
                decltype(_queue) out;
                for (; !_queue.empty(); _queue.pop()) {
                    const auto &tmp = _queue.top();
                    if (tmp.folder != folder) {
                        out.push(std::move(tmp));
                    } else {
                        out.emplace(folder, priority);
                    }
                }
                _queue = std::move(out);
                // replace the the old priority
                _scheduledFolders[folder] = priority;
            }
        }
    }

    auto empty() { return _queue.empty(); }
    auto size() { return _queue.size(); }

    std::pair<Folder *, SyncScheduler::Priority> pop()
    {
        Element out;
        while (!_queue.empty() && !out.folder) {
            // could be a nullptr by now
            out = _queue.top();
            [[maybe_unused]] auto removed = _scheduledFolders.erase(_queue.top().rawFolder);
            Q_ASSERT(removed = 1);
            _queue.pop();
        }
        return std::make_pair(out.folder, out.priority);
    }

private:
    // the actual queue
    std::priority_queue<Element> _queue;
    // helper container to ensure we don't enqueue a Folder multiple times
    std::unordered_map<Folder *, SyncScheduler::Priority> _scheduledFolders;
};

// Refactoring todo: I do not see where/how the syncs and etag watcher get updated when eg an account is removed.
// the main dep here is on the FolderMan which means we rely on the folder list being updated and/or folders deleted
// to disconnect this stuff, but afaik the accountStatePtr is ACTUALLY the first thing to go when an account is removed.
// this scheduler/watcher combo needs deep investigation as it might be the source of "mysterious" crashes.
SyncScheduler::SyncScheduler(FolderMan *parent)
    : QObject(parent)
    , _pauseSyncWhenMetered(ConfigFile().pauseSyncWhenMetered())
    , _queue(new FolderPriorityQueue)
{
    new ETagWatcher(parent, this);

    // Normal syncs are performed incremental but when fullLocalDiscoveryInterval times out
    // a complete local discovery is performed.
    // This timer here triggers a sync independent of etag changes on the server.
    auto *fullLocalDiscoveryTimer = new QTimer(this);
    fullLocalDiscoveryTimer->setInterval(ConfigFile().fullLocalDiscoveryInterval() + 2min);
    connect(fullLocalDiscoveryTimer, &QTimer::timeout, this, [parent, this] {
        for (auto *f : parent->folders()) {
            if (f->isReady() && f->accountState()->state() == AccountState::State::Connected) {
                enqueueFolder(f);
            }
        }
    });
    fullLocalDiscoveryTimer->start();
}


SyncScheduler::~SyncScheduler()
{
    delete _queue;
}

void SyncScheduler::enqueueFolder(Folder *folder, Priority priority)
{
    if (!folder->canSync()) {
        qCWarning(lcSyncScheduler) << "Cannot enqueue folder" << folder->path() << ": folder is marked as cannot sync";
        return;
    }

    qCInfo(lcSyncScheduler) << "Enqueue" << folder->path() << priority << "QueueSize:" << _queue->size();
    _queue->enqueueFolder(folder, priority);
    if (!_currentSync) {
        startNext();
    }
}

void SyncScheduler::startNext()
{
    if (!_running) {
        qCInfo(lcSyncScheduler) << "Scheduler is paused, next sync is not started";
        return;
    }

    if (!_currentSync.isNull()) {
        qCInfo(lcSyncScheduler) << "Another sync is already running, waiting for that to finish before starting a new sync";
        return;
    }

    auto nextSync = _queue->pop();
    while (nextSync.first && !nextSync.first->canSync()) {
        nextSync = _queue->pop();
    }
    _currentSync = nextSync.first;
    auto syncPriority = nextSync.second;

    if (!_currentSync.isNull()) {
        if (_pauseSyncWhenMetered && NetworkInformation::instance()->isMetered()) {
            if (syncPriority == Priority::High) {
                qCInfo(lcSyncScheduler) << "Scheduler is paused due to metered internet connection, BUT next sync is HIGH priority, so allow sync to start";
            } else {
                enqueueFolder(_currentSync, syncPriority);
                qCInfo(lcSyncScheduler) << "Scheduler is paused due to metered internet connection, next sync is not started";
                return;
            }
        }

        connect(
            _currentSync, &Folder::syncFinished, this,
            [this](const SyncResult &result) {
                qCInfo(lcSyncScheduler) << "Sync finished for" << _currentSync->path() << "with status" << result.status();
                _currentSync = nullptr;
                startNext();
            },
            Qt::SingleShotConnection);
        connect(_currentSync, &Folder::destroyed, this, &SyncScheduler::startNext, Qt::SingleShotConnection);
        qCInfo(lcSyncScheduler) << "Starting sync for" << _currentSync->path();
        _currentSync->startSync();
    }
}

void SyncScheduler::start()
{
    _running = true;
    startNext();
}

void SyncScheduler::stop()
{
    _running = false;
}

bool SyncScheduler::hasCurrentRunningSyncRunning() const
{
    return _currentSync;
}

void SyncScheduler::setPauseSyncWhenMetered(bool pauseSyncWhenMetered)
{
    _pauseSyncWhenMetered = pauseSyncWhenMetered;
    if (!pauseSyncWhenMetered) {
        startNext();
    }
}
