/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "etagwatcher.h"
#include "gui/folder.h"

#include <QObject>

#include <queue>
#include <unordered_map>


class FolderPriorityQueue;

namespace OCC {

class FolderMan;

class SyncScheduler : public QObject
{
    Q_OBJECT
public:
    enum class Priority : uint8_t {
        // Normal sync triggered by etag change or something similar
        Low,

        // Related to a user action
        Medium,

        // Usually triggered by a user (ForceSync)
        High
    };
    Q_ENUM(Priority);

    explicit SyncScheduler(FolderMan *parent);
    ~SyncScheduler() override;

    void enqueueFolder(Folder *folder, Priority priority = Priority::Low);

    void start();

    void stop();

    void setPauseSyncWhenMetered(bool pauseSyncWhenMetered);

    void connectSpacesManager(OCC::GraphApi::SpacesManager *spaceMan);

public Q_SLOTS:

    void handleEnqueueFolder(Folder *f);

private:
    void startNext();

    bool _running = false;
    bool _pauseSyncWhenMetered;
    ETagWatcher *_watcher = nullptr;
    QPointer<Folder> _currentSync;
    FolderPriorityQueue *_queue;
};
}
