#pragma once

#include "../theme.h"
#include "openidconfig.h"
#include "owncloudlib.h"

#include <QOperatingSystemVersion>
#include <QString>
#include <QVariant>


namespace OCC {

/**
 * @brief The ConfigResolver class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT ConfigResolver
{
public:
    // account setup
    static bool allowServerUrlChange();
    static QString serverUrl();

    // Return true if update checks should be skipped (e.g., via env or system config)
    static bool skipUpdateCheck();

    // OAuth/OpenID Connect
    static OpenIdConfig openIdConfig();
};
}
