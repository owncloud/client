#include "configresolver.h"
#include "../theme.h"
#include "systemconfig.h"

namespace OCC {

bool ConfigResolver::allowServerUrlChange()
{
    return SystemConfig::allowServerUrlChange();
}

QString ConfigResolver::serverUrl()
{
    // a theme can provide a hardcoded url which is not subject of change by definition
    auto serverUrl = Theme::instance()->overrideServerUrlV2();
    if (!serverUrl.isEmpty()) {
        return serverUrl;
    }

    return SystemConfig::serverUrl();;
}

bool ConfigResolver::skipUpdateCheck()
{
    // only in SystemConfig - this is not a Theme option
    return SystemConfig::skipUpdateCheck();
}

OpenIdConfig ConfigResolver::openIdConfig()
{
    // TODO: shall we fill values in from the Theme in case not set within SystemConfig?
    //       not reasonable for clientId/secret, but maybe for ports/scopes/prompt?
    // system config has precedence here
    auto cfg = SystemConfig::openIdConfig();
    if (!cfg.clientId().isEmpty()) {
        return cfg;
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
