/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "fetchuserinfojobfactory.h"
#include "common/utility.h"
#include "creds/credentialssupport.h"

#include <QApplication>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QStringLiteral>

Q_LOGGING_CATEGORY(lcFetchUserInfoJob, "sync.networkjob.fetchuserinfojob", QtInfoMsg);

namespace OCC {

FetchUserInfoJobFactory FetchUserInfoJobFactory::fromOAuth2Credentials(QNetworkAccessManager *nam, const QString &bearerToken)
{
    QString authorizationHeader = QStringLiteral("Bearer %1").arg(bearerToken);
    return { nam, authorizationHeader };
}

FetchUserInfoJobFactory::FetchUserInfoJobFactory(QNetworkAccessManager *nam, const QString &authHeaderValue)
    : AbstractCoreJobFactory(nam)
    , _authorizationHeader(authHeaderValue)
{
}

CoreJob *FetchUserInfoJobFactory::startJob(const QUrl &url, QObject *parent)
{
    QUrlQuery urlQuery({ { QStringLiteral("format"), QStringLiteral("json") } });

    auto req = makeRequest(Utility::concatUrlPath(url, QStringLiteral("ocs/v2.php/cloud/user"), urlQuery));

    // We are not connected yet so we need to handle the authentication manually
    req.setRawHeader("Authorization", _authorizationHeader.toUtf8());
    // todo: #20 - apparently this is oc10 related - there are many instances but they should go.
    req.setRawHeader(QByteArrayLiteral("OCS-APIREQUEST"), QByteArrayLiteral("true"));

    // We just added the Authorization header, don't let the credentials AccessManager tamper with it
    req.setAttribute(OCC::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    auto *job = new CoreJob(_nam->get(req), parent);

    QObject::connect(job->reply(), &QNetworkReply::finished, job, [job] {
        const auto data = job->reply()->readAll();
        const auto statusCode = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (job->reply()->error() != QNetworkReply::NoError || statusCode != 200) {
            setJobError(job, QApplication::translate("FetchUserInfoJobFactory", "Failed to retrieve user info"));
        } else {
            qCDebug(lcFetchUserInfoJob) << data;

            QJsonParseError error = {};
            const auto json = QJsonDocument::fromJson(data, &error);

            if (error.error == QJsonParseError::NoError) {
                const auto jsonData = json.object().value(QStringLiteral("ocs")).toObject().value(QStringLiteral("data")).toObject();

                FetchUserInfoResult result(jsonData.value(QStringLiteral("id")).toString(), jsonData.value(QStringLiteral("display-name")).toString());

                setJobResult(job, QVariant::fromValue(result));
            } else {
                setJobError(job, error.errorString());
            }
        }
    });

    return job;
}

} // OCC
