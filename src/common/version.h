/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ocsynclib.h"
#include "version-defines.h"

#include <QString>
#include <QVersionNumber>


namespace OCC::Version {
OCSYNC_EXPORT const QVersionNumber &version();

OCSYNC_EXPORT const QVersionNumber &versionWithBuildNumber();

inline int buildNumber()
{
    return versionWithBuildNumber().segmentAt(3);
}

/**
 * git, rc1, rc2
 * Empty in releases
 */
OCSYNC_EXPORT QString suffix();

/**
 * The commit id
 */
OCSYNC_EXPORT QString gitSha();

OCSYNC_EXPORT QString displayString();
}
