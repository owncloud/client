/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "template.h"

#include "common/asserts.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>

using namespace OCC::Resources;

Q_LOGGING_CATEGORY(lcResourcesTeplate, "sync.resoruces.template", QtInfoMsg)

QString Template::renderTemplateFromFile(const QString &filePath, const QMap<QString, QString> &values)
{
    return renderTemplate(
        [&] {
            QFile f(filePath);
            OC_ASSERT(f.open(QFile::ReadOnly));
            return QString::fromUtf8(f.readAll());
        }(),
        values);
}

QString Template::renderTemplate(QString &&templ, const QMap<QString, QString> &values)
{
    static const QRegularExpression pattern(QStringLiteral("@{([^{}]+)}"));
    const auto replace = [&templ, &values](QRegularExpressionMatchIterator it) {
        while (it.hasNext()) {
            const auto match = it.next();
            Q_ASSERT(match.lastCapturedIndex() == 1);
            Q_ASSERT([&] {
                if (!values.contains(match.captured(1))) {
                    qWarning(lcResourcesTeplate) << "Unknown key:" << match.captured(1);
                    return false;
                }
                return true;
            }());
            templ.replace(match.captured(0), values.value(match.captured(1)));
        }
    };

    auto matches = pattern.globalMatch(templ);
    do {
        replace(matches);
        // the placeholder can again contain a placeholder
        matches = pattern.globalMatch(templ);
    } while (matches.hasNext());

    return templ;
}
