/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIODevice>
#include <QLoggingCategory>
#include "utility.h"

namespace OCC {

// Device that reads via FETCH_DATA to feed data into checksum validation functions
class PlaceholderValidationDevice : public QIODevice
{
    Q_OBJECT

public:
    PlaceholderValidationDevice(
        CfAPIUtil::CfOpdata opdata,
        quint64 startOffset,
        quint64 size)
        : _opdata(opdata)
        , _offset(startOffset)
        , _size(size)
    {
    }

    qint64 size() const override;
    qint64 pos() const override;
    bool atEnd() const override;

protected:
    qint64 readData(char *data, qint64 maxSize) override;

    bool isSequential() const override
    {
        return true;
    }

    qint64 writeData(const char *, qint64) override
    {
        // can't write data
        return -1;
    }

private:
    CfAPIUtil::CfOpdata _opdata;

    quint64 _offset = 0;
    quint64 _size = 0;
};

}
