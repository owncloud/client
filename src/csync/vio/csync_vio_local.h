/*
 * libcsync -- a library to sync a directory with another
 *
 * SPDX-FileCopyrightText: 2008-2013 Andreas Schneider <asn@cryptomilk.org>
 * SPDX-FileCopyrightText: 2014-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _CSYNC_VIO_LOCAL_H
#define _CSYNC_VIO_LOCAL_H

#include "ocsynclib.h"

#include <QString>

struct csync_vio_handle_t;
namespace OCC {
class Vfs;
}

csync_vio_handle_t OCSYNC_EXPORT *csync_vio_local_opendir(const QString &name);
int OCSYNC_EXPORT csync_vio_local_closedir(csync_vio_handle_t *dhandle);
std::unique_ptr<csync_file_stat_t> OCSYNC_EXPORT csync_vio_local_readdir(csync_vio_handle_t *dhandle, OCC::Vfs *vfs);

int OCSYNC_EXPORT csync_vio_local_stat(const QString &uri, csync_file_stat_t *buf);

#endif /* _CSYNC_VIO_LOCAL_H */
