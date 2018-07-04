/*
 * libcsync -- a library to sync a directory with another
 *
 * Copyright (c) 2008-2013 by Andreas Schneider <asn@cryptomilk.org>
 * Copyright (c) 2012-2013 by Klaas Freitag <freitag@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _CSYNC_UPDATE_H
#define _CSYNC_UPDATE_H

#include "csync.h"

/**
 * @brief The file tree walker.
 *
 * This function walks through the directory tree that is located under the uri
 * specified. It calls a walker function which is provided as a function pointer
 * once for each entry in the tree. By default, directories are handled before
 * the files and subdirectories they contain (pre-order traversal).
 *
 * @param  ctx          The csync context to use.
 *
 * @param  uri          The uri/path to the directory tree to walk.
 *
 * @param directory_fs    The file entry corresponding to that directory. nullptr for the root item
 *
 * @param  depth        The max depth to walk down the tree.
 *
 * @return 0 on success, < 0 on error. If fn() returns non-zero, then the tree
 *         walk is terminated and the value returned by fn() is returned as the
 *         result.
 */
int csync_ftw(CSYNC *ctx, const char *uri, csync_file_stat_t *directory_fs, unsigned int depth);

#endif /* _CSYNC_UPDATE_H */

/* vim: set ft=c.doxygen ts=8 sw=2 et cindent: */
