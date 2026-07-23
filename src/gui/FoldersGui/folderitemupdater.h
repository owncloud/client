/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "accountstate.h"
#include "progressdispatcher.h"
#include <chrono>

namespace OCC {

class FolderItem;

class FolderItemUpdater : public QObject
{
    Q_OBJECT
public:
    explicit FolderItemUpdater(FolderItem *item);


private:
    FolderItem *_item;

    void onSpaceChanged();
    void onConnectedChanged(AccountState::State newState);
    void onSyncStateChanged();
    void onProgressUpdated(const ProgressInfo &progress);
    void onImageChanged();

    QMetaObject::Connection _progressInfoConnection;
    QMetaObject::Connection _imageChangeConnection;

    inline static const auto ProgressUpdateTimeout = std::chrono::seconds(1);
    std::chrono::steady_clock::time_point _lastProgressUpdated;
};
}
