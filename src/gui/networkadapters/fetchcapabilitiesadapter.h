/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QUrl>

#include "capabilities.h"

class QNetworkAccessManager;

namespace OCC {

struct FetchCapabilitiesResult
{
    QString error;
    Capabilities capabilities = Capabilities(QUrl(), {});

    bool success() const { return error.isEmpty() && capabilities.isValid(); }
};

class FetchCapabilitiesAdapter : public QObject
{
    Q_OBJECT
public:
    explicit FetchCapabilitiesAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    FetchCapabilitiesResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;
    QString _authorizationHeader;
};

}
