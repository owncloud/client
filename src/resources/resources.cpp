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
#include "resources/qmlresources.h"
#include "resources/template.h"
#include "resources/themewatcher.h"

#include "common/asserts.h"

#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QPalette>

using namespace OCC;
using namespace Resources;

Q_LOGGING_CATEGORY(lcResources, "sync.resources", QtInfoMsg)

namespace {
struct IconCache
{
    IconCache()
    {
        auto *watcher = new ThemeWatcher(qApp);
        QObject::connect(watcher, &ThemeWatcher::themeChanged, [this]() { _cache.clear(); });
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

QString whiteTheme()
{
    return QStringLiteral("white");
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

bool Resources::hasMonoTheme()
{
    // mono icons are only supported in vanilla and if a customer provides them
    // no fallback to vanilla
    return hasTheme(Resources::IconType::BrandedIcon, whiteTheme());
}

bool Resources::isVanillaTheme()
{
    return std::string_view(APPLICATION_SHORTNAME) == "ownCloud";
}

bool OCC::Resources::isUsingDarkTheme()
{
    // TODO: replace by a command line switch
    static bool forceDark = qEnvironmentVariableIntValue("OWNCLOUD_FORCE_DARK_MODE") != 0;
    return forceDark || QPalette().base().color().lightnessF() <= 0.5;
}

QIcon OCC::Resources::getCoreIcon(const QString &iconName)
{
    if (iconName.isEmpty()) {
        return {};
    }
    QIcon &cached = iconCache->_cache[iconName]; // Take reference, this will also "set" the cache entry
    if (cached.isNull()) {
        const QString iconPath = QStringLiteral(":/client/resources/core/%1.svg").arg(iconName);
        Q_ASSERT(QFileInfo::exists(iconPath));
        const QString color = isUsingDarkTheme() ? QStringLiteral("#ADACAB") : QStringLiteral("#435671");
        QByteArray data = Template::renderTemplateFromFile(iconPath, {{QStringLiteral("color"), color}}).toUtf8();
        QBuffer buffer(&data);
        QImageReader iconReader(&buffer, "svg");
        return cached = QPixmap::fromImageReader(&iconReader);
    }
    return cached;
}

/*
 * helper to load a icon from either the icon theme the desktop provides or from
 * the apps Qt resources.
 */
QIcon OCC::Resources::loadIcon(const QString &flavor, const QString &name, IconType iconType)
{
    // prevent recusion
    const bool useCoreIcon = (iconType == IconType::VanillaIcon) || isVanillaTheme();
    const QString path = QStringLiteral("%1/%2/%3").arg(useCoreIcon ? vanillaThemePath() : brandThemePath(), flavor, name);
    QIcon &cached = iconCache->_cache[path]; // Take reference, this will also "set" the cache entry
    if (cached.isNull()) {
        if (isVanillaTheme() && QIcon::hasThemeIcon(name)) {
            // use from theme
            return cached = QIcon::fromTheme(name);
        }
        const QString svg = QStringLiteral("%1.svg").arg(path);
        if (QFile::exists(svg)) {
            return cached = QIcon(svg);
        }

        const QString png = QStringLiteral("%1.png").arg(path);
        if (QFile::exists(png)) {
            return cached = QIcon(png);
        }

        const QList<int> sizes{16, 22, 32, 48, 64, 128, 256, 512, 1024};
        QString previousIcon;
        for (int size : sizes) {
            QString pixmapName = QStringLiteral("%1-%2.png").arg(path, QString::number(size));
            if (QFile::exists(pixmapName)) {
                previousIcon = pixmapName;
                cached.addFile(pixmapName, {size, size});
            } else if (size >= 128) {
                if (!previousIcon.isEmpty()) {
                    qCWarning(lcResources) << "Upscaling:" << previousIcon << "to" << size;
                    cached.addPixmap(QPixmap(previousIcon).scaled({size, size}, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        }
    }
    if (cached.isNull()) {
        if (!useCoreIcon && iconType == IconType::BrandedIconWithFallbackToVanillaIcon) {
            return loadIcon(flavor, name, IconType::VanillaIcon);
        }
        qCWarning(lcResources) << "Failed to locate the icon" << path;
    }
    return cached;
}

QIcon OCC::Resources::themeIcon(const QString &name, IconType iconType)
{
    return loadIcon((Resources::isUsingDarkTheme() && hasDarkTheme()) ? darkTheme() : coloredTheme(), name, iconType);
}

QIcon OCC::Resources::themeUniversalIcon(const QString &name, IconType iconType)
{
    return loadIcon(QStringLiteral("universal"), name, iconType);
}

CoreImageProvider::CoreImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}
QPixmap CoreImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const auto qmlIcon = QMLResources::parseIcon(id);

    QIcon icon;
    if (qmlIcon.theme == QLatin1String("core")) {
        icon = getCoreIcon(qmlIcon.iconName);
    } else if (qmlIcon.theme == QLatin1String("universal")) {
        icon = themeUniversalIcon(qmlIcon.iconName);
    } else {
        icon = themeIcon(qmlIcon.iconName);
    }
    return Resources::pixmap(requestedSize, icon, qmlIcon.enabled ? QIcon::Normal : QIcon::Disabled, size);
}
