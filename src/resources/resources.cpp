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

#include "resources/resources.h"
#include "resources/iconresources.h"
#include "resources/qmlresources.h"
#include "resources/template.h"
#include "resources/themewatcher.h"

#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QPainter>
#include <QPalette>
#include <QStyleHints>

using namespace OCC;
using namespace Resources;

Q_LOGGING_CATEGORY(lcResources, "sync.resources", QtInfoMsg)

namespace {
struct IconCache
{
    IconCache()
    {
        // auto *watcher = new ThemeWatcher(qApp);
        // QObject::connect(watcher, &ThemeWatcher::themeChanged, [this]() { _cache.clear(); });

        // as of qt 6.5 this is the "correct" way to listen for system color changes
        // this generally works to clear the old icons, but places where the icon is already set, eg
        // the toolbar actions and connection status, need to be reset to what is in the new cache.
        // this will not work here, as we need to be sure the cache has been cleared *before* anyone
        // asks for the icon again.
        // I think at some point we should have a colorManager that orchestrates these updates relative
        // to system color changes. The responsibility does not belong in resources!
        QObject::connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, [this]() { _cache.clear(); });
    }
    QMap<QString, QIcon> _cache;
};
Q_GLOBAL_STATIC(IconCache, iconCache)

QString vanillaThemePath()
{
    return QStringLiteral(":/client/ownCloud/theme");
}

QString brandThemePath()
{
    return QStringLiteral(":/client/" APPLICATION_SHORTNAME "/theme");
}

QString darkTheme()
{
    return QStringLiteral("dark");
}

QString coloredTheme()
{
    return QStringLiteral("colored");
}


bool hasTheme(IconType type, const QString &theme)
{
    // <<is vanilla, theme name>, bool
    // caches the availability of themes for branded and unbranded themes
    static QMap<QPair<bool, QString>, bool> _themeCache;
    const auto key = qMakePair(type != IconType::VanillaIcon, theme);
    auto it = _themeCache.constFind(key);
    if (it == _themeCache.cend()) {
        return _themeCache[key] = QFileInfo(QStringLiteral("%1/%2/").arg(type == IconType::VanillaIcon ? vanillaThemePath() : brandThemePath(), theme)).isDir();
    }
    return it.value();
}

}

bool OCC::Resources::hasDarkTheme()
{
    static bool _hasBrandedColored = hasTheme(IconType::BrandedIcon, coloredTheme());
    static bool _hasBrandedDark = hasTheme(IconType::BrandedIcon, darkTheme());
    return _hasBrandedColored == _hasBrandedDark;
}

// todo: all of this will die soon :)
CoreImageProvider::CoreImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}
QPixmap CoreImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const auto qmlIcon = QMLResources::parseIcon(id);

    QIcon icon;
    if (qmlIcon.theme == QLatin1String("core")) {
        icon = IconResources::getCoreIcon(qmlIcon.iconName);
    } else if (qmlIcon.theme == QLatin1String("universal")) {
        icon = IconResources::getUniversalIcon(qmlIcon.iconName);
    } else {
        QString themeName = IconResources::isUsingDarkTheme() && hasDarkTheme() ? darkTheme() : coloredTheme();
        icon = IconResources::getThemedIcon(themeName, qmlIcon.iconName);
    }
    return Resources::pixmap(requestedSize, icon, qmlIcon.enabled ? QIcon::Normal : QIcon::Disabled, size);
}
