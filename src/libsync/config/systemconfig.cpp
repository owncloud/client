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
    _serverUrl = Theme::instance()->overrideServerUrlV2();
    // If a theme provides a hardcoded URL, do not allow for URL change.
    _allowServerURLChange = Theme::instance()->overrideServerUrlV2().isEmpty();
    _skipUpdateCheck = false;
    _openIdConfig = loadOpenIdConfigFromTheme();

    if (!Theme::instance()->allowSystemConfigOverrides())
        return;

    // Load all overrides

    auto format = Utility::isWindows() ? QSettings::NativeFormat : QSettings::IniFormat;
    const QSettings system(configPath(QOperatingSystemVersion::currentType(), *Theme::instance()), format);

    _serverUrl = system.value(SetupServerUrlKey, QString()).toString();
    if (system.contains(SetupAllowServerUrlChangeKey)) {
        _allowServerURLChange = system.value(SetupAllowServerUrlChangeKey).toBool();
    }

    _skipUpdateCheck = system.value(UpdaterSkipUpdateCheckKey, false).toBool();

    OpenIdConfig systemConfig = loadOpenIdConfigFromSystemConfig(system);
    if (systemConfig.isValid()) {
        qCInfo(lcSystemConfig()) << "Using OpenID config from system config";
        _openIdConfig = systemConfig;
    }
}

OpenIdConfig SystemConfig::loadOpenIdConfigFromTheme()
{
    Theme *theme = Theme::instance();

    QString clientId = theme->oauthClientId();
    QString clientSecret = theme->oauthClientSecret();
    QVector<quint16> ports = theme->oauthPorts();
    QString scopes = theme->openIdConnectScopes();
    QString prompt = theme->openIdConnectPrompt();

    OpenIdConfig cfg(clientId, clientSecret, ports, scopes, prompt);
    OC_ASSERT(cfg.isValid());

    return cfg;
}

OpenIdConfig SystemConfig::loadOpenIdConfigFromSystemConfig(const QSettings &system)
{
    QString clientId = system.value(OidcClientIdKey, QString()).toString();
    QString clientSecret = system.value(OidcClientSecretKey, QString()).toString();
    QString scopes = system.value(OidcScopesKey, QString()).toString();
    QString prompt = system.value(OidcPortsKey, QString()).toString();

    QVector<quint16> ports;
    QVariant portsVar = system.value(OidcPortsKey, QString()).toString();
    const auto parts = portsVar.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        bool ok = false;
        const quint16 val = static_cast<quint16>(p.trimmed().toUInt(&ok));
        if (ok) {
            ports.append(val);
        }
    }

    if (ports.isEmpty())
        ports.append(0); // 0 means any port

    return OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
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
