/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ocsynclib.h"

#include <QStringList>

#include <functional>

namespace OCC {
class OCSYNC_EXPORT RestartManager
{
public:
    RestartManager(std::function<int(int, char **)> &&main);

    ~RestartManager();

    int exec(int argc, char **argv) const;

    static void requestRestart();

private:
    static RestartManager *_instance;

    std::function<int(int, char **)> _main;

    QString _applicationToRestart;
    QStringList _args;
};

}
