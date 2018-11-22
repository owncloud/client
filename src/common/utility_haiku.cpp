/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <QDir>
#include <QFile>
#include <QString>

#include <FindDirectory.h>
#include <Path.h>

namespace OCC {

static void setupFavLink_private(const QString &folder)
{
	Q_UNUSED(folder);
    // Haiku doesn't really have that concept of bookmarks on folders
}

// returns the autostart directory the Haiku way
// should probably use launch_daemon service directly though
// instead of the old way
QString getUserAutostartDir_private()
{
    BPath path;
    find_directory(B_USER_SETTINGS_DIRECTORY, &path);
    QString config = QString::fromUtf8(path.Path());
    config += QLatin1String("/boot/launch/");
    return config;
}

bool hasLaunchOnStartup_private(const QString &appName)
{
    QString startupFileLocation = getUserAutostartDir_private() + appName;
    return QFile::exists(startupFileLocation);
}

void setLaunchOnStartup_private(const QString &appName, const QString &guiName, bool enable)
{
	Q_UNUSED(guiName);
    QString userAutoStartPath = getUserAutostartDir_private();
    QString startupFileLocation = userAutoStartPath + appName;
    if (enable) {
        if (!QDir().exists(userAutoStartPath) && !QDir().mkpath(userAutoStartPath)) {
            qCWarning(lcUtility) << "Could not create autostart folder" << userAutoStartPath;
            return;
        }
        if (!QFile::link(QCoreApplication::applicationFilePath(), startupFileLocation)) {
            qCWarning(lcUtility) << "Could not write autostart symlink" << startupFileLocation;
            return;
        }
    } else {
        if (!QFile::remove(startupFileLocation)) {
            qCWarning(lcUtility) << "Could not remove autostart symlink";
        }
    }
}

static inline bool hasDarkSystray_private()
{
    return true;
}

} // namespace OCC
