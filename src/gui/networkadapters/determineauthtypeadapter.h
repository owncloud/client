/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "creds/credentialssupport.h"
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

namespace OCC {

struct DetermineAuthTypeResult
{
    QString error;
    AuthenticationType type = AuthenticationType::Unknown;
    bool success() const { return error.isEmpty(); }
};

/**
 * @brief The DetermineAuthTypeAdapter class allows us to validate that a given server URL supports oauth authentication.
 * If oauth is not supported, the authentication type will be reported as unknown. It is up to the caller to report an
 * error or handle this condition in some responsible manner.
 *
 * This routine used to identify basic http server authentication as well but the Basic type is obsolete after client 6.0
 *
 */
class DetermineAuthTypeAdapter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief DetermineAuthTypeAdapter
     * @param nam the network access manager which will run the request.
     * @param url the url for the request.
     * @param parent follows the typical QObject parenting scheme, but this param is rarely needed since an instance should normally be
     * created on the stack and will naturally destruct when it goes out of scope.
     */
    DetermineAuthTypeAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult runs the request to determine authentication type synchronously
     * @return the relevant data extracted from the reply
     */
    DetermineAuthTypeResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;
};
}
