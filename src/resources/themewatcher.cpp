/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "themewatcher.h"

#include <QCoreApplication>
#include <QEvent>

namespace OCC {
namespace Resources {
    ThemeWatcher::ThemeWatcher(QObject *parent)
        : QObject(parent)
    {
        qApp->installEventFilter(this);
    }

    bool ThemeWatcher::eventFilter(QObject *watched, QEvent *event)
    {
        if (event->type() == QEvent::ThemeChange) {
            Q_EMIT themeChanged();
        }
        return QObject::eventFilter(watched, event);
    }
} // Resources
} // OCC
