/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
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

#pragma once

#include "constexpr_list.h"
#include "ocsynclib.h"
#include "utility.h"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>

#include <cstdint>
#include <ctime>


class QFile;

namespace OCC {

OCSYNC_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcFileSystem)

/**
 *  \addtogroup libsync
 *  @{
 */

/**
 * @brief This file contains file system helper
 */
namespace FileSystem {
    OCSYNC_EXPORT Q_NAMESPACE;

    /**
     * List of characters not allowd in filenames on Windows
     */
    constexpr_list auto IllegalFilenameCharsWindows = {
        QLatin1Char('\\'), QLatin1Char(':'), QLatin1Char('?'), QLatin1Char('*'), QLatin1Char('"'), QLatin1Char('>'), QLatin1Char('<'), QLatin1Char('|')
    };

    /**
     * @brief Mark the file as hidden  (only has effects on windows)
     */
    void OCSYNC_EXPORT setFileHidden(const QString &filename, bool hidden);

    /**
     * @brief Marks the file as read-only.
     *
     * On linux this either revokes all 'w' permissions or restores permissions
     * according to the umask.
     */
    void OCSYNC_EXPORT setFileReadOnly(const QString &filename, bool readonly);

    /**
     * @brief Marks the file as read-only.
     *
     * It's like setFileReadOnly(), but weaker: if readonly is false and the user
     * already has write permissions, no change to the permissions is made.
     *
     * This means that it will preserve explicitly set rw-r--r-- permissions even
     * when the umask is 0002. (setFileReadOnly() would adjust to rw-rw-r--)
     */
    void OCSYNC_EXPORT setFileReadOnlyWeak(const QString &filename, bool readonly);

    /**
     * @brief Try to set permissions so that other users on the local machine can not
     * go into the folder.
     */
    void OCSYNC_EXPORT setFolderMinimumPermissions(const QString &filename);

    /*
     * This function takes a path and converts it to a UNC representation of the
     * string. That means that it prepends a \\?\ (unless already UNC) and converts
     * all slashes to backslashes.
     *
     * Note the following:
     *  - The string must be absolute.
     *  - it needs to contain a drive character to be a valid UNC
     *  - A conversion is only done if the path len is larger than 245. Otherwise
     *    the windows API functions work with the normal "unixoid" representation too.
     */
    QString OCSYNC_EXPORT longWinPath(const QString &inpath);

    /**
     * @brief Checks whether a file exists.
     *
     * Use this over QFileInfo::exists() and QFile::exists() to avoid bugs with lnk
     * files, see above.
     */
    bool OCSYNC_EXPORT fileExists(const QString &filename, const QFileInfo & = QFileInfo());

    /**
     * @brief Rename the file \a originFileName to \a destinationFileName.
     *
     * It behaves as QFile::rename() but handles .lnk files correctly on Windows.
     */
    bool OCSYNC_EXPORT rename(const QString &originFileName,
        const QString &destinationFileName,
        QString *errorString = nullptr);

    /**
     * Rename the file \a originFileName to \a destinationFileName, and
     * overwrite the destination if it already exists - without extra checks.
     */
    bool OCSYNC_EXPORT uncheckedRenameReplace(const QString &originFileName,
        const QString &destinationFileName,
        QString *errorString);

    /**
     * Removes a file.
     *
     * Equivalent to QFile::remove(), except on Windows, where it will also
     * successfully remove read-only files.
     */
    bool OCSYNC_EXPORT remove(const QString &fileName, QString *errorString = nullptr);

    /**
     * Replacement for QFile::open(ReadOnly) followed by a seek().
     * This version sets a more permissive sharing mode on Windows.
     *
     * Warning: The resulting file may have an empty fileName and be unsuitable for use
     * with QFileInfo! Calling seek() on the QFile with >32bit signed values will fail!
     */
    bool OCSYNC_EXPORT openAndSeekFileSharedRead(QFile * file, QString * error, qint64 seek);

    enum class LockMode {
        Shared,
        Exclusive,
        SharedRead
    };
    Q_ENUM_NS(LockMode);
    /**
     * Returns true when a file is locked. (Windows only)
     */
    bool OCSYNC_EXPORT isFileLocked(const QString &fileName, LockMode mode);

#ifdef Q_OS_WIN

    bool OCSYNC_EXPORT longPathsEnabledOnWindows();

#endif
    /**
     * Returns the file system used at the given path.
     */
    QString OCSYNC_EXPORT fileSystemForPath(const QString &path);

    /**
     * Returns whether the file is a shortcut file (ends with .lnk)
     */
    bool OCSYNC_EXPORT isLnkFile(const QString &filename);

    /**
     * Returns whether the file is a junction (windows only)
     */
    bool OCSYNC_EXPORT isJunction(const QString &filename);

    /**
     * Returns whether a Path is a child of another
     */
    bool OCSYNC_EXPORT isChildPathOf(QStringView child, QStringView parent);

    /**
     * Ensures the file name length is allowed on all platforms and the file name does not contain illegal characters
     * reservedSize: The resulting path will be reservedSize < MAX and allows appending.
     */
    QString OCSYNC_EXPORT createPortableFileName(const QString &path, const QString &fileName, qsizetype reservedSize = 0);

    /*
     * Replace path navigation elements from the string
     */
    QString OCSYNC_EXPORT pathEscape(const QString &s);

    namespace SizeLiterals {
        constexpr auto BinaryBase = 1024;
        constexpr auto DecimalBase = 1000;

        constexpr unsigned long long operator"" _B(unsigned long long sz)
        {
            return sz;
        }

        constexpr unsigned long long operator"" _KiB(unsigned long long sz)
        {
            return operator"" _B(sz) * BinaryBase;
        }

        constexpr unsigned long long operator"" _MiB(unsigned long long sz)
        {
            return operator"" _KiB(sz) * BinaryBase;
        }

        constexpr unsigned long long operator"" _GiB(unsigned long long sz)
        {
            return operator"" _MiB(sz) * BinaryBase;
        }

        constexpr unsigned long long operator"" _kB(unsigned long long sz)
        {
            return operator"" _B(sz) * DecimalBase;
        }

        constexpr unsigned long long operator"" _MB(unsigned long long sz)
        {
            return operator"" _kB(sz) * DecimalBase;
        }

        constexpr unsigned long long operator"" _GB(unsigned long long sz)
        {
            return operator"" _MB(sz) * DecimalBase;
        }
    }
}

/** @} */
}
