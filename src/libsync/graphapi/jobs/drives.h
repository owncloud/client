/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "networkjobs/jsonjob.h"

#include "owncloudlib.h"
#include <OAIDrive.h>

namespace OCC {
namespace GraphApi {


    class OWNCLOUDSYNC_EXPORT Drives : public JsonJob
    {
        Q_OBJECT
    public:
        Drives(Account *account, QObject *parent = nullptr);
        ~Drives();
        const QList<OpenAPI::OAIDrive> &drives() const;

    private:
        mutable QList<OpenAPI::OAIDrive> _drives;
    };
}
}
