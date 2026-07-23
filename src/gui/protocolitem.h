/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/owncloudguilib.h"

#include "gui/folder.h"

#include "csync/csync.h"
#include "libsync/syncfileitem.h"

#include "common/fixedsizeringbuffer.h"

namespace OCC {

class OWNCLOUDGUI_EXPORT ProtocolItem
{
    Q_GADGET
public:
    ProtocolItem() = default;
    explicit ProtocolItem(Folder *folder, const SyncFileItemPtr &item);
    QString path() const;

    Folder *folder() const;

    /**
     * UTC Time
     */
    QDateTime timestamp() const;

    qint64 size() const;

    SyncFileItem::Status status() const;

    SyncFileItem::Direction direction() const;

    QString message() const;

    bool isSizeRelevant() const;

private:
    QString _path;
    Folder *_folder;
    QDateTime _timestamp;
    qint64 _size;
    SyncFileItem::Status _status;
    SyncFileItem::Direction _direction;

    QString _message;
    bool _sizeIsRelevant;

    friend class TestProtocolModel;
};

}
