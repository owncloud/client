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

#pragma once

#include "resources/owncloudresources.h"

#include <QHash>
#include <QIcon>
#include <QUuid>

namespace OCC {

class OWNCLOUDRESOURCES_EXPORT IconResources
{
public:
    IconResources() = delete;
    ~IconResources() = delete;

    /**
     * checks whether the system is using dark theme. If this returns false assume system is using light theme.
     */
    static bool isUsingDarkTheme();

    static QIcon getCoreIcon(const QString &name);

    /**
     * Returns general branded icons for the application and wizard. If for some reason a branded icon does not exist we may return
     * a default "owncloud" icon but should never actually happen in practice aside from the wizard_footer_logo which most brands don't
     * provide, so the default oc version is a blank icon
     */
    static QIcon getUniversalIcon(const QString &name);

    // tries to find the icon with "name" in the branded resources subdir "iconTheme". If the branding does not contain the icon,
    // automatically fall back to default/owncloud icon which should always be there. If that fails, we have to assume the filename was
    // "bad" -> ie that no such icon exists in the app.
    static QIcon getThemedIcon(const QString &iconTheme, const QString &name);

    static bool hasMonoIcons();
    static bool useMonoTrayIcons();
    static void setUseMonoTrayIcons(bool useMono);
    static QIcon themedTrayIcon(const QString &name, bool sysTrayMenuVisible, bool trayIsDark);

    // attempts to find the path for an icon theme in branded resources. If the theme does not exist in the branding,
    // the fallback's theme path is automatically returned. If this also fails we have to suspect there is no
    // such theme name in play for the app
    static QString findIconPath(const QString &iconTheme, const QString &name, bool branded = true);

    // this can stay here for now but may move to a more central avatar handling location in future
    // this impl returns a default avatar which has the user name's initials placed in a circle.
    // the colors for the avatar currently match the core icon colors (shades of gray, which can change depending on whether system
    // theme is light or dark)
    // the avatar is cached to avoid painting it every time it's needed.
    static QIcon buildAvatar(const QString &initials, QUuid accountUid);

    static void handleSystemStyleChanged();

private:
    // map icon path including name and file extension to icon for themed icons
    static inline QHash<QString, QIcon> _themedIconCache = {};
    // map icon name alone, with no path or extension, to icon for core icons.
    // Note these get wiped out whenever the system theme changes
    // Also note default avatars for accounts live in this core cache as well
    static inline QHash<QString, QIcon> _coreIconCache = {};

    // theme name can be white, dark, black, colored or universal
    static inline QHash<QString, QString> _brandedThemePaths = {};
    // these are the paths associated with the base, oc theme. These are only needed when/if the branded theme
    // does not provide the theme at all, or perhaps individual icons are missing. Since we are in control of all branding
    // this really should never be needed.
    static inline QHash<QString, QString> _fallbackThemePaths = {};

    // this is a user setting which can change at runtime!
    static inline bool _useMonoTrayIcons = false;

    static QString brandedRootPath();
    static QString pathForTheme(const QString &theme, bool branded = true);

    static const inline QString _defaultRootPath = ":/client/ownCloud/theme";
    // *please* do not use this var directly as it may not be initialzed yet. Instead, use brandedRootPath() to ensure the value is legit
    static inline QString _brandedRootPath = {};


    // after deeper investigation, it appears most of these theme names are related to system (tray) icons. The values of these "themes"
    // is the folder name in the resources so should never be mucked with unless you want to edit all the theme resources to match the new
    // scheme!

    // dark and colored contain non-mono tray icons with colorful state badges
    // "dark" should be used with dark system theme - the app icon is light or white and the state badge is colored
    static const inline QString _darkTheme = "dark";
    // colored location contains dark app icons with colored state badge. This is appropriate for light system theme so I have named it
    // accordingly.
    static const inline QString _lightTheme = "colored";

    // these have consistent color scheme both in terms of the app icon and the badge color. In essence these are "mono" icons:
    // white/light colored mono icons that should be used with "dark" system tray
    static const inline QString _whiteMonoTheme = "white";
    // dark colored mono icons for use with "light" sytem tray
    static const inline QString _blackMonoTheme = "black";

    // universal "theme" is where we should find primary branding icons, eg app and wizard icons.
    static const inline QString _universalTheme = "universal";
};
}
