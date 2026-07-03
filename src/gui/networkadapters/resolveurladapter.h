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
#include <QSslCertificate>
#include <QSslError>
#include <QUrl>

class QNetworkAccessManager;
class QWidget;

namespace OCC {

/**
 * @brief The ResolveUrlResult struct is a simple container to collect the result of the network request
 * error holds the error message if the operation failed
 * resolvedUrl holds the final resolved url
 * acceptedCertificates is an optional list of certificates the user accepted in the course of resolving the url
 */
struct ResolveUrlResult
{
    QString error;
    QUrl resolvedUrl;
    QSet<QSslCertificate> acceptedCertificates;
    bool success() const { return error.isEmpty() && resolvedUrl.isValid(); }
};

/**
 * @brief The ResolveUrlAdapter class is used to determine if a given URL is reachable via a network request for status.

 * No authentication is in play, it's just a simple first pass reality check to make sure that the URL exists.
 */
class ResolveUrlAdapter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief ResolveUrlAdapter
     * @param nam is the network access manager provided by the caller
     * @param url is the url which will be validated by the request
     * @param errorDialogParent should be the current top level window. Because this adapter will often be used in the new account wizard
     * which is already a modal dialog on top of the main window, and we get some quirky behavior on linux if we use the main window to
     * parent the tls dialog
     * @param parent fits normal QObject parenting concept. The intent of the adapter is to create an instance on the stack and
     * let it naturally go out of scope after getResult has completed, so a parent should not be required in normal use
     */
    explicit ResolveUrlAdapter(QNetworkAccessManager *nam, const QUrl &url, QWidget *errorDialogParent, QObject *parent = nullptr);

    /**
     * @brief getResult executes a network request synchronously
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
    QWidget *_tlsDialogParent;

    QStringList _sslErrors;
    QSet<QSslCertificate> _certificates;
};
}
