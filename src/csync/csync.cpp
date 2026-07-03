/*
 * libcsync -- a library to sync a directory with another
 *
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "csync.h"

#include <QMetaEnum>

QDebug operator<<(QDebug debug, const SyncInstructions &enumValue)
{
    static const QMetaEnum me = QMetaEnum::fromType<SyncInstruction>();
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << me.enumName() << "(" << me.valueToKeys(enumValue) << ")";
    return debug;
}
