/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "drives.h"

#include "account.h"

#include <OAICollection_of_drives.h>
#include <OAIDrive.h>


using namespace OCC;
using namespace GraphApi;

namespace {

const auto mountpointC = QLatin1String("mountpoint");
}

Drives::Drives(Account *account, QObject *parent)
    : JsonJob(account, account->url(), QStringLiteral("/graph/v1.0/me/drives"), "GET", {}, {}, parent)
{
}

Drives::~Drives() { }

const QList<OpenAPI::OAIDrive> &Drives::drives() const
{
    if (_drives.isEmpty() && parseError().error == QJsonParseError::NoError) {
        OpenAPI::OAICollection_of_drives drives;
        drives.fromJsonObject(data());
        _drives = drives.getValue();
        // At the moment we don't support mountpoints but use the Share Jail
        _drives.erase(
            std::remove_if(_drives.begin(), _drives.end(), [](const OpenAPI::OAIDrive &it) { return it.getDriveType() == mountpointC; }), _drives.end());
    }
    return _drives;
}
