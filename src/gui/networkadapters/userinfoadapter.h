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

namespace OCC {

/**
 * @brief The UserInfoResult class is a simple container which holds the user info
 * userId is the username, davUser, etc
 * displayName is the "user friendly" username
 * error is exactly that - an error message from the adapter when it fails.
 */
struct UserInfoResult
{
    QString userId;
    QString displayName;
    QString error;

    bool success() const { return error.isEmpty() && !userId.isEmpty() && !displayName.isEmpty(); }
};

/**
 * @brief The UserInfoAdapter class is a simple implementation that retrieves the user info in a synchronous manner.
 */
class UserInfoAdapter : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief UserInfoAdapter configures the adapter with the necessary data
     * @param nam the QNetworkAccessManager this adapter should use to retrieve the info
     * @param authToken the authentication token for this user
     * @param url the url to use when retrieving the info. In practice this will either be the original url (when webfinger is not in play)
     * or it will be the webfinger user endpoint that is discovered using the WebfingerLookupAdapter
     * @param parent this class will normally be created on the stack so no parent is needed, but if you want to run it on the heap, give it
     * give it a sensible parent Qt style.
     */
    explicit UserInfoAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    /**
     * @brief getResult runs the network request/reply synchronously
     * @return the results from the reply, cleanly bundled in the result struct
     */
    UserInfoResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;
    QString _authorizationHeader;
};
}
