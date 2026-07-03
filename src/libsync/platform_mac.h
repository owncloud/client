/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "platform.h"

#include <QProcess>
#include <qglobal.h>

namespace OCC {

class MacPlatformPrivate;

class MacPlatform : public Platform
{
public:
    MacPlatform();
    ~MacPlatform() override;

    void migrate() override;

    void startServices() override;

private:
    Q_DECLARE_PRIVATE(MacPlatform)
    QScopedPointer<MacPlatformPrivate> d_ptr;
};

} // namespace OCC
