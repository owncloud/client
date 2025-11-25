
#include "common/asserts.h"
#include "common/utility.h"
#include "systemconfig.h"
#include "theme.h"

#include <QFile>
#include <QSettings>
#include <QOperatingSystemVersion>

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
}
