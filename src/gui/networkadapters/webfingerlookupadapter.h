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

/**
 * @brief The WebFingerLookupAdapter class is used to retrieve the URLs associated with a webfinger provider.
 * Prior authentication is required.
 * If the result is successful, we take the first entry from the URL list as the de facto url for the account.
 */

class WebFingerLookupAdapter : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief WebFingerLookupAdapter
     * @param nam the network manager
     * @param authToken the token to use with the request
     * @param url for the request. Note this value should be the original server url, not the webfinger url (if it exists). I still do
     * not fully understand why that is but will find out.
     * @param parent is used for the standard QObject parenting scheme. This param should not be needed in most cases as the intent is
     * to create the adapter on the stack then just let it go out of scope, no parenting required.
     */
    explicit WebFingerLookupAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult runs the network request synchronously and is intended for use in a gui, where it makes sense to block until we
     * have the required data to move on.
     * @return the result of the network request. If successful the result contains a list of webfinger links.
     *
     * it is safe to call get result repeatedly, so long as the parameters passed to the adapter on construction are still valid.
     */
    WebFingerLookupResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;

    QString _authorizationHeader;
};
}
