/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIODevice>
#include "utility.h"

namespace OCC {

// Device that feeds data to Windows via CfExecute instead of storing to a file
class PlaceholderHydrationDevice : public QIODevice
{
    Q_OBJECT

public:
    PlaceholderHydrationDevice(
        CfAPIUtil::CfOpdata opdata,
        quint64 startOffset,
        quint64 size);

    quint64 transferedAmount() const
    {
        return _offset;
    }

    quint64 fileSize() const
    {
        return _size;
    }

protected:
    qint64 readData(char *, qint64) override
    {
        // can't read data
        return -1;
    }

    qint64 writeData(const char *data, qint64 maxSize) override;

    void close() override;

private:
    CfAPIUtil::CfOpdata _opdata;

    qsizetype _offset = 0;
    const qsizetype _size = 0;

    QByteArray _buffer;

    // for the progress bar
    LARGE_INTEGER _progressCompleted = {};
};

}
