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
#include <QUrl>
#include <QtQuick/QQuickImageProvider>

namespace OCC::Resources {
Q_NAMESPACE
/**
 * Wehther we allow a fallback to a vanilla icon
 */
enum class IconType { BrandedIcon, BrandedIconWithFallbackToVanillaIcon, VanillaIcon };
Q_ENUM_NS(IconType);


/**
 *
 * @return Whether we are using the vanilla theme
 */
bool OWNCLOUDRESOURCES_EXPORT isVanillaTheme();

/**
 * Whether use the dark icon theme
 * The function also ensures the theme supports the dark theme
 */
bool OWNCLOUDRESOURCES_EXPORT isUsingDarkTheme();

bool OWNCLOUDRESOURCES_EXPORT hasDarkTheme();

/** Whether the theme provides monochrome tray icons
 */
bool OWNCLOUDRESOURCES_EXPORT hasMonoTheme();

QIcon OWNCLOUDRESOURCES_EXPORT getCoreIcon(const QString &icon_name);

QIcon OWNCLOUDRESOURCES_EXPORT loadIcon(const QString &flavor, const QString &name, IconType iconType);
QIcon OWNCLOUDRESOURCES_EXPORT themeIcon(const QString &name, IconType iconType = IconType::BrandedIconWithFallbackToVanillaIcon);

/**
 * Returns a universal (non color schema aware) icon.
 */
QIcon OWNCLOUDRESOURCES_EXPORT themeUniversalIcon(const QString &name, IconType iconType = IconType::BrandedIcon);

class OWNCLOUDRESOURCES_EXPORT CoreImageProvider : public QQuickImageProvider
{
    Q_OBJECT
public:
    CoreImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
}
