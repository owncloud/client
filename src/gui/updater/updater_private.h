/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace OCC {
static const QString updateAvailableC = QStringLiteral("Updater/updateAvailable");
static const QString updateTargetVersionC = QStringLiteral("Updater/updateTargetVersion");
static const QString updateTargetVersionStringC = QStringLiteral("Updater/updateTargetVersionString");
// the config file key's name is preserved for legacy reasons
static const QString previouslySkippedVersionC = QStringLiteral("Updater/seenVersion");
static const QString autoUpdateAttemptedC = QStringLiteral("Updater/autoUpdateAttempted");
}
