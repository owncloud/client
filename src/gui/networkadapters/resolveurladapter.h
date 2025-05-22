/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

/**
 * @brief The ResolveUrlResult struct is a simple container to collect the result of the network request
 * error holds the error message if the operation failed
 * resolvedUrl holds the final resolved url
 * acceptedCertificats is a list of certificates the user accepted in the course of resolving the url
 */
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
    /**
     * @brief ResolveUrlAdapter
     * @param nam is the network access manager provided by the caller
     * @param url is the url for the request
     * @param parent fits normal QObject parenting concept. The intent of the adapter is to create an instance on the stack and
     * let it naturally go out of scope after getResult has completed, so a parent should not be required in normal use
     */
    explicit ResolveUrlAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult executes a network request asynchronously
     * @return the result of the request
     *
     * Important detail: if the reply contains certificates the user will be asked to accept them before this function returns.
     * If the user rejects the certificates, the net result is a ssl failure which will be reported as such.
     */
    ResolveUrlResult getResult();

private:
    void handleSslErrors(const QList<QSslError> &errors);

    QNetworkAccessManager *_nam;
    QUrl _url;
    QStringList _sslErrors;
    QList<QSslCertificate> _certificates;
};
}
