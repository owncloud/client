/*
 * c_time - time functions
 *
 * SPDX-FileCopyrightText: 2008-2013 Andreas Schneider <asn@cryptomilk.org>
 * SPDX-FileCopyrightText: 2014-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _C_TIME_H
#define _C_TIME_H

#include <QString>

#include "ocsynclib.h"

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

OCSYNC_EXPORT int c_utimes(const QString &uri, const struct timeval *times);


#endif /* _C_TIME_H */
