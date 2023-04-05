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

#include "discoverwebfingerservicejobfactory.h"

#include "accessmanager.h"
#include "common/utility.h"
#include "creds/httpcredentials.h"
#include "gui/tlserrordialog.h"
#include "gui/updateurldialog.h"
#include "theme.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonParseError>
#include <QNetworkReply>

namespace OCC::Wizard::Jobs {

CoreJob *DiscoverWebFingerServiceJobFactory::startJob(const QUrl &url, QObject *parent)
{
    // this first request needs to be done without any authentication, since our goal is to find a server to authenticate to before the actual (authenticated)
    // WebFinger request
    auto req = makeRequest(Utility::concatUrlPath(url, QStringLiteral("/.well-known/webfinger"), {{QStringLiteral("resource"), url.toString()}}));

    auto *job = new CoreJob(nam()->get(req), parent);

    QObject::connect(job->reply(), &QNetworkReply::finished, job, [job, url]() {
        switch (job->reply()->error()) {
        case QNetworkReply::NoError:
            // all good, perform additional checks below
            break;
        default:
            setJobError(job, QApplication::translate("DiscoverWebFingerServiceJobFactory", "Invalid reply received from server"));
            return;
        }

        if (!job->reply()->header(QNetworkRequest::ContentTypeHeader).toString().toLower().contains(QStringLiteral("application/json"))) {
            setJobError(job, QApplication::translate("DiscoverWebFingerServiceJobFactory", "Invalid reply received from server"));
            return;
        }

        // next up, parse JSON
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(job->reply()->readAll(), &error);
        // empty or invalid response
        if (error.error != QJsonParseError::NoError || doc.isNull()) {
            setJobError(job, QApplication::translate("DiscoverWebFingerServiceJobFactory", "Invalid reply received from server"));
            return;
        }

        // make sure the reported subject matches the requested resource
        const auto subject = doc.object().value(QStringLiteral("subject"));
        if (subject != url.toString()) {
            setJobError(job, QApplication::translate("DiscoverWebFingerServiceJobFactory", "Invalid reply received from server"));
            return;
        }

        // check for an OIDC issuer in the list of links provided (we use the first that matches our conditions)
        const auto links = doc.object().value(QStringLiteral("links")).toArray();
        for (const auto &link : links) {
            const auto linkObject = link.toObject();

            if (linkObject.value(QStringLiteral("rel")).toString() == QStringLiteral("http://openid.net/specs/connect/1.0/issuer")) {
                // we have good faith in the server to provide a meaningful value and do not have to validate this any further
                const auto href = linkObject.value(QStringLiteral("href")).toString();
                setJobResult(job, href);
                return;
            }
        }

        setJobError(job, QApplication::translate("DiscoverWebFingerServiceJobFactory", "Invalid reply received from server"));
    });

    return job;
}

DiscoverWebFingerServiceJobFactory::DiscoverWebFingerServiceJobFactory(QNetworkAccessManager *nam)
    : AbstractCoreJobFactory(nam)
{
}

}
