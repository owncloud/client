/*
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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

#include "discoverwebfingerserviceadapter.h"

#include "abstractcorejob.h"
#include "common/utility.h"

#include <QEventLoop>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace OCC {

Q_LOGGING_CATEGORY(lcDiscoverWebFingerServiceAdapter, "gui.networkadapters.discoverwebfingerserviceadapter");

DiscoverWebFingerServiceAdapter::DiscoverWebFingerServiceAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent)
    : QObject(parent)
    , _nam(nam)
    , _url(url)
{
}

DiscoverWebFingerServiceResult DiscoverWebFingerServiceAdapter::getResult()
{
    DiscoverWebFingerServiceResult result;

    if (!_nam) {
        return result;
    }

    // this first request needs to be done without any authentication, since our goal is to find a server to authenticate to before the actual (authenticated)
    // WebFinger request
    QNetworkRequest req = AbstractCoreJobFactory::makeRequest(
        Utility::concatUrlPath(_url, QStringLiteral("/.well-known/webfinger"), {{QStringLiteral("resource"), _url.toString()}}));

    QNetworkReply *reply = _nam->get(req);

    QEventLoop waitLoop;
    QObject::connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    result = processReply(reply);

    delete reply;
    return result;
}

/* for future async version of the adapter we can do something like:

DiscoverWebfingerServiceAdapter::runAsync() {
QNetworkRequest req = ....

QNetworkReply *reply = _nam->get(req);
connect(reply, reply::finished, this, [this, reply] () {
emit done(processReply(reply));
reply->deleteLater
}

In theory we could call getResult using QtConcurrent but I think this would add more overhead than it's worth for just a single operation.
*/

DiscoverWebFingerServiceResult DiscoverWebFingerServiceAdapter::processReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        return formatError(reply->errorString());
    }

    const QString contentTypeHeader = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (!contentTypeHeader.contains(QStringLiteral("application/json"), Qt::CaseInsensitive)) {
        return formatError(QStringLiteral("server sent invalid content type: %1").arg(contentTypeHeader));
    }

    // next up, parse JSON
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &error);
    // empty or invalid response
    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        return formatError(QStringLiteral("could not parse JSON response from server."));
    }

    // make sure the reported subject matches the requested resource
    QJsonValue value = doc.object().value(QStringLiteral("subject"));
    if (value.isString()) {
        const QString subject = value.toString();
        if (subject != _url.toString()) {
            return formatError(QStringLiteral("reply sent for different subject (server): %1").arg(subject));
        }
    }

    // check for an OIDC issuer in the list of links provided (we use the first that matches our conditions)
    const auto links = doc.object().value(QStringLiteral("links")).toArray();
    for (const auto &link : links) {
        const auto linkObject = link.toObject();

        if (linkObject.value(QStringLiteral("rel")).toString() == QStringLiteral("http://openid.net/specs/connect/1.0/issuer")) {
            // we have good faith in the server to provide a meaningful value and do not have to validate this any further
            DiscoverWebFingerServiceResult result;
            result.href = linkObject.value(QStringLiteral("href")).toString();
            return result;
        }
    }

    return formatError(QStringLiteral("could not find suitable relation in WebFinger response"));
}

DiscoverWebFingerServiceResult DiscoverWebFingerServiceAdapter::formatError(const QString &errorDetail)
{
    DiscoverWebFingerServiceResult res;
    res.error = _defaultError;
    qCWarning(lcDiscoverWebFingerServiceAdapter) << errorDetail;
    return res;
}
}
