
#pragma once

#include "owncloudlib.h"
#include "theme.h"

#include <QOperatingSystemVersion>
#include <QString>
#include <QVariant>


namespace OCC {

/**
 * @brief The SystemConfig class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT SystemConfig
{
public:
    // Access system configuration
    static bool allowServerUrlChange();
    static QString serverUrl();

    // General purpose function
    static QVariant value(QAnyStringView key, const QVariant &defaultValue);
    static QString configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme);

};
}
