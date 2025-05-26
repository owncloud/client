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

#include "determineauthtypeadapter.h"

#include "abstractcorejob.h"
#include "common/utility.h"
#include "creds/httpcredentials.h"
#include "theme.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace OCC {

Q_LOGGING_CATEGORY(lcDetermineAuthTypeAdapter, "gui.networkadapters.determineauthtypeadapter");

DetermineAuthTypeAdapter::DetermineAuthTypeAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent)
    : QObject(parent)
    , _nam(nam)
    , _url(url)
{
}

DetermineAuthTypeResult DetermineAuthTypeAdapter::getResult()
{
    DetermineAuthTypeResult res;
    if (!_nam)
        return res;

    // we explicitly use a legacy dav path here
    QNetworkRequest req = AbstractCoreJobFactory::makeRequest(Utility::concatUrlPath(_url, Theme::instance()->webDavPath()));

    req.setAttribute(HttpCredentials::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    QEventLoop waitLoop;
    QNetworkReply *reply = _nam->sendCustomRequest(req, "PROPFIND");
    connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
        // the header key is case insensitive per debug testing
        const QByteArray rawHeader = reply->rawHeader(QByteArrayLiteral("www-authenticate")).toLower();

        if (rawHeader.contains(QByteArrayLiteral("bearer "))) {
            res.type = AuthenticationType::OAuth;
        } else {
            // todo: #18 - if not OAuth this should most likely be Unknown
            res.type = AuthenticationType::Basic;
            if (rawHeader.isEmpty()) {
                qCWarning(lcDetermineAuthTypeAdapter) << "Did not receive WWW-Authenticate reply to auth-test PROPFIND";
            }
        }
    } else if (reply->error() == QNetworkReply::NoError) {
        res.error = tr("Server did not ask for authorization");
    } else {
        res.error = tr("Failed to determine auth type: %1").arg(reply->errorString());
    }

    delete reply;
    return res;
}
}
