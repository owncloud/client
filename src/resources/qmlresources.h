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

#include <QIcon>
#include <QtQml/QtQml>

namespace OCC {
namespace Resources {

    class OWNCLOUDRESOURCES_EXPORT QMLResources : public QObject
    {
        Q_OBJECT
        QML_SINGLETON
        QML_ELEMENT
    public:
        using QObject::QObject;
        struct Icon
        {
            QString theme;
            QString iconName;
            bool enabled;
        };
        Q_INVOKABLE static QUrl resourcePath(const QString &theme, const QString &icon, bool enabled);
        Q_INVOKABLE static QUrl resourcePath2(const QString &provider, const QString &icon, bool enabled, const QVariantMap &properies = {});

        static Icon parseIcon(const QString &id);
    };

    QPixmap OWNCLOUDRESOURCES_EXPORT pixmap(const QSize &requestedSize, const QIcon &icon, QIcon::Mode mode, QSize *outSize);
} // Resources
} // OCC
