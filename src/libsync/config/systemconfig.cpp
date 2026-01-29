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

namespace chrono = std::chrono;

namespace {
    // Setup related keys
    static constexpr auto KEY_SETUP_ALLOW_SERVER_URL_CHANGE = "Setup/AllowServerUrlChange";
    static constexpr auto KEY_SETUP_SERVER_URL = "Setup/ServerUrl";
    static constexpr auto KEY_SETUP_MOVE_TO_TRASH = "Setup/MoveToTrash";
    // Updater related keys
    static constexpr auto KEY_UPDATER_SKIP_UPDATE_CHECK = "Updater/SkipUpdateCheck";
    // OpenID Connect related keys
    static constexpr auto KEY_OIDC_CLIENT_ID = "OpenIDConnect/ClientId";
    static constexpr auto KEY_OIDC_CLIENT_SECRET = "OpenIDConnect/ClientSecret";
    static constexpr auto KEY_OIDC_PORTS = "OpenIDConnect/Ports";
    static constexpr auto KEY_OIDC_SCOPES = "OpenIDConnect/Scopes";
    static constexpr auto KEY_OIDC_PROMPT = "OpenIDConnect/Prompt";
} // anonymous namespace

SystemConfig::SystemConfig()
{
    auto format = Utility::isWindows() ? QSettings::NativeFormat : QSettings::IniFormat;
    QSettings system(configPath(QOperatingSystemVersion::currentType(), *Theme::instance()), format);

    _allowServerURLChange = system.value(KEY_SETUP_ALLOW_SERVER_URL_CHANGE, true).toBool();
    _serverUrl = system.value(KEY_SETUP_SERVER_URL, QString()).toString();
    _skipUpdateCheck = system.value(KEY_UPDATER_SKIP_UPDATE_CHECK, false).toBool();
    _moveToTrash = system.value(KEY_SETUP_MOVE_TO_TRASH, false).toBool();

    // read OpenID Connect configuration
    auto clientId = system.value(KEY_OIDC_CLIENT_ID, QString()).toString();
    auto clientSecret = system.value(KEY_OIDC_CLIENT_SECRET, QString()).toString();

    QVariant portsVar = system.value(KEY_OIDC_PORTS, "0").toString();
    QList<quint16> ports;
    const auto parts = portsVar.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        bool ok = false;
        const quint16 val = static_cast<quint16>(p.trimmed().toUInt(&ok));
        if (ok) {
            ports.append(val);
        }
    }

    QString scopes = system.value(KEY_OIDC_SCOPES, QString()).toString();
    QString prompt = system.value(KEY_OIDC_PROMPT, QString()).toString();

    _openIdConfig = OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
}

QString SystemConfig::configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme)
{
    if (os == QOperatingSystemVersion::Windows) {
        // we use HKEY_LOCAL_MACHINE\Software\Policies since this is the location whe GPO operates
        return QString("HKEY_LOCAL_MACHINE\\Software\\Policies\\%1\\%2").arg(theme.vendor(), theme.appNameGUI());
    }
    if (os == QOperatingSystemVersion::MacOS) {
        // we use a subfolder to have one common location where in the future more files can be stored (like icons, images and such)
        // ini is used on macOS in contrary to plist because they are easier to maintain
        return QString("/Library/Preferences/%1/%2.ini").arg(theme.orgDomainName(), theme.appName());
    }

    return QString("/etc/%1/%1.ini").arg(theme.appName());
}

bool SystemConfig::allowServerUrlChange() const
{
    return _allowServerURLChange;
}

QString SystemConfig::serverUrl() const
{
    // a theme can provide a hardcoded url which is not subject of change by definition
    auto serverUrl = Theme::instance()->overrideServerUrlV2();
    if (!serverUrl.isEmpty()) {
        return serverUrl;
    }

    return _serverUrl;
}

bool SystemConfig::skipUpdateCheck() const
{
    return _skipUpdateCheck;
}

bool SystemConfig::moveToTrash() const
{
    return _moveToTrash;
}

OpenIdConfig SystemConfig::openIdConfig() const
{
    // system config has precedence here
    if (!_openIdConfig.clientId().isEmpty()) {
        return _openIdConfig;
    }

    // load config from theme
    QString clientId = Theme::instance()->oauthClientId();
    QString clientSecret = Theme::instance()->oauthClientSecret();

    const auto ports = Theme::instance()->oauthPorts();
    QString scopes = Theme::instance()->openIdConnectScopes();
    QString prompt = Theme::instance()->openIdConnectPrompt();

    return OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);

}
}
