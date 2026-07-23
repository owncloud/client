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

namespace OCC::NewAccount {
Q_NAMESPACE

// I am not making this a strongly typed enum class as it's used to support the radio button ids in the advanced settings gui.
// an old fashioned enum does not require as many casts back and forth, which for this use case is a good thing.
enum SyncType { NONE, USE_VFS, SYNC_ALL, SELECTIVE_SYNC };

Q_ENUM_NS(SyncType)

}
