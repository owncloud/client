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
    explicit SystemConfig();
    /**
     * Determine if changing the server URL is allowed based on system configuration.
     * This value is only relevant if SystemConfig::serverUrl() returns a non-empty string.
     * @return True if changing the server URL is allowed, false otherwise.
     */
    bool allowServerUrlChange() const;

    /**
     * Retrieve the server URL from the system configuration or theme.
     * @return The server URL as a QString. If not set, returns an empty string.
     */
    QString serverUrl() const;

    /**
     * Retrieve the OpenID Connect configuration from the system configuration or the theme.
     * The configuration includes client ID, client secret, ports, scopes, and prompt settings.
     * The returned OpenIdConfig object may have empty values if not set in the system configuration.
     * @return An OpenIdConfig object containing the OpenID Connect settings.
     */
    OpenIdConfig openIdConfig() const;

    /**
     * Determine if update checks should be skipped based on system configuration.
     * @return True if update checks should be skipped, false otherwise.
     */
    bool skipUpdateCheck() const;

    /**
     * Get the path to the system configuration file or registry path based on the operating system and theme.
     * @param os operating system type
     * @param theme the theme instance
     * @return the path to the system configuration file or registry path
     * @internal kept public for testing purposes
     */
    static QString configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme);

private:
    void loadOpenIdConfig(const QSettings &system);

private: // System settings keys
    // Setup related keys
    inline static const QString SetupAllowServerUrlChangeKey = QStringLiteral("Setup/AllowServerUrlChange");
    inline static const QString SetupServerUrlKey = QStringLiteral("Setup/ServerUrl");
    // Updater related keys
    inline static const QString UpdaterSkipUpdateCheckKey = QStringLiteral("Updater/SkipUpdateCheck");
    // OpenID Connect related keys
    inline static const QString OidcClientIdKey = QStringLiteral("OpenIDConnect/ClientId");
    inline static const QString OidcClientSecretKey = QStringLiteral("OpenIDConnect/ClientSecret");
    inline static const QString OidcPortsKey = QStringLiteral("OpenIDConnect/Ports");
    inline static const QString OidcScopesKey = QStringLiteral("OpenIDConnect/Scopes");
    inline static const QString OidcPromptKey = QStringLiteral("OpenIDConnect/Prompt");

private:
    bool _allowServerURLChange;
    QString _serverUrl;
    bool _skipUpdateCheck;
    OpenIdConfig _openIdConfig;
};

} // OCC namespace
