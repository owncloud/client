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

struct UserInfoResult
{
    QString error;
    QString userId;
    QString displayName;

    bool success() const { return error.isEmpty() && !userId.isEmpty() && !displayName.isEmpty(); }
};

class UserInfoAdapter : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    UserInfoResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;
    QString _authorizationHeader;
};
}
