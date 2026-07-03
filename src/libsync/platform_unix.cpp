/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "platform_unix.h"

#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>

#include <signal.h>
#include <sys/resource.h>

namespace OCC {

Q_LOGGING_CATEGORY(lcPlatform, "gui.platform")

UnixPlatform::UnixPlatform()
{
    signal(SIGPIPE, SIG_IGN);
    setLimitsForCoreDumps();
}

UnixPlatform::~UnixPlatform()
{
}

void UnixPlatform::setLimitsForCoreDumps()
{
    // check a environment variable for core dumps
    if (!qEnvironmentVariableIsEmpty("OWNCLOUD_CORE_DUMP")) {
        struct rlimit core_limit;
        core_limit.rlim_cur = RLIM_INFINITY;
        core_limit.rlim_max = RLIM_INFINITY;

        if (setrlimit(RLIMIT_CORE, &core_limit) < 0) {
            fprintf(stderr, "Unable to set core dump limit\n");
        } else {
            qCInfo(lcPlatform) << "Core dumps enabled";
        }
    }
}


} // namespace OCC
