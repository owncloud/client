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
#include <shlobj.h>
#include <winbase.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <shlguid.h>


static const char runPathC[] = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

typedef HRESULT (WINAPI* lpSHGetKnownFolderPath)(
    REFKNOWNFOLDERID rfid,
    DWORD dwFlags,
    HANDLE hToken,
    PWSTR *ppszPath
);


static void setupFavLink_private(const QString &folder)
{
    // Windows Explorer: Place under "Favorites" (Links)
    QString profile;
    bool hasKnownFolderPath = false;
    HMODULE hndl_shell32;
    lpSHGetKnownFolderPath pSHGetKnownFolderPath;

    /* Always check the return value of LoadLibrary. */
    hndl_shell32 = LoadLibrary(L"shell32");
    if (NULL != hndl_shell32) {
        pSHGetKnownFolderPath = (lpSHGetKnownFolderPath) GetProcAddress(hndl_shell32, "SHGetKnownFolderPath");

        if(pSHGetKnownFolderPath != NULL) {
            wchar_t *path;
            if (SUCCEEDED(pSHGetKnownFolderPath(FOLDERID_Links, 0,NULL,&path))) {
		hasKnownFolderPath = true;
                profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
                CoTaskMemFree(path);
            }
        }
        else {
            // We're on .
            wchar_t path[MAX_PATH];
            SHGetSpecialFolderPath(0, path, CSIDL_PROFILE, FALSE);
            profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
        }
        FreeLibrary(hndl_shell32);
    }
    else {
	// In case we cannot load shell32.dll, choose legacy function that surely exists
        wchar_t path[MAX_PATH];
        SHGetSpecialFolderPath(0, path, CSIDL_PROFILE, FALSE);
        profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
    } 
    QDir folderDir(QDir::fromNativeSeparators(folder));
    QString linkName;
    if (hasKnownFolderPath) {
        linkName = profile + QLatin1String("/") + folderDir.dirName() + QLatin1String(".lnk");
    } else {
        linkName = profile+QLatin1String("/Links/") + folderDir.dirName() + QLatin1String(".lnk");
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
