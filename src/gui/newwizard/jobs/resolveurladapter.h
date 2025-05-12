/*
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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
#include <QSslCertificate>
#include <QSslError>
#include <QUrl>

class QNetworkAccessManager;

namespace OCC {

struct ResolveUrlResult
{
    QString error;
    QUrl resolvedUrl;
    QList<QSslCertificate> acceptedCertificates;
    bool success() const { return error.isEmpty() && resolvedUrl.isValid(); }
};

class ResolveUrlAdapter : public QObject
{
    Q_OBJECT

public:
    explicit ResolveUrlAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent);

    ResolveUrlResult getResult();

private:
    void handleSslErrors(const QList<QSslError> &errors);

    QNetworkAccessManager *_nam;
    QUrl _url;
    QStringList _sslErrors;
    QList<QSslCertificate> _certificates;
};
}
