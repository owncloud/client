//
// Created by deepdiver on 16/01/2026.
//

#include "openidconfig.h"

namespace OCC {

OpenIdConfig::OpenIdConfig(const QString &clientId, const QString &clientSecret, const QList<quint16> &ports, const QString &scopes, const QString &prompt)
    : _clientId(clientId)
    , _clientSecret(clientSecret)
    , _ports(ports)
    , _scopes(scopes)
    , _prompt(prompt)
{
}

QString OpenIdConfig::clientId() const {
    return _clientId;
}

QString OpenIdConfig::clientSecret() const {
    return _clientSecret;
}

QList<quint16> OpenIdConfig::ports() const {
    return _ports;
}

QString OpenIdConfig::scopes() const {
    return _scopes;
}

QString OpenIdConfig::prompt() const {
    return _prompt;
}

}
