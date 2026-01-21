// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Thomas Müller <thomas.mueller@tmit.eu>

#pragma once

#include "openidconfig.h"
#include "owncloudlib.h"

#include <QString>


namespace OCC {

/**
 * @brief The ConfigResolver class
 * @ingroup libsync
 * @note This class provides methods to resolve configuration settings
 * from various sources such as system configuration and themes.
 */
class OWNCLOUDSYNC_EXPORT ConfigResolver
{
public:
    /**
     * Determine if changing the server URL is allowed.
     * This checks the system configuration for the relevant setting.
     * @return True if changing the server URL is allowed, false otherwise.
     */
    static bool allowServerUrlChange();

    /**
     * Retrieve the server URL.
     * This checks the system configuration and theme overrides for the server URL.
     * @return The server URL as a QString. If not set, returns an empty string.
     */
    static QString serverUrl();

    /**
     * Determine if update checks should be skipped.
     * This checks the system configuration for the relevant setting.
     * @return True if update checks should be skipped, false otherwise.
     */
    static bool skipUpdateCheck();

    /**
     * Retrieve the OpenID Connect configuration.
     * This checks the system configuration and theme settings for OpenID Connect parameters.
     * @return An OpenIdConfig object containing the OpenID Connect settings.
     */
    static OpenIdConfig openIdConfig();
};
}
