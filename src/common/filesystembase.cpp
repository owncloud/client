/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
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

#include "filesystembase.h"
#include "utility.h"
#include "common/asserts.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QSettings>
#include <QStorageInfo>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef Q_OS_WIN
#include "utility_win.h"
#include <fcntl.h>
#include <io.h>
#endif

namespace {
// Regarding
// https://en.wikipedia.org/wiki/Comparison_of_file_systems#Limits
// the file name limit appears to be about 255 chars
// -10 for safety
constexpr qsizetype fileNameMaxC = 255 - 10;

constexpr auto replacementCharC = QLatin1Char('_');
}

namespace OCC {

Q_LOGGING_CATEGORY(lcFileSystem, "sync.filesystem", QtInfoMsg)

QString FileSystem::longWinPath(const QString &inpath)
{
#ifndef Q_OS_WIN
    return inpath;
#else
    Q_ASSERT(QFileInfo(inpath).isAbsolute());
    if (inpath.isEmpty()) {
        return inpath;
    }
    const QString str = QDir::toNativeSeparators(inpath);
    const QLatin1Char sep('\\');

    // we already have a unc path
    if (str.startsWith(sep + sep)) {
        return str;
    }
    // prepend \\?\ and to support long names

    if (str.at(0) == sep) {
        // should not happen as we require the path to be absolute
        return QStringLiteral("\\\\?") + str;
    }
    return QStringLiteral("\\\\?\\") + str;
#endif
}

void FileSystem::setFileHidden(const QString &filename, bool hidden)
{
#ifdef _WIN32
    QString fName = longWinPath(filename);
    DWORD dwAttrs;

    dwAttrs = GetFileAttributesW((wchar_t *)fName.utf16());

    if (dwAttrs != INVALID_FILE_ATTRIBUTES) {
        if (hidden && !(dwAttrs & FILE_ATTRIBUTE_HIDDEN)) {
            SetFileAttributesW((wchar_t *)fName.utf16(), dwAttrs | FILE_ATTRIBUTE_HIDDEN);
        } else if (!hidden && (dwAttrs & FILE_ATTRIBUTE_HIDDEN)) {
            SetFileAttributesW((wchar_t *)fName.utf16(), dwAttrs & ~FILE_ATTRIBUTE_HIDDEN);
        }
    }
#else
    Q_UNUSED(filename);
    Q_UNUSED(hidden);
#endif
}

static QFile::Permissions getDefaultWritePermissions()
{
    QFile::Permissions result = QFile::WriteUser;
#ifndef Q_OS_WIN
    mode_t mask = umask(0);
    umask(mask);
    if (!(mask & S_IWGRP)) {
        result |= QFile::WriteGroup;
    }
    if (!(mask & S_IWOTH)) {
        result |= QFile::WriteOther;
    }
#endif
    return result;
}

void FileSystem::setFileReadOnly(const QString &filename, bool readonly)
{
    QFile file(filename);
    QFile::Permissions permissions = file.permissions();

    QFile::Permissions allWritePermissions =
        QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther | QFile::WriteOwner;
    static QFile::Permissions defaultWritePermissions = getDefaultWritePermissions();

    permissions &= ~allWritePermissions;
    if (!readonly) {
        permissions |= defaultWritePermissions;
    }
    // Warning: This function does not manipulate ACLs, which may limit its effectiveness.
    file.setPermissions(permissions);
}

void FileSystem::setFolderMinimumPermissions(const QString &filename)
{
#ifdef Q_OS_MAC
    QFile::Permissions perm = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
    QFile file(filename);
    file.setPermissions(perm);
#else
    Q_UNUSED(filename);
#endif
}


void FileSystem::setFileReadOnlyWeak(const QString &filename, bool readonly)
{
    QFile file(filename);
    QFile::Permissions permissions = file.permissions();

    if (!readonly && (permissions & QFile::WriteOwner)) {
        return; // already writable enough
    }

    setFileReadOnly(filename, readonly);
}

bool FileSystem::rename(const QString &originFileName,
    const QString &destinationFileName,
    QString *errorString)
{
    bool success = false;
    QString error;
#ifdef Q_OS_WIN
    const QString originalFileNameLong = longWinPath(originFileName);
    const QString dest = longWinPath(destinationFileName);
    if (FileSystem::isFileLocked(dest, FileSystem::LockMode::Exclusive)) {
        error = QCoreApplication::translate("FileSystem", "Can't rename %1, the file is currently in use").arg(destinationFileName);
    } else if (FileSystem::isFileLocked(originalFileNameLong, FileSystem::LockMode::Exclusive)) {
        error = QCoreApplication::translate("FileSystem", "Can't rename %1, the file is currently in use").arg(originFileName);
    } else if (isLnkFile(originFileName) || isLnkFile(destinationFileName)) {
        success = MoveFileEx((wchar_t *)originalFileNameLong.utf16(), (wchar_t *)dest.utf16(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
        if (!success) {
            error = Utility::formatWinError(GetLastError());
        }
    } else
#endif
    {
        QFile orig(originFileName);
        success = orig.rename(destinationFileName);
        if (!success) {
            error = orig.errorString();
        }
    }

    if (!success) {
        qCWarning(lcFileSystem) << "Error renaming file" << originFileName
                                << "to" << destinationFileName
                                << "failed: " << error;
        if (errorString) {
            *errorString = error;
        }
    }
    return success;
}

bool FileSystem::uncheckedRenameReplace(const QString &originFileName,
    const QString &destinationFileName,
    QString *errorString)
{
    Q_ASSERT(errorString);
#ifndef Q_OS_WIN
    bool success;
    QFile orig(originFileName);
    // We want a rename that also overwrites.  QFile::rename does not overwrite.
    // Qt 5.1 has QSaveFile::renameOverwrite we could use.
    // ### FIXME
    success = true;
    bool destExists = fileExists(destinationFileName);
    if (destExists && !QFile::remove(destinationFileName)) {
        *errorString = orig.errorString();
        qCWarning(lcFileSystem) << "Target file could not be removed.";
        success = false;
    }
    if (success) {
        success = orig.rename(destinationFileName);
    }
    if (!success) {
        *errorString = orig.errorString();
        qCWarning(lcFileSystem) << "Renaming temp file to final failed: " << *errorString;
        return false;
    }

#else //Q_OS_WIN
    // You can not overwrite a read-only file on windows.

    if (!QFileInfo(destinationFileName).isWritable()) {
        setFileReadOnly(destinationFileName, false);
    }
    const QString orig = longWinPath(originFileName);
    const QString dest = longWinPath(destinationFileName);
    if (FileSystem::isFileLocked(dest, FileSystem::LockMode::Exclusive)) {
        *errorString = QCoreApplication::translate("FileSystem", "Can't rename %1, the file is currently in use").arg(destinationFileName);
        qCWarning(lcFileSystem) << "Renaming failed: " << *errorString;
        return false;
    }
    if (FileSystem::isFileLocked(orig, FileSystem::LockMode::Exclusive)) {
        *errorString = QCoreApplication::translate("FileSystem", "Can't rename %1, the file is currently in use").arg(originFileName);
        qCWarning(lcFileSystem) << "Renaming failed: " << *errorString;
        return false;
    }
    const BOOL ok = MoveFileEx(reinterpret_cast<const wchar_t *>(orig.utf16()),
        reinterpret_cast<const wchar_t *>(dest.utf16()),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
    if (!ok) {
        *errorString = Utility::formatWinError(GetLastError());
        qCWarning(lcFileSystem) << "Renaming temp file to final failed: " << *errorString;
        return false;
    }
#endif
    return true;
}

bool FileSystem::openAndSeekFileSharedRead(QFile *file, QString *errorOrNull, qint64 seek)
{
    QString errorDummy;
    // avoid many if (errorOrNull) later.
    QString &error = errorOrNull ? *errorOrNull : errorDummy;
    error.clear();

#ifdef Q_OS_WIN
    //
    // The following code is adapted from Qt's QFSFileEnginePrivate::nativeOpen()
    // by including the FILE_SHARE_DELETE share mode.
    //

    // Enable full sharing.
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    int accessRights = GENERIC_READ;
    DWORD creationDisp = OPEN_EXISTING;

    // Create the file handle.
    SECURITY_ATTRIBUTES securityAtts = {sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
    QString fName = longWinPath(file->fileName());

    HANDLE fileHandle = CreateFileW((const wchar_t *)fName.utf16(), accessRights, shareMode, &securityAtts, creationDisp, FILE_ATTRIBUTE_NORMAL, nullptr);

    // Bail out on error.
    if (fileHandle == INVALID_HANDLE_VALUE) {
        error = qt_error_string();
        return false;
    }

    // Convert the HANDLE to an fd and pass it to QFile's foreign-open
    // function. The fd owns the handle, so when QFile later closes
    // the fd the handle will be closed too.
    int fd = _open_osfhandle((intptr_t)fileHandle, _O_RDONLY);
    if (fd == -1) {
        error = QStringLiteral("could not make fd from handle");
        CloseHandle(fileHandle);
        return false;
    }
    if (!file->open(fd, QIODevice::ReadOnly, QFile::AutoCloseHandle)) {
        error = file->errorString();
        _close(fd); // implicitly closes fileHandle
        return false;
    }

    // Seek to the right spot
    LARGE_INTEGER *li = reinterpret_cast<LARGE_INTEGER *>(&seek);
    DWORD newFilePointer = SetFilePointer(fileHandle, li->LowPart, &li->HighPart, FILE_BEGIN);
    if (newFilePointer == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        error = qt_error_string();
        return false;
    }

    return true;
#else
    if (!file->open(QFile::ReadOnly)) {
        error = file->errorString();
        return false;
    }
    if (!file->seek(seek)) {
        error = file->errorString();
        return false;
    }
    return true;
#endif
}

#ifdef Q_OS_WIN
static bool fileExistsWin(const QString &filename)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    QString fName = FileSystem::longWinPath(filename);

    hFind = FindFirstFileW((wchar_t *)fName.utf16(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }
    FindClose(hFind);
    return true;
}
#endif

bool FileSystem::fileExists(const QString &filename, const QFileInfo &fileInfo)
{
#ifdef Q_OS_WIN
    if (isLnkFile(filename)) {
        // Use a native check.
        return fileExistsWin(filename);
    }
#endif
    bool re = fileInfo.exists();
    // if the filename is different from the filename in fileInfo, the fileInfo is
    // not valid. There needs to be one initialised here. Otherwise, the incoming
    // fileInfo is re-used.
    if (fileInfo.filePath() != filename) {
        re = QFileInfo::exists(filename);
    }
    return re;
}

QString FileSystem::fileSystemForPath(const QString &path)
{
    QString p = path;
    while (true) {
        if (!fileExists(p)) {
            QFileInfo file(p);
            p = file.absolutePath();
            continue;
        }
        const QStorageInfo storage(p);
        if (!storage.isValid() || !storage.isReady()) {
            return {};
        }

        return QString::fromUtf8(storage.fileSystemType());
    }
}

#ifdef Q_OS_WIN
bool FileSystem::longPathsEnabledOnWindows()
{
    static std::optional<bool> longPathsEnabledCached = {};

    if (!longPathsEnabledCached.has_value()) {
        // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry#enable-long-paths-in-windows-10-version-1607-and-later
        QSettings fsSettings(QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QSettings::NativeFormat);
        QVariant longPathsEnabled = fsSettings.value(QStringLiteral("LongPathsEnabled"));
        qCDebug(lcFileSystem) << "LongPathsEnabled:" << longPathsEnabled;
        longPathsEnabledCached = longPathsEnabled.value<uint32_t>() == 1;
    }

    return longPathsEnabledCached.value();
}
#endif

bool FileSystem::remove(const QString &fileName, QString *errorString)
{
#ifdef Q_OS_WIN
    // You cannot delete a read-only file on windows, but we want to
    // allow that.

    if (!QFileInfo(fileName).isWritable()) {
        setFileReadOnly(fileName, false);
    }
#endif
    QFile f(fileName);
    if (!f.remove()) {
        if (errorString) {
            *errorString = f.errorString();
        }
        qCWarning(lcFileSystem) << "Failed to delete:" << fileName << "Error:" << f.errorString();
        return false;
    }
    return true;
}

#ifdef Q_OS_WIN

namespace {
    /**
     * This function creates a file handle with the desired LockMode
     */
    Utility::Handle lockFile(const QString &fileName, FileSystem::LockMode mode)
    {
        const QString fName = FileSystem::longWinPath(fileName);
        int shareMode = 0;
        int accessMode = GENERIC_READ | GENERIC_WRITE;
        switch (mode) {
        case FileSystem::LockMode::Exclusive:
            shareMode = 0;
            break;
        case FileSystem::LockMode::Shared:
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;
        case FileSystem::LockMode::SharedRead:
            shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            accessMode = GENERIC_READ;
            break;
        }
        // Check if file exists
        DWORD attr = GetFileAttributesW(reinterpret_cast<const wchar_t *>(fName.utf16()));
        if (attr != INVALID_FILE_ATTRIBUTES) {
            // Try to open the file with as much access as possible..
            auto out = Utility::Handle{CreateFileW(reinterpret_cast<const wchar_t *>(fName.utf16()), accessMode, shareMode, nullptr, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr)};

            if (out) {
                LARGE_INTEGER start;
                start.QuadPart = 0;
                LARGE_INTEGER end;
                end.QuadPart = -1;
                if (LockFile(out.handle(), start.LowPart, start.HighPart, end.LowPart, end.HighPart)) {
                    OC_ENFORCE(UnlockFile(out.handle(), start.LowPart, start.HighPart, end.LowPart, end.HighPart));
                    return out;
                } else {
                    return {};
                }
            }
            return out;
        }
        return {};
    }
}
#endif


bool FileSystem::isFileLocked(const QString &fileName, LockMode mode)
{
#ifdef Q_OS_WIN
    const auto handle = lockFile(fileName, mode);
    if (!handle) {
        const auto error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION) {
            return true;
        } else if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            qCWarning(lcFileSystem()) << Q_FUNC_INFO << Utility::formatWinError(error);
        }
    }
#else
    Q_UNUSED(fileName);
    Q_UNUSED(mode);
#endif
    return false;
}

bool FileSystem::isLnkFile(const QString &filename)
{
    return filename.endsWith(QLatin1String(".lnk"));
}

bool FileSystem::isJunction(const QString &filename)
{
#ifdef Q_OS_WIN
    WIN32_FIND_DATA findData;
    HANDLE hFind =
        FindFirstFileEx(reinterpret_cast<const wchar_t *>(longWinPath(filename).utf16()), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        return false;
    }
    return findData.dwFileAttributes != INVALID_FILE_ATTRIBUTES
        && findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT
        && findData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT;
#else
    Q_UNUSED(filename);
    return false;
#endif
}

bool FileSystem::isChildPathOf(QStringView child, QStringView parent)
{
    // if it is a relative path assume a local file, resolve it based on root
    const auto sensitivity = Utility::fsCaseSensitivity();

    // Fast-path the 3 common cases in this if-else statement:
    if (parent.isEmpty()) {
        // The empty parent is often used as the sync root, as (child) items do not start with a `/`
        return true;
    }

    const auto isSeparator = [](QChar c) { return c == QLatin1Char('/') || (Utility::isWindows() && c == QLatin1Char('\\')); };

    if (isSeparator(parent.back())) {
        // Here we can do a normal prefix check, because the parent is "terminated" with a slash,
        // and we can't walk into the case in the else below.
        if (child.startsWith(parent, sensitivity)) {
            return true;
        }
        // else: do the `cleanPath` version below
    } else {
        // Here we fast-path for the case the child is "Documents/a" and the parent is "Documents",
        // and *not* the case where the child is "Documents/a" and the parent is "Document". In
        // the latter case, the does start with the name of the parent, but there is an extra 's',
        // and not a slash.

        // Prevent a string concatenation like in `child.startsWith(parent + QLatin1Char('/'))`:
        // first check if the child string starts with the parent string
        if (child.startsWith(parent, sensitivity)) {
            // ok, now check if the character after the parent is a '/'
            if (child.length() >= parent.length() + 1 && isSeparator(child.at(parent.length()))) {
                return true;
            }
            // else: do the `cleanPath` version below
        }
    }

    // Slow path (`QDir::cleanPath` does lots of string operations):
    const QString cleanParent = QDir::cleanPath(parent.toString());
    const QString cleanChild = QDir::cleanPath(child.toString());
    // cleanPath removes trailing slashes, add one to parent to be sure we handle a child path
    // /root/foo/bar is not  a child of /root/fo, therefor the trailing slash is important
    if (cleanChild.startsWith(cleanParent + QLatin1Char('/'), sensitivity)) {
        return true;
    }
    // both paths are the same /root/foo == /root/foo
    if (cleanChild.compare(cleanParent, sensitivity) == 0) {
        return true;
    }
    return false;
}

QString FileSystem::createPortableFileName(const QString &path, const QString &fileName, qsizetype reservedSize)
{
    // remove leading and trailing whitespace
    QString tmp = pathEscape(fileName);
    // limit size to fileNameMaxC
    tmp.resize(std::min<qsizetype>(tmp.size(), fileNameMaxC - reservedSize));
    // remove eventual trailing whitespace after the resize
    tmp = tmp.trimmed();

    return QDir::cleanPath(path + QLatin1Char('/') + tmp);
}

QString FileSystem::pathEscape(const QString &s)
{
    QString tmp = s;
    tmp.replace(QLatin1String("../"), QString(replacementCharC));
#ifdef Q_OS_WIN
    tmp.replace(QLatin1String("..\\"), QString(replacementCharC));
#endif

    // escape the folder separator, "\" is handled in IllegalFilenameCharsWindows
    tmp.replace(QLatin1Char('/'), replacementCharC);

    // replace non portable characters
    for (auto c : IllegalFilenameCharsWindows) {
        tmp.replace(c, replacementCharC);
    }
    return tmp.trimmed();
}

} // namespace OCC

#include "moc_filesystembase.cpp"
