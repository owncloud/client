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
#include <QMessageBox>

static const char runPathC[] = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const char panePathC[] = "Software\\Classes\\CLSID\\{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\\Instance\\InitPropertyBag";
static const char panePath2C[] = "Software\\Classes\\CLSID\\Wow6432Node\\{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\\Instance\\InitPropertyBag";

static bool CreateRegistryKeyValue( HKEY hKeyParent, PWCHAR KeyName, PWCHAR ValueName, PBYTE pValue,  DWORD dwValueSize )
{
    DWORD dwDisposition;
    HKEY  hKey;
    DWORD dwRet;

    dwRet =
        RegCreateKeyEx(
            hKeyParent,
            KeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE | KEY_WOW64_64KEY,
            NULL,
            &hKey,
            &dwDisposition);

    if (dwRet != ERROR_SUCCESS)
    {
        printf("Error opening or creating key.\n");
        return FALSE;
    }

    // Attempt to set the value of the key.
    // If the call fails, close the key and return.
    if (ERROR_SUCCESS !=
            RegSetValueEx(
                hKey,
                ValueName,
                0,
                REG_EXPAND_SZ,
                pValue,
                dwValueSize))
    {
        printf("Could not set registry value.\n");
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

static void setupFavLink_private(const QString &folder)
{
    // Windows Explorer: Place under "Favorites" (Links)
    wchar_t path[MAX_PATH];
    SHGetSpecialFolderPath(0, path, CSIDL_PROFILE, FALSE);
    QString profile =  QDir::fromNativeSeparators(QString::fromWCharArray(path));
    QDir folderDir(QDir::fromNativeSeparators(folder));
    QString linkName = profile+QLatin1String("/Links/") + folderDir.dirName() + QLatin1String(".lnk");
    if (!QFile::link(folder, linkName))
        qDebug() << Q_FUNC_INFO << "linking" << folder << "to" << linkName << "failed!";
}

static void updateNavPanel_private(const QString &folder)
{
    // Set folder to navigation pane
    QString panePath = QLatin1String(panePathC);
    panePath.replace("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}", QLatin1String(APPLICATION_CLSID));
    wchar_t* newPanePath = new wchar_t[panePath.length()+1]();
    panePath.toWCharArray(newPanePath);
    QString panePath2 = QLatin1String(panePath2C);
    panePath2.replace("{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}", QLatin1String(APPLICATION_CLSID));
    wchar_t* newPanePath2 = new wchar_t[panePath2.length()+1]();
    panePath2.toWCharArray(newPanePath2);
    QString f = folder;
    f.replace('/','\\');
    wchar_t* path = new wchar_t[f.length()+1]();
    f.toWCharArray(path);
    wchar_t* valuename = new wchar_t[17]();
    wcsncpy(valuename, L"TargetFolderPath", 16);

    CreateRegistryKeyValue ( HKEY_CURRENT_USER, newPanePath, valuename, (PBYTE)path, sizeof(wchar_t) * f.length());
    CreateRegistryKeyValue ( HKEY_CURRENT_USER, newPanePath2, valuename, (PBYTE)path, sizeof(wchar_t) * f.length());

    // memory cleanup
    delete newPanePath;
    delete path;
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
