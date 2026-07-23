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

class QNetworkAccessManager;
class QNetworkReply;

namespace OCC {

/*
 * simple struct that collects the result data into a simple bundle.
 *
 * error contains an error message if something went wrong
 * href is the first link found in the response which matches "http://openid.net/specs/connect/1.0/issuer"
 */
struct DiscoverWebFingerServiceResult
{
    QString error;
    QString href;

    bool success() const { return error.isEmpty() && !href.isEmpty(); }
};

/**
 *  This adapter allows the caller to find the href of a webfinger service associated with a given URL. No authentication is
 *  required for this operation, we are merely checking to see if webfinger is available, and if so, we collect the href
 *  for future use.
 *
 *  The request is run synchronously, which is very useful when called from a gui because we can't continue until the required values are known.
 *
 *  In future we may also extend the adapter to run in async mode as well, but for now it's not needed.
 *
 *  A key benefit to using this impl is that the adapter can simply be allocated on the stack so there are no
 *  memory management concerns. It also simplifies and improves readability in the caller.
 *
 */
class DiscoverWebFingerServiceAdapter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief DiscoverWebFingerServiceAdapter
     * @param nam - an instance of a network access manager to host the request
     * @param url - the url for the request
     * @param parent - typical QObject parenting scheme. A parent is rarely needed for this class because the typical use is to
     *  allocate a local instance on the stack, call getResult, then let the instance naturally go out of scope.
     */
    DiscoverWebFingerServiceAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult runs the QNetworkRequest and handles the reply internally.
     * @return the result instance with either an error or the successfully retrieved href to the webfinger service
     *
     * It is safe to call getResult repeatedly on the same adapter instance.
     *
     * Important: this function blocks the event loop to provide a simple, synchronous method of getting the href. For use in a gui that needs
     * to collect pieces of data one at a time it is of no concern, but this outside of a gui the blocking behavior could be problematic.
     */
    DiscoverWebFingerServiceResult getResult();

private:
    DiscoverWebFingerServiceResult processReply(QNetworkReply *reply);
    DiscoverWebFingerServiceResult formatError(const QString &errorDetail);

    QNetworkAccessManager *_nam;
    QUrl _url;

    const QString _defaultError = tr("Invalid reply received from server.");
};
}
