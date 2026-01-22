// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Thomas Müller <thomas.mueller@tmit.eu>

#pragma once

#include "../theme.h"
#include "owncloudlib.h"
#include "openidconfig.h"

#include <QOperatingSystemVersion>
#include <QString>
#include <QVariant>


namespace OCC {

/**
 * @brief The SystemConfig class
 * @ingroup libsync
 * @note This class provides access to system-wide configuration settings.
 * These settings are typically read-only and affect the behavior of the application globally.
 * On Windows, settings are read from the registry path:
 * HKEY_LOCAL_MACHINE\Software\Policies\<Vendor>\<AppName>
 * On macOS, settings are read from the file:
 * /Library/Preferences/<OrgDomainName>/<AppName>.ini
 * On Linux and other Unix-like systems, settings are read from the file:
 * /etc/<AppName>/<AppName>.ini
 *
 * @example owncloud.ini
 * [Setup]
 * ServerUrl=https://cloud.example.com
 * AllowServerUrlChange=false
 * MoveToTrash=true
 *
 * [Updater]
 * SkipUpdateCheck=true
 *
 * [OpenIDConnect]
 * ClientId=your-client-id
 * ClientSecret=your-client-secret
 * Ports=8080,8443
 * Scopes=openid offline_access email profile
 * Prompt=select_account consent
 *
 * @example owncloud.reg (Windows Registry)
 * Windows Registry Editor Version 5.00
 *
 * [HKEY_LOCAL_MACHINE\Software\Policies\ownCloud\ownCloud]
 *
 * [HKEY_LOCAL_MACHINE\Software\Policies\ownCloud\ownCloud\Setup]
 * "ServerUrl"="https://cloud.example.com"
 * "AllowServerUrlChange"=dword:00000000
 * "MoveToTrash"=dword:00000001
 *
 * [HKEY_LOCAL_MACHINE\Software\Policies\ownCloud\ownCloud\Updater]
 * "SkipUpdateCheck"=dword:00000001
 *
 * [HKEY_LOCAL_MACHINE\Software\Policies\ownCloud\ownCloud\OpenIDConnect]
 * "ClientId"="your-client-id"
 * "ClientSecret"="your-client-secret"
 * "Ports"="8080,8443"
 * "Scopes"="openid offline_access email profile"
 * "Prompt"="select_account consent"
 *
 */
class OWNCLOUDSYNC_EXPORT SystemConfig
{
public:
    /**
     * Determine if changing the server URL is allowed based on system configuration.
     * This value is only relevant if SystemConfig::serverUrl() returns a non-empty string.
     * @return True if changing the server URL is allowed, false otherwise.
     */

    static bool allowServerUrlChange();
    /**
     * Retrieve the server URL from the system configuration.
     * @return The server URL as a QString. If not set, returns an empty string.
     */
    static QString serverUrl();

    /**
     * Retrieve the OpenID Connect configuration from the system configuration.
     * The configuration includes client ID, client secret, ports, scopes, and prompt settings.
     * The returned OpenIdConfig object may have empty values if not set in the system configuration.
     * @return An OpenIdConfig object containing the OpenID Connect settings.
     */
    static OpenIdConfig openIdConfig();

    /**
     * Determine if update checks should be skipped based on system configuration.
     * @return True if update checks should be skipped, false otherwise.
     */
    static bool skipUpdateCheck();

    /**
     * Determine if files should be moved to trash based on system configuration.
     * @return True if files should be moved to trash, false otherwise.
     */
    static bool moveToTrash();

    // General purpose function
    /**
     * Retrieve a configuration value from the system configuration.
     * @param key The key of the configuration value to retrieve.
     * @param defaultValue The default value to return if the key is not found.
     * @return The configuration value associated with the key, or the default value if the key is not found.
     */
    static QVariant value(QAnyStringView key, const QVariant &defaultValue);
    /**
     * Get the path to the system configuration file or registry path based on the operating system and theme.
     * @param os operating system type
     * @param theme the theme instance
     * @return the path to the system configuration file or registry path
     */
    static QString configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme);
};
}
