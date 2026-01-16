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
 */
class OWNCLOUDSYNC_EXPORT SystemConfig
{
public:
    // Access system configuration
    // section: account setup
    static bool allowServerUrlChange();
    static QString serverUrl();

    // section: OpenID Connect
    static OpenIdConfig openIdConfig();

    // section: updater
    static bool skipUpdateCheck();

    // section: trash handling
    static bool moveToTrash();

    // General purpose function
    static QVariant value(QAnyStringView key, const QVariant &defaultValue);
    static QString configPath(const QOperatingSystemVersion::OSType& os, const Theme& theme);
};
}
