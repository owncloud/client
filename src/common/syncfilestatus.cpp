/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "syncfilestatus.h"

#include <QDebug>

namespace OCC {
SyncFileStatus::SyncFileStatus()
    : _tag(StatusNone)
    , _shared(false)
{
}

SyncFileStatus::SyncFileStatus(SyncFileStatusTag tag)
    : _tag(tag)
    , _shared(false)
{
}

void SyncFileStatus::set(SyncFileStatusTag tag)
{
    _tag = tag;
}

SyncFileStatus::SyncFileStatusTag SyncFileStatus::tag() const
{
    return _tag;
}

void SyncFileStatus::setShared(bool isShared)
{
    _shared = isShared;
}

bool SyncFileStatus::shared() const
{
    return _shared;
}

QString SyncFileStatus::toSocketAPIString() const
{
    QString statusString;
    bool canBeShared = true;

    switch (_tag) {
    case StatusNone:
        statusString = QStringLiteral("NOP");
        canBeShared = false;
        break;
    case StatusSync:
        statusString = QStringLiteral("SYNC");
        break;
    case StatusWarning:
        // The protocol says IGNORE, but all implementations show a yellow warning sign.
        statusString = QStringLiteral("IGNORE");
        break;
    case StatusUpToDate:
        statusString = QStringLiteral("OK");
        break;
    case StatusError:
        statusString = QStringLiteral("ERROR");
        break;
    case StatusExcluded:
        // The protocol says IGNORE, but all implementations show a yellow warning sign.
        statusString = QStringLiteral("IGNORE");
        break;
    }
    if (canBeShared && _shared) {
        statusString += QLatin1String("+SWM");
    }

    return statusString;
}
}


QDebug &operator<<(QDebug &debug, const OCC::SyncFileStatus &item)
{
    QDebugStateSaver saver(debug);
    debug.setAutoInsertSpaces(false);
    debug << "OCC::SyncFileStatus(shared=" << item.shared() << ", tag=" << item.tag() << ")";
    return debug;
}
