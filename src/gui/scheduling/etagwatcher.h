/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "libsync/graphapi/space.h"

#include <QObject>

#include <unordered_map>

namespace OCC {

class FolderMan;
class Folder;

class ETagWatcher : public QObject
{
    Q_OBJECT
public:
    ETagWatcher(QObject *parent);

public Q_SLOTS:
    void slotSpaceChanged(GraphApi::Space *space);

    void slotFolderListChanged(const QUuid &accountId, const QList<Folder *> folders);

    void onFolderAdded(const QUuid &accountId, Folder *folder);

    void onFolderRemoved(const QUuid &accountId, Folder *folder);


Q_SIGNALS:
    void requestEnqueueFolder(Folder *folder);

private:
    void updateEtag(const QUuid &accountId, const QString &spaceId, const QString &etag);

    struct ETagInfo
    {
        QString etag;
        Folder *folder;
    };

    // we have to separate the data into accounts because it's NOT the case that all spaceid's are unique
    // specifically, the Shares space always has the same space id -> see Space.cpp sharesIdC
    // so we have a hash keyed on the uuid of the account, which in turn holds the hash for the space ids->etag info
    QHash<QUuid, QHash<QString, ETagInfo>> _lastEtagJobForSpace;
};

}
