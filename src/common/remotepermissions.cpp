/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "remotepermissions.h"
#include <cstring>

namespace OCC {

static const char letters[] = " WDNVCKRSMmz";


template <typename Char>
void RemotePermissions::fromArray(const Char *p)
{
    _value = notNullMask;
    if (!p)
        return;
    while (*p) {
        if (auto res = std::strchr(letters, static_cast<char>(*p)))
            _value |= (1 << (res - letters));
        ++p;
    }
}

QByteArray RemotePermissions::toDbValue() const
{
    QByteArray result;
    if (isNull())
        return result;
    result.reserve(PermissionsCount);
    for (uint i = 1; i <= PermissionsCount; ++i) {
        if (_value & (1 << i))
            result.append(letters[i]);
    }
    if (result.isEmpty()) {
        // Make sure it is not empty so we can differentiate null and empty permissions
        result.append(' ');
    }
    return result;
}

QString RemotePermissions::toString() const
{
    return QString::fromUtf8(toDbValue());
}

RemotePermissions RemotePermissions::fromDbValue(const QByteArray &value)
{
    if (value.isEmpty())
        return RemotePermissions();
    RemotePermissions perm;
    perm.fromArray(value.constData());
    return perm;
}

RemotePermissions RemotePermissions::fromServerString(const QString &value)
{
    RemotePermissions perm;
    perm.fromArray(value.utf16());
    return perm;
}

} // namespace OCC
