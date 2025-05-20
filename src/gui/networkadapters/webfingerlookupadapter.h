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
#include <QUrl>


class QNetworkAccessManager;

namespace OCC {

/**
 * @brief The WebFingerLookupResult struct is a simple container for collecting the result of the WebFingerLookup
 *
 * error contains the error string if the request failed
 * urls contains the webfinger links from the reply if the request succeeded
 *
 */
struct WebFingerLookupResult
{
    QString error;
    QList<QUrl> urls;

    bool success() const { return error.isEmpty() && !urls.isEmpty(); }
};

class WebFingerLookupAdapter : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief WebFingerLookupAdapter
     * @param nam the network manager
     * @param authToken the token to use with the request
     * @param url the url for the request
     * @param parent is used for the standard QObject parenting scheme. This param should not be needed in most cases as the intent is
     * to create the adapter on the stack then just let it go out of scope, no parenting required.
     */
    explicit WebFingerLookupAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult runs the network request synchronously and is intended for use in a gui, where it makes sense to block until we
     * have the required data to move on.
     * @return the result of the network request
     *
     * it is safe to call get result repeatedly, so long as the parameters passed the the adapter on construction are still valid.
     */
    WebFingerLookupResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;

    QString _authorizationHeader;
};
}
