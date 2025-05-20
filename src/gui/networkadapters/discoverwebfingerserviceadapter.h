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
 *  This adapter allows the caller to find a webfinger service synchronously, which is very useful when called from a gui
 *  because we can't continue until the required values are known.
 *
 *  In future we may also extend the adapter to run in asycn mode as well, but for now it's not needed.
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
