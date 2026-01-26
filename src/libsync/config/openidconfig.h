// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Thomas Müller <thomas.mueller@tmit.eu>

#pragma once
#include "owncloudlib.h"

#include <QString>
#include <QList>
#include <QtGlobal>

namespace OCC {

/**
 * @brief The OpenIdConfig class
 * @ingroup libsync
 * @note This class encapsulates the configuration settings required for OpenID Connect authentication.
 */
class OWNCLOUDSYNC_EXPORT OpenIdConfig
{
public:
    OpenIdConfig();
    explicit OpenIdConfig(const QString &clientId, const QString &clientSecret, const QList<quint16> &ports, const QString &scopes, const QString &prompt);

    QString clientId() const;
    QString clientSecret() const;
    QList<quint16> ports() const;
    QString scopes() const;
    QString prompt() const;

private:
    QString _clientId;
    QString _clientSecret;
    QList<quint16> _ports;
    QString _scopes;
    QString _prompt;
};
}