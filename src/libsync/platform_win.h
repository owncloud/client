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

namespace OCC {

class WinPlatform : public Platform
{
public:
    WinPlatform();

    ~WinPlatform() override;

    void setApplication(QCoreApplication *application) override;

    void startServices() override;

private:
    /// Utility thread that takes care of proper Windows logout handling.
    void startShutdownWatcher();
};

} // namespace OCC
