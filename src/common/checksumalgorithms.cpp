/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "checksumalgorithms.h"

using namespace OCC;
CheckSums::Algorithm CheckSums::fromByteArray(const QByteArray &s)
{
    // assert to ensure that all keys are uppercase
    Q_ASSERT([] {
        // ensure to run the check only once
        static bool once = [] {
            for (const auto &a : All) {
                const QString s = Utility::enumToString(a.first);
                if (s != s.toUpper()) {
                    return false;
                }
            }
            return true;
        }();
        return once;
    }());
    return fromName(s.toUpper().constData());
}

template <>
QString Utility::enumToString(CheckSums::Algorithm algo)
{
    const auto n = toString(algo);
    return QString::fromUtf8(n.data(), static_cast<int>(n.size()));
}

#include "moc_checksumalgorithms.cpp"
