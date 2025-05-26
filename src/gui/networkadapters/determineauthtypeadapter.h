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

enum class AuthenticationType { Unknown = 0, Basic, OAuth };


struct DetermineAuthTypeResult
{
    QString error;
    AuthenticationType type = AuthenticationType::Unknown;
    bool success() const { return error.isEmpty(); }
};

/**
 * @brief The DetermineAuthTypeAdapter class allows us to determine the authentication type for the server using a synchronous call.
 * This is intended for use in the gui only.
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
