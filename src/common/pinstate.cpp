/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pinstate.h"
#include "moc_pinstate.cpp"

#include <QCoreApplication>

using namespace OCC;

template <>
QString Utility::enumToDisplayName(VfsItemAvailability availability)
{
    switch (availability) {
    case VfsItemAvailability::AlwaysLocal:
        return QCoreApplication::translate("pinstate", "Always available locally");
    case VfsItemAvailability::AllHydrated:
        return QCoreApplication::translate("pinstate", "Currently available locally");
    case VfsItemAvailability::Mixed:
        return QCoreApplication::translate("pinstate", "Some available online only");
    case VfsItemAvailability::AllDehydrated:
        [[fallthrough]];
    case VfsItemAvailability::OnlineOnly:
        return QCoreApplication::translate("pinstate", "Available online only");
    }
    Q_UNREACHABLE();
}
