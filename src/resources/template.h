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

#include <QUrl>

namespace OCC {
namespace Resources {
    namespace Template {

        /**
         * Replace all occurrences of @{} values in template with the values from values
         */
        OWNCLOUDRESOURCES_EXPORT QString renderTemplateFromFile(const QString &filePath, const QMap<QString, QString> &values);
        OWNCLOUDRESOURCES_EXPORT QString renderTemplate(QString &&templ, const QMap<QString, QString> &values);
    };
}
}
