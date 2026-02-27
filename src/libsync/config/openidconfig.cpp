// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Thomas Müller <thomas.mueller@tmit.eu>

#include "openidconfig.h"

namespace OCC {

OpenIdConfig::OpenIdConfig()
{
}

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

bool OpenIdConfig::isValid() const
{
    return !_clientId.isEmpty() && !_ports.isEmpty();
}

}
