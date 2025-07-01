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
#include "userinfoadapter.h"

#include "abstractcorejob.h"
#include "common/utility.h"
#include "creds/credentialssupport.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace OCC {

Q_LOGGING_CATEGORY(lcUserInfoAdapter, "gui.networkadapters.userinfoadapter")

UserInfoAdapter::UserInfoAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent)
    : QObject(parent)
    , _nam(nam)
    , _url(url)
    , _authorizationHeader(QStringLiteral("Bearer %1").arg(authToken))
{
}

UserInfoResult UserInfoAdapter::getResult()
{
    UserInfoResult result;
    if (!_nam) {
        return result;
    }

    QUrlQuery urlQuery({{QStringLiteral("format"), QStringLiteral("json")}});

    auto req = AbstractCoreJobFactory::makeRequest(Utility::concatUrlPath(_url, QStringLiteral("ocs/v2.php/cloud/user"), urlQuery));

    req.setRawHeader("Authorization", _authorizationHeader.toUtf8());
    req.setAttribute(DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    QEventLoop waitLoop;
    QNetworkReply *reply = _nam->get(req);
    connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    const auto data = reply->readAll();
    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
        result.error = tr("Failed to retrieve user info");
    } else {
        QJsonParseError error = {};
        const auto json = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError) {
            const auto jsonData = json.object().value(QStringLiteral("ocs")).toObject().value(QStringLiteral("data")).toObject();
            result.userId = jsonData.value(QStringLiteral("id")).toString();
            result.displayName = jsonData.value(QStringLiteral("display-name")).toString();
        } else {
            result.error = error.errorString();
        }
    }

    return result;
}
}
