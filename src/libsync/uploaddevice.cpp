/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "account.h"
#include "filesystem.h"
#include "propagateuploadfile.h"
#include "syncengine.h"
#include "uploaddevice.h"

#include "common/asserts.h"
#include "common/utility.h"

#include "libsync/theme.h"

#include <QFileInfo>

#include <cmath>

using namespace std::chrono_literals;

namespace OCC {

UploadDevice::UploadDevice(const QString &fileName, qint64 start, qint64 size)
    : _file(fileName)
    , _start(start)
    , _size(size)
    , _read(0)
{
}

bool UploadDevice::open(QIODevice::OpenMode mode)
{
    if (mode & QIODevice::WriteOnly)
        return false;

    // Get the file size now: _file.fileName() is no longer reliable
    // on all platforms after openAndSeekFileSharedRead().
    auto fileDiskSize = FileSystem::getSize(QFileInfo{_file.fileName()});

    QString openError;
    if (!FileSystem::openAndSeekFileSharedRead(&_file, &openError, _start)) {
        setErrorString(openError);
        return false;
    }

    _size = qBound(0ll, _size, fileDiskSize - _start);
    _read = 0;

    return QIODevice::open(mode);
}

void UploadDevice::close()
{
    _file.close();
    QIODevice::close();
}

qint64 UploadDevice::writeData(const char *, qint64)
{
    OC_ASSERT_X(false, "write to read only device");
    return 0;
}

qint64 UploadDevice::readData(char *data, qint64 maxLen)
{
    if (_size - _read <= 0) {
        return -1;
    }
    maxLen = qMin(maxLen, _size - _read);
    if (maxLen <= 0) {
        return 0;
    }

    auto c = _file.read(data, maxLen);
    if (c < 0) {
        setErrorString(_file.errorString());
        return -1;
    }
    _read += c;
    return c;
}

bool UploadDevice::atEnd() const
{
    return _read >= _size;
}

qint64 UploadDevice::size() const
{
    return _size;
}

qint64 UploadDevice::bytesAvailable() const
{
    return _size - _read + QIODevice::bytesAvailable();
}

// random access, we can seek
bool UploadDevice::isSequential() const
{
    return false;
}

bool UploadDevice::seek(qint64 pos)
{
    if (!QIODevice::seek(pos)) {
        return false;
    }
    if (pos < 0 || pos > _size) {
        return false;
    }
    _read = pos;
    _file.seek(_start + pos);
    return true;
}
}
