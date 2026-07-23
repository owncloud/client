/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "translations.h"

#include <QApplication>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QStandardPaths>

namespace OCC {

namespace Translations {

    Q_LOGGING_CATEGORY(lcTranslations, "gui.translations", QtInfoMsg)

    const QString translationsFilePrefix()
    {
        return QStringLiteral("client_");
    }

    const QString translationsFileSuffix()
    {
        return QStringLiteral(".qm");
    }

    QString translationsDirectoryPath()
    {
        return QStringLiteral(":/client/translations/");
    }

    QSet<QString> listAvailableTranslations()
    {
        QSet<QString> availableTranslations;

        // calculate a glob pattern which can be used in the iterator below to match only translations files
        const QString pattern = translationsFilePrefix() + QLatin1Char('*') + translationsFileSuffix();

        QDirIterator it(Translations::translationsDirectoryPath(), QStringList() << pattern);

        while (it.hasNext()) {
            // extract locale part from filename
            // of course, this method relies on the filenames to be accurate
            const auto fileName = QFileInfo(it.next()).fileName();
            const auto localePartLength = fileName.length() - translationsFileSuffix().length() - translationsFilePrefix().length();
            QString localeName = fileName.mid(translationsFilePrefix().length(), localePartLength);

            availableTranslations.insert(localeName);
        }

        return availableTranslations;
    }

} // namespace Translations

} // namespace OCC
