#include "systemconfig.h"

#include "../theme.h"
#include "common/utility.h"
#include "openidconfig.h"

#include <QFile>
#include <QOperatingSystemVersion>
#include <QSettings>

namespace OCC {

namespace chrono = std::chrono;

QVariant SystemConfig::value(QAnyStringView key, const QVariant &defaultValue)
{
    auto format = Utility::isWindows() ? QSettings::NativeFormat : QSettings::IniFormat;
    QSettings system(configPath(QOperatingSystemVersion::currentType(), *Theme::instance()), format);

    return system.value(key, defaultValue);
}
QString SystemConfig::configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme)
{
    if (os == QOperatingSystemVersion::Windows) {
        return QString("HKEY_LOCAL_MACHINE\\Software\\%1\\%2").arg(theme.vendor(), theme.appNameGUI());
    }
    if (os == QOperatingSystemVersion::MacOS) {
        return QString("/Library/Preferences/%1/%2.ini").arg(theme.orgDomainName(), theme.appName());
    }

    return QString("/etc/%1/%1.ini").arg(theme.appName());
}

bool SystemConfig::allowServerUrlChange()
{
    return value("Setup/AllowServerUrlChange", true).toBool();
}

QString SystemConfig::serverUrl()
{
    return value("Setup/ServerUrl", QString()).toString();
}

OpenIdConfig SystemConfig::openIdConfig()
{
    auto clientId = value("OpenIDConnect/ClientId", QString()).toString();
    auto clientSecret = value("OpenIDConnect/ClientSecret", QString()).toString();

    QVariant portsVar = value("OpenIDConnect/Ports", "0").toString();
    QList<quint16> ports;
    const auto parts = portsVar.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        bool ok = false;
        const quint16 val = static_cast<quint16>(p.trimmed().toUInt(&ok));
        if (ok) {
            ports.append(val);
        }
    }

    QString scopes = value("OpenIDConnect/Scopes", QString()).toString();
    QString prompt = value("OpenIDConnect/Prompt", QString()).toString();

    return OpenIdConfig(clientId, clientSecret, ports, scopes, prompt);
}
}
