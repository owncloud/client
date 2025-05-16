/*
 * Copyright (C) Lisa Reese (lisa.reese@kiteworks.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace OCC {

struct DiscoverWebFingerServiceResult
{
    QString error;
    QString href;

    bool success() const { return error.isEmpty() && !href.isEmpty(); }
};

class DiscoverWebFingerServiceAdapter : public QObject
{
    Q_OBJECT

public:
    DiscoverWebFingerServiceAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent);

    DiscoverWebFingerServiceResult getResult();

private:
    DiscoverWebFingerServiceResult processReply(QNetworkReply *reply);
    DiscoverWebFingerServiceResult formatError(const QString &errorDetail);

    QNetworkAccessManager *_nam;
    QUrl _url;

    const QString _defaultError = tr("Invalid reply received from server.");
};
}
