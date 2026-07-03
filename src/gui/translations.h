/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/owncloudguilib.h"

#include <QSet>
#include <QString>

namespace OCC {

namespace Translations {

    /**
    * @return translation files' filename prefix
    */
    OWNCLOUDGUI_EXPORT const QString translationsFilePrefix();

    /**
    * @returntranslation files' filename suffix
    */
    OWNCLOUDGUI_EXPORT const QString translationsFileSuffix();

    /**
     * @return path to translation files
     */
    OWNCLOUDGUI_EXPORT QString translationsDirectoryPath();

    /**
     * @return list of locales for which translations are available
     */
    OWNCLOUDGUI_EXPORT QSet<QString> listAvailableTranslations();

} // namespace Translations

} // namespace OCC
