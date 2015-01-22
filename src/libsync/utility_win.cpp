/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */
#define NTDDI_VERSION 0x06010000
#include <shlobj.h>
#include <winbase.h>
#include <windows.h>

static const char runPathC[] = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

static bool isWinVistaOrHigher() {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    if (osvi.dwMajorVersion < 6) 
       return false;
    return true;

}

static void setupFavLink_private(const QString &folder)
{
    // Windows Explorer: Place under "Favorites" (Links)
    QString profile;
    if (isWinVistaOrHigher()) {
        wchar_t *path;
        SHGetKnownFolderPath(FOLDERID_Links, 0, NULL, &path);
        profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
    } else {
        wchar_t path[MAX_PATH];
        SHGetSpecialFolderPath(0, path, CSIDL_PROFILE, FALSE); 
        profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
    }
    QDir folderDir(QDir::fromNativeSeparators(folder));
    if (isWinVistaOrHigher()) {
        QString linkName = profile + QLatin1String("/") + folderDir.dirName() + QLatin1String(".lnk");
    } else {
        QString linkName = profile+QLatin1String("/Links/") + folderDir.dirName() + QLatin1String(".lnk");
    }
    if (!QFile::link(folder, linkName))
        qDebug() << Q_FUNC_INFO << "linking" << folder << "to" << linkName << "failed!";
}

bool hasLaunchOnStartup_private(const QString &appName)
{
    QString runPath = QLatin1String(runPathC);
    QSettings settings(runPath, QSettings::NativeFormat);
    return settings.contains(appName);
}

void setLaunchOnStartup_private(const QString &appName, const QString& guiName, bool enable)
{
    Q_UNUSED(guiName);
    QString runPath = QLatin1String(runPathC);
    QSettings settings(runPath, QSettings::NativeFormat);
    if (enable) {
        settings.setValue(appName, QCoreApplication::applicationFilePath().replace('/','\\'));
    } else {
        settings.remove(appName);
    }
}

static inline bool hasDarkSystray_private()
{
    return true;
}
