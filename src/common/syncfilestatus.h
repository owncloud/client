/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SYNCFILESTATUS_H
#define SYNCFILESTATUS_H

#include <QMetaType>
#include <QObject>
#include <QString>

#include "ocsynclib.h"

namespace OCC {

/**
 * @brief The SyncFileStatus class
 * @ingroup libsync
 */
class OCSYNC_EXPORT SyncFileStatus
{
    Q_GADGET
public:
    enum SyncFileStatusTag {
        StatusNone,
        StatusSync,
        StatusWarning,
        StatusUpToDate,
        StatusError,
        StatusExcluded,
    };
    Q_ENUM(SyncFileStatusTag);

    SyncFileStatus();
    SyncFileStatus(SyncFileStatusTag);

    void set(SyncFileStatusTag tag);
    SyncFileStatusTag tag() const;

    void setShared(bool isShared);
    bool shared() const;

    QString toSocketAPIString() const;

private:
    SyncFileStatusTag _tag;
    bool _shared;
};

inline bool operator==(const SyncFileStatus &a, const SyncFileStatus &b)
{
    return a.tag() == b.tag() && a.shared() == b.shared();
}

inline bool operator!=(const SyncFileStatus &a, const SyncFileStatus &b)
{
    return !(a == b);
}
}
OCSYNC_EXPORT QDebug &operator<<(QDebug &debug, const OCC::SyncFileStatus &item);

Q_DECLARE_METATYPE(OCC::SyncFileStatus)

#endif // SYNCFILESTATUS_H
