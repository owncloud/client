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

QVariant SystemConfig::value(QAnyStringView key, const QVariant &defaultValue)
{
    auto format = Utility::isWindows() ? QSettings::NativeFormat : QSettings::IniFormat;
    QSettings system(configPath(QOperatingSystemVersion::currentType(), *Theme::instance()), format);

    return system.value(key, defaultValue);
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

bool SystemConfig::allowServerUrlChange()
{
    return value(KEY_SETUP_ALLOW_SERVER_URL_CHANGE, true).toBool();
}

QString SystemConfig::serverUrl()
{
    return value(KEY_SETUP_SERVER_URL, QString()).toString();
}

bool SystemConfig::skipUpdateCheck()
{
    return value(KEY_UPDATER_SKIP_UPDATE_CHECK, false).toBool();
}

bool SystemConfig::moveToTrash()
{
    // check settings first; if not present, fall back to the Theme default
    QVariant v = value(KEY_SETUP_MOVE_TO_TRASH, QVariant());
    if (v.isValid()) {
        return v.toBool();
    }

    return false;
}

OpenIdConfig SystemConfig::openIdConfig()
{
    auto clientId = value(KEY_OIDC_CLIENT_ID, QString()).toString();
    auto clientSecret = value(KEY_OIDC_CLIENT_SECRET, QString()).toString();

    QVariant portsVar = value(KEY_OIDC_PORTS, "0").toString();
    QList<quint16> ports;
    const auto parts = portsVar.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        bool ok = false;
        const quint16 val = static_cast<quint16>(p.trimmed().toUInt(&ok));
        if (ok) {
            ports.append(val);
        }
    }

    QString scopes = value(KEY_OIDC_SCOPES, QString()).toString();
    QString prompt = value(KEY_OIDC_PROMPT, QString()).toString();

    return OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
}
}
