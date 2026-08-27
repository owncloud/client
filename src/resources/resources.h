/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#pragma once
#include "resources/owncloudresources.h"

#include <QIcon>
#include <QUrl>
#include <QtQuick/QQuickImageProvider>

namespace OCC::Resources {
Q_NAMESPACE
/**
 * Wehther we allow a fallback to a vanilla icon
 */
enum class IconType { BrandedIcon, BrandedIconWithFallbackToVanillaIcon, VanillaIcon };
Q_ENUM_NS(IconType);

bool OWNCLOUDRESOURCES_EXPORT hasDarkTheme();

// QIcon OWNCLOUDRESOURCES_EXPORT loadIcon(const QString &flavor, const QString &name, IconType iconType);

// this needs to stay until we get rid of qml in spacesBrowser
class OWNCLOUDRESOURCES_EXPORT CoreImageProvider : public QQuickImageProvider
{
    Q_OBJECT
public:
    CoreImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
}
