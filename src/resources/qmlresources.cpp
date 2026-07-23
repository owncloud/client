/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/qmlresources.h"

#include "resources/resources.h"

namespace {
constexpr QSize minIconSize{16, 16};
}

using namespace OCC;
QUrl Resources::QMLResources::resourcePath2(const QString &provider, const QString &icon, bool enabled, const QVariantMap &properies)
{
    auto map =
        QVariantMap{{QStringLiteral("enabled"), enabled}, {QStringLiteral("icon"), icon}, {QStringLiteral("systemtheme"), Resources::isUsingDarkTheme()}};
    map.insert(properies);
    const auto data = QJsonDocument::fromVariant(map).toJson();
    return QUrl(QStringLiteral("image://%1/%2").arg(provider, QString::fromUtf8(data.toBase64())));
}

QUrl Resources::QMLResources::resourcePath(const QString &theme, const QString &icon, bool enabled)
{
    return resourcePath2(QStringLiteral("ownCloud"), icon, enabled, {{QStringLiteral("theme"), theme}});
}

Resources::QMLResources::Icon Resources::QMLResources::parseIcon(const QString &id)
{
    const auto data = QJsonDocument::fromJson(QByteArray::fromBase64(id.toUtf8())).object();
    return Icon{data.value(QLatin1String("theme")).toString(), data.value(QLatin1String("icon")).toString(), data.value(QLatin1String("enabled")).toBool()};
}

QPixmap Resources::pixmap(const QSize &requestedSize, const QIcon &icon, QIcon::Mode mode, QSize *outSize)
{
    Q_ASSERT(requestedSize.isValid());
    QSize actualSize = requestedSize.isValid() ? requestedSize : icon.availableSizes().constFirst();
    if (outSize) {
        *outSize = actualSize;
    }
    return icon.pixmap(actualSize.expandedTo(minIconSize), mode);
}
