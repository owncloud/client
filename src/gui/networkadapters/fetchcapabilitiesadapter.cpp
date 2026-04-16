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

#include "fetchcapabilitiesadapter.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "abstractcorejob.h"
#include "creds/credentialssupport.h"

namespace OCC {

Q_LOGGING_CATEGORY(lcFetchCapabilitesAdapter, "gui.networkadapters.fetchcapabiltiesadapter");


FetchCapabilitiesAdapter::FetchCapabilitiesAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent)
    : QObject{parent}
    , _nam(nam)
    , _url(url)
    , _authorizationHeader(QStringLiteral("Bearer %1").arg(authToken))
{
}

FetchCapabilitiesResult FetchCapabilitiesAdapter::getResult()
{
    FetchCapabilitiesResult result;
    if (!_nam) {
        return result;
    }

    QUrlQuery urlQuery({{QStringLiteral("format"), QStringLiteral("json")}});

    QNetworkRequest req = AbstractCoreJobFactory::makeRequest(Utility::concatUrlPath(_url, QStringLiteral("ocs/v2.php/cloud/capabilities"), urlQuery));

    // next 4 stolen from webfingerlookupadapter
    // we are not connected yet, so we need to handle the authentication manually
    req.setRawHeader("Authorization", _authorizationHeader.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // we just added the Authorization header, don't let the credentials AccessManager tamper with it
    req.setAttribute(DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    QNetworkReply *reply = _nam->get(req);
    QEventLoop waitLoop;
    QObject::connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    const QString contentTypeHeader = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (!contentTypeHeader.contains(QStringLiteral("application/json"), Qt::CaseInsensitive)) {
        result.error = tr("server sent invalid content type: %1.").arg(contentTypeHeader);
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        result.error = tr("could not parse Capabilities from server.");
    }

    // make sure the reported subject matches the requested resource
    QJsonValue subjectValue = doc.object().value(QStringLiteral("subject"));
    if (subjectValue.isString()) {
        const QString subject = subjectValue.toString();
        if (subject != _url.toString()) {
            result.error = tr("reply sent for different subject (server): %1").arg(subject);
        }
    }
    QJsonObject capabilitiesValue =
        doc.object().value(QStringLiteral("ocs")).toObject().value(QStringLiteral("data")).toObject().value(QStringLiteral("capabilities")).toObject();

    result.capabilities = Capabilities(_url, capabilitiesValue.toVariantMap());

    delete reply;
    return result;
}
}
