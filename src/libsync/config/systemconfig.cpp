// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Thomas Müller <thomas.mueller@tmit.eu>

#include "systemconfig.h"

#include "../theme.h"
#include "common/utility.h"
#include "openidconfig.h"

#include <QFile>
#include <QOperatingSystemVersion>
#include <QSettings>

namespace OCC {

Q_LOGGING_CATEGORY(lcSystemConfig, "sync.systemconfig", QtInfoMsg)

namespace chrono = std::chrono;

SystemConfig::SystemConfig()
{
    const bool allowSystemConfigOverrides = Theme::instance()->allowSystemConfigOverrides();
    auto format = Utility::isWindows() ? QSettings::NativeFormat : QSettings::IniFormat;
    QSettings system(configPath(QOperatingSystemVersion::currentType(), *Theme::instance()), format);

    if (allowSystemConfigOverrides && system.contains(SetupServerUrlKey)) {
        _serverUrl = system.value(SetupServerUrlKey).toString();
    } else {
        _serverUrl = Theme::instance()->overrideServerUrlV2();
    }
    if (allowSystemConfigOverrides && system.contains(SetupAllowServerUrlChangeKey)) {
        _allowServerURLChange = system.value(SetupAllowServerUrlChangeKey).toBool();
    } else {
        // If a theme provides a hardcoded URL, do not allow for URL change.
        _allowServerURLChange = Theme::instance()->overrideServerUrlV2().isEmpty();
    }
    _skipUpdateCheck = system.value(UpdaterSkipUpdateCheckKey, false).toBool();

    loadOpenIdConfig(system);
}

void SystemConfig::loadOpenIdConfig(const QSettings &system)
{
    QString clientId;
    QString clientSecret;
    QVector<quint16> ports;
    QString scopes;
    QString prompt;
    auto theme = Theme::instance();

    if (theme->allowSystemConfigOverrides()) {
        if (system.contains(OidcClientIdKey) || system.contains(OidcClientSecretKey) || system.contains(OidcPortsKey) || system.contains(OidcScopesKey)
            || system.contains(OidcPromptKey)) {
            // Load *all* settings from the system config.
            // When done, check if the config is valid. If it is not valid, fall back to the theme.
            clientId = system.value(OidcClientIdKey).toString();
            clientSecret = system.value(OidcClientSecretKey, QString()).toString();

            if (system.contains(OidcPortsKey)) {
                QVariant portsVar = system.value(OidcPortsKey).toString();
                const auto parts = portsVar.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString &p : parts) {
                    bool ok = false;
                    const quint16 val = static_cast<quint16>(p.trimmed().toUInt(&ok));
                    if (ok) {
                        ports.append(val);
                    }
                }
            } else {
                ports.append(0); // 0 means any port
            }

            scopes = system.value(OidcScopesKey, QString()).toString();
            prompt = system.value(OidcPortsKey, QString()).toString();
            _openIdConfig = OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
            if (_openIdConfig.isValid())
                return;
            else
                qCWarning(lcSystemConfig) << "Invalid OpenIDConnect configuration in system config, falling back to defaults";
        }
    }

    // Load *all* settings from the theme.
    clientId = theme->oauthClientId();
    clientSecret = theme->oauthClientSecret();
    ports = theme->oauthPorts();
    scopes = theme->openIdConnectScopes();
    prompt = theme->openIdConnectPrompt();
    _openIdConfig = OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
}

QString SystemConfig::configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme)
{
    // Important: these paths conform to how names typically work on the systems on which they are used. This includes usage of upper-/lowercase.

    if (os == QOperatingSystemVersion::Windows) {
        // We use HKEY_LOCAL_MACHINE\Software\Policies since this is the location where GPO operates.
        // Note: use of uppercase/camelcase is common.
        return QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\Policies\\%1\\%2").arg(theme.vendor(), theme.appNameGUI());
    }

    if (os == QOperatingSystemVersion::MacOS) {
        // We use a subfolder to have one common location where in the future more files can be stored (like icons, images and such)
        // ini is used on macOS in contrary to plist because they are easier to maintain.
        // Note: rev-domain notation and lowercase is typically used.
        return QStringLiteral("/Library/Preferences/%1/%2.ini").arg(theme.orgDomainName(), theme.appName());
    }

    // On Unix style systems, the application name in lowercase is typically used.
    return QStringLiteral("/etc/%1/%1.ini").arg(theme.appName());
}

bool SystemConfig::allowServerUrlChange() const
{
    return _allowServerURLChange;
}

QString SystemConfig::serverUrl() const
{
    return _serverUrl;
}

bool SystemConfig::skipUpdateCheck() const
{
    return _skipUpdateCheck;
}

OpenIdConfig SystemConfig::openIdConfig() const
{
    return _openIdConfig;
}

} // OCC namespace
