/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

#include "iconresources.h"

#include "template.h"

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QImageReader>
#include <QLoggingCategory>
#include <QPainter>
#include <QPalette>
#include <QStringBuilder>
#include <QStyleHints>

namespace OCC {

Q_LOGGING_CATEGORY(lcIconResources, "sync.iconresources", QtInfoMsg)

bool IconResources::isDefaultTheme()
{
    return std::string_view(APPLICATION_SHORTNAME) == "ownCloud";
}

QString IconResources::brandedRootPath()
{
    if (_brandedRootPath.isEmpty())
        _brandedRootPath = QStringLiteral(":/client/" APPLICATION_SHORTNAME "/theme");
    return _brandedRootPath;
}

bool IconResources::isUsingDarkTheme()
{
    return qGuiApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString IconResources::pathForTheme(const QString &iconTheme, bool branded)
{
    if (branded) {
        if (_brandedThemePaths.contains(iconTheme))
            return _brandedThemePaths[iconTheme];

        if (!_brandedThemePaths.contains(iconTheme)) {
            QString themeRoot = brandedRootPath();
            QString themePath = themeRoot % "/" % iconTheme % "/";
            //  QFileInfo brandedInfo(themeRoot);
            // if (brandedInfo.exists(iconTheme)) {
            if (QFileInfo::exists(themePath)) {
                // QString themePath = themeRoot % "/" % iconTheme % "/";
                _brandedThemePaths.insert(iconTheme, themePath);
                return themePath;
            }
        } /*else
            // we can't find the theme folder in the branding resources so fall back to default oc theme
            return pathForTheme(iconTheme, false);
        }*/
    }


    if (!_fallbackThemePaths.contains(iconTheme)) {
        QFileInfo baseInfo(_defaultRootPath);
        QString fullPath = _defaultRootPath % "/" % iconTheme % "/";
        if (QFileInfo::exists(fullPath)) {
            // we intentionally add the default full path to the _fallbackThemePaths *even if it is empty*
            // this indicates that if the caller gets an empty path back, it's really *nowhere* to be found.
            // should never happen but who knows.
            _fallbackThemePaths.insert(iconTheme, fullPath);
        } else {
            _fallbackThemePaths.insert(iconTheme, {});
        }
    }
    return _fallbackThemePaths[iconTheme];
}


QIcon IconResources::getCoreIcon(const QString &name)
{
    if (name.isEmpty()) {
        return {};
    }

    QIcon &cached = _coreIconCache[name]; // Take reference, this will also "set" the cache entry
    if (cached.isNull()) {
        const QString iconPath = QStringLiteral(":/client/resources/core/%1.svg").arg(name);
        if (!QFileInfo::exists(iconPath))
            return {};
        const QString color = isUsingDarkTheme() ? QStringLiteral("#ADACAB") : QStringLiteral("#435671");
        // todo add icon pixmap for checked state, too
        QByteArray data = Resources::Template::renderTemplateFromFile(iconPath, {{QStringLiteral("color"), color}}).toUtf8();
        QBuffer buffer(&data);
        QImageReader iconReader(&buffer, "svg");
        return cached = QPixmap::fromImageReader(&iconReader);
    }
    return cached;
}

QIcon IconResources::getUniversalIcon(const QString &name)
{
    return getThemedIcon(QStringLiteral("universal"), name);
}

QIcon IconResources::themedTrayIcon(const QString &name, [[maybe_unused]] bool sysTrayMenuVisible, bool trayIsDark)
{
    QString iconTheme;
    if (_useMonoTrayIcons) {
        iconTheme = trayIsDark ? _whiteMonoTheme : _blackMonoTheme;


#ifdef Q_OS_MAC
        if (sysTrayMenuVisible) {
            iconTheme = _whiteMonoTheme;
        }
#endif

    } else { // not using mono tray icons
        iconTheme = trayIsDark ? _darkTheme : _lightTheme;
    }

    QIcon icon = getThemedIcon(iconTheme, QStringLiteral("state-%1").arg(name));

#ifdef Q_OS_MAC
    // This defines the icon as a template and enables automatic macOS color handling
    // See https://bugreports.qt.io/browse/QTBUG-42109
    icon.setIsMask(_useMonoTrayIcons && !sysTrayMenuVisible);
#endif

    return icon;
}

QString IconResources::findIconPath(const QString &iconTheme, const QString &name, bool branded)
{
    // if a brand doesn't provide the iconTheme at all, we are hoping to find it in the fallback theme paths
    // if the fallback doesn't have the theme we are totally out of luck
    QString themePath = pathForTheme(iconTheme, branded);
    if (themePath.isEmpty())
        return {};

    // most icons are svg's so this should hit early
    QString iconPath = themePath % name % ".svg";
    QFileInfo infoSvg(iconPath);
    bool found = infoSvg.exists() && infoSvg.isFile();
    if (found)
        return iconPath;

    // try again for png:
    iconPath = themePath % name % ".png";
    QFileInfo infoPng(iconPath);
    found = infoPng.exists() && infoPng.isFile();
    if (found)
        return iconPath;

    // also check to see if this "path" will be built from multiple sized png's
    // filter on the icon name + a wildcard where the size will be
    QStringList filter{name + "-*.png"};
    QDir themeDir(themePath);
    QStringList searchRes = themeDir.entryList(filter);
    if (!searchRes.isEmpty())
        return themePath % name;

    // if we haven't found it in the expected location so try one more time in case the icon itself is missing, hopefully we can find it
    // in the fallback theme
    if (branded)
        return findIconPath(iconTheme, name, false);

    return {};
}

QIcon IconResources::getThemedIcon(const QString &iconTheme, const QString &name)
{
    // find the icon: prefer branded location but if not, take it from the fallback
    // note that findIconPath also "clears"
    QString iconPath = findIconPath(iconTheme, name);
    if (iconPath.isEmpty()) {
        qCWarning(lcIconResources) << "Failed to locate the icon" << iconPath;
        // this may be where we add the sized png's in the commented code below?
        // I have no idea what those are actually for in the first place and would expect the caller to specify the size in the name?

        // returning this placeholder so we can easily see where icons are dead related to the sized icons - should be temporary
        return getCoreIcon("delete");
    }

    QIcon &cached = _themedIconCache[iconPath];
    if (cached.isNull()) {
        cached = QIcon(iconPath);

        if (cached.isNull()) {
            // then this needs to be built from multiple png's - it is very likely the app icon but anything is possible, apparently
            const QList<int> sizes{16, 22, 32, 48, 64, 128, 256, 512, 1024};
            QString previousIcon;
            for (int size : sizes) {
                QString pixmapName = QStringLiteral("%1-%2.png").arg(iconPath, QString::number(size));
                if (QFile::exists(pixmapName)) {
                    previousIcon = pixmapName;
                    cached.addFile(pixmapName, {size, size});
                } else if (size >= 128) {
                    if (!previousIcon.isEmpty()) {
                        qCWarning(lcIconResources) << "Upscaling:" << previousIcon << "to" << size;
                        cached.addPixmap(QPixmap(previousIcon).scaled({size, size}, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                }
            }
        }
        Q_ASSERT(!cached.isNull());
    }

    return cached;
}

bool IconResources::useMonoTrayIcons()
{
    return _useMonoTrayIcons;
}

void IconResources::setUseMonoTrayIcons(bool useMono)
{
    _useMonoTrayIcons = useMono;
}

void IconResources::handleSystemStyleChanged()
{
    // for now this is all we should do
    _coreIconCache.clear();
}

QIcon IconResources::buildAvatar(const QString &initials, QUuid accountUid)
{
    QIcon &cached = _coreIconCache[accountUid.toString()]; // Take reference, this will also "set" the cache entry
    if (cached.isNull()) {
        // for now we are going for a color scheme that mimics our other icons.
        // the logic:
        // it is extremely difficult to "pick" random colors that are going to look good and have
        // sufficient contrast with dark and light mode on all platforms
        // additionally, a random colorful icon looks a bit out of place given we have no other "custom" colors
        // in the app outside of the account wizard - I think using the same gray we do for other icons is harmonious, at least
        QColor badgeColor = isUsingDarkTheme() ? "#ADACAB" : "#435671";

        QPalette pal = qGuiApp->palette();
        // I really don't think this needs to be larger than 64x64 ever...let's see how it goes
        QPixmap pix(64, 64);
        pix.fill(Qt::transparent);

        QPainter painter;
        painter.begin(&pix);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(badgeColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(pix.rect());

        QFont font = painter.font();
        font.setPixelSize(32);
        font.setBold(true);
        painter.setFont(font);

        // use base color to simulate transparent text that reveals what is behind it, as we have in other icons...ie the base color
        // no I can't use color "Transparent" because the circle/ellipse has already been filled, so the text would be net invisible ;)
        painter.setPen(pal.color(QPalette::Base));
        painter.drawText(pix.rect(), Qt::AlignCenter, initials);
        painter.end();
        cached = pix;
    }
    return cached;
}
}
