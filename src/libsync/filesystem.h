/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>
#include <ctime>
#include <functional>

#include <owncloudlib.h>
// Chain in the base include and extend the namespace
#include "common/filesystembase.h"
#include "common/result.h"

class QFile;

namespace OCC {

class SyncJournal;

/**
 *  \addtogroup libsync
 *  @{
 */

/**
 * @brief This file contains file system helper
 */
namespace FileSystem {

    /**
     * @brief compare two files with given filename and return true if they have the same content
     */
    bool fileEquals(const QString &fn1, const QString &fn2);

    /**
     * @brief Get the mtime for a filepath
     *
     * Use this over QFileInfo::lastModified() to avoid timezone related bugs. See
     * owncloud/core#9781 for details.
     */
    time_t OWNCLOUDSYNC_EXPORT getModTime(const QString &filename);

    bool OWNCLOUDSYNC_EXPORT setModTime(const QString &filename, time_t modTime);

    /**
     * @brief Get the size for a file
     *
     * Use this over QFileInfo::size() to avoid bugs with lnk files on Windows.
     * See https://bugreports.qt.io/browse/QTBUG-24831.
     */
    qint64 OWNCLOUDSYNC_EXPORT getSize(const QFileInfo &info);

    /**
     * @brief Retrieve a file inode with csync
     */
    bool OWNCLOUDSYNC_EXPORT getInode(const QString &filename, quint64 *inode);

    /**
     * @brief Check if \a fileName has changed given previous size and mtime
     *
     * Nonexisting files are covered through mtime: they have an previousMtime of -1.
     *
     * @return true if the file's mtime or size are not what is expected.
     */
    bool OWNCLOUDSYNC_EXPORT fileChanged(const QFileInfo &info, qint64 previousSize, time_t previousMtime, std::optional<quint64> previousInode = {});


    struct RemoveEntry
    {
        const QString path;
        const bool isDir;
    };
    struct RemoveError
    {
        const RemoveEntry entry;
        const QString error;
    };

    using RemoveEntryList = std::vector<RemoveEntry>;
    using RemoveErrorList = std::vector<RemoveError>;

    /**
     * Removes a directory and its contents recursively
     *
     * Returns true if all removes succeeded.
     */
    bool OWNCLOUDSYNC_EXPORT removeRecursively(const QString &path,
        RemoveEntryList *success,
        RemoveEntryList *locked,
        RemoveErrorList *errors);

    namespace Tags {
        std::optional<QByteArray> OWNCLOUDSYNC_EXPORT get(const QString &path, const QString &key);
        OCC::Result<void, QString> OWNCLOUDSYNC_EXPORT set(const QString &path, const QString &key, const QByteArray &value);
        bool OWNCLOUDSYNC_EXPORT remove(const QString &path, const QString &key);
    }
}

/** @} */
}
