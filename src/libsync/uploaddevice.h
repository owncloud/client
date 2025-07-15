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
#pragma once

#include <QFile>

namespace OCC {

/**
 * @brief The UploadDevice class
 * @ingroup libsync
 */
class UploadDevice : public QIODevice
{
    Q_OBJECT
public:
    UploadDevice(const QString &fileName, qint64 start, qint64 size);

    bool open(QIODevice::OpenMode mode) override;
    void close() override;

    qint64 writeData(const char *, qint64) override;
    qint64 readData(char *data, qint64 maxLen) override;
    bool atEnd() const override;
    qint64 size() const override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override;
    bool seek(qint64 pos) override;

    Q_SIGNALS:

    private:
    /// The local file to read data from
    QFile _file;

    /// Start of the file data to use
    qint64 _start = 0;
    /// Amount of file data after _start to use
    qint64 _size = 0;
    /// Position between _start and _start+_size
    qint64 _read = 0;
};
}