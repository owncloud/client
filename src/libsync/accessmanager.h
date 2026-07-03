/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MIRALL_ACCESS_MANAGER_H
#define MIRALL_ACCESS_MANAGER_H

#include "owncloudlib.h"
#include <QNetworkAccessManager>

class QByteArray;
class QUrl;

namespace OCC {
class CookieJar;

/**
 * @brief The AccessManager class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT AccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    void reset();

    static QByteArray generateRequestId();

    AccessManager(QObject *parent);

    QSet<QSslCertificate> customTrustedCaCertificates();

    /***
     * Warning calling those will break running network jobs
     */
    void setCustomTrustedCaCertificates(const QSet<QSslCertificate> &certificates);
    /***
     * Warning calling those will break running network jobs
     */
    void addCustomTrustedCaCertificates(const QSet<QSslCertificate> &certificates);

    CookieJar *ownCloudCookieJar() const;

    /***
     * Remove all errors for already accepted certificates
     */
    QList<QSslError> filterSslErrors(const QList<QSslError> &errors) const;

protected:
    QNetworkReply *createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData = nullptr) override;

private:
    QSet<QSslCertificate> _customTrustedCaCertificates;
};

} // namespace OCC

#endif
