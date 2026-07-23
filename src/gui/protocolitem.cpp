/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "protocolitem.h"

#include "folderman.h"
#include "progressdispatcher.h"

#include <QMenu>


using namespace OCC;


ProtocolItem::ProtocolItem(Folder *folder, const SyncFileItemPtr &item)
    : _path(item->destination())
    , _folder(folder)
    , _size(item->_size)
    , _status(item->_status)
    , _direction(item->_direction)
    , _message(item->_errorString)
    , _sizeIsRelevant(ProgressInfo::isSizeDependent(*item))
{
    if (!item->_responseTimeStamp.isEmpty()) {
        _timestamp = Utility::parseRFC1123Date(QString::fromUtf8(item->_responseTimeStamp));
    } else {
        _timestamp = QDateTime::currentDateTimeUtc();
    }
    if (_message.isEmpty()) {
        _message = Progress::asResultString(*item);
    }
}

QString ProtocolItem::path() const
{
    return _path;
}

Folder *ProtocolItem::folder() const
{
    return _folder;
}

QDateTime ProtocolItem::timestamp() const
{
    return _timestamp;
}

qint64 ProtocolItem::size() const
{
    return _size;
}

SyncFileItem::Status ProtocolItem::status() const
{
    return _status;
}

SyncFileItem::Direction ProtocolItem::direction() const
{
    return _direction;
}

QString ProtocolItem::message() const
{
    return _message;
}

bool ProtocolItem::isSizeRelevant() const
{
    return _sizeIsRelevant;
}
