/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "resources/owncloudresources.h"

#include <QtWidgets/QWidget>

namespace OCC {
namespace Resources {

    class OWNCLOUDRESOURCES_EXPORT ThemeWatcher : public QObject
    {
        Q_OBJECT
    public:
        ThemeWatcher(QObject *parent = nullptr);

    Q_SIGNALS:
        void themeChanged();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
    };

} // Resources
} // OCC
