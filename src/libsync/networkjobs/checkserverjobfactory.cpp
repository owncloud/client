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

#include "checkserverjobfactory.h"

#include "common/utility.h"
#include "libsync/accessmanager.h"
#include "libsync/account.h"
#include "libsync/cookiejar.h"
#include "libsync/creds/abstractcredentials.h"
#include "libsync/theme.h"

#include <QJsonParseError>

namespace {

// FIXME: this is not a permanent solution, eventually we want to replace the job factories with job classes so we can store such information there
class CheckServerCoreJob : OCC::CoreJob
{
    Q_OBJECT
    friend OCC::CheckServerJobFactory;

public:
    using OCC::CoreJob::CoreJob;

};

}

namespace OCC {

Q_LOGGING_CATEGORY(lcCheckServerJob, "sync.checkserverjob", QtInfoMsg)

CheckServerJobResult::CheckServerJobResult() = default;

CheckServerJobResult::CheckServerJobResult(const QJsonObject &statusObject, const QUrl &serverUrl)
    : _statusObject(statusObject)
    , _serverUrl(serverUrl)
{
}

QJsonObject CheckServerJobResult::statusObject() const
{
    return _statusObject;
}

QUrl CheckServerJobResult::serverUrl() const
{
    return _serverUrl;
}

// thinking out loud here:
// it may be we can turn this into a job adapter.
// consider:
// adapter::createFromAccount returns the adapter
// adapter::startJob -> returns nothing!
// could signal out "finished" with a copy of the result?
// once "finished" it could delete the job, woopie do!
// caller could keep an adapter around as member for re-use if needed, or hit and quit.
CheckServerJobFactory CheckServerJobFactory::createFromAccount(const Account *account, bool clearCookies)
{
    // this is horrid but we can't return an "empty" factory so just crash.
    // TODO: the jobs need a lot of work.
    Q_ASSERT(account);

    // in order to receive all ssl erorrs we need a fresh QNam
    // see startJob where we parent the nam to the job itself to ensure it is cleaned up asap
    auto nam = account->credentials()->createAccessManager();
    nam->setCustomTrustedCaCertificates(account->approvedCerts());
    // do we start with the old cookies or new
    if (!clearCookies) {
        const auto newJar = account->accessManager()->ownCloudCookieJar()->clone();
        nam->setCookieJar(newJar);
    }
    return CheckServerJobFactory(nam);
}

CoreJob *CheckServerJobFactory::startJob(const QUrl &url, QObject *parent)
{
    // the custom job class is used to store some state we need to maintain until the job has finished

    auto req = makeRequest(Utility::concatUrlPath(url, QStringLiteral("status.php")));

    // We never want to follow any redirects. At least until proven otherwise.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader(QByteArrayLiteral("OC-Connection-Validator"), QByteArrayLiteral("desktop"));
    req.setMaximumRedirectsAllowed(0);

    auto job = new CheckServerCoreJob(_nam->get(req), parent);
    // this looks shady, but for the check server jobs we create a new nam on every run. The old approach was to parent it to the value
    // passed to the factory but in fact that was often the connection validator which lives for a really long time, and hence is not the
    // best parent for that temp nam.
    // setting the job as parent is a more reasonable choice but this is not a great place for it -
    _nam->setParent(job);

    // make this handle maintenance mode - only if necessary
    /*  QObject::connect(job->reply(), &QNetworkReply::redirected, job, [job] {
          //  const auto code = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
          // if (code == 302 || code == 307) {
          // job->_redirectDistinct = false;
          //}
      });*/

    QObject::connect(job->reply(), &QNetworkReply::finished, job, [url, job] {

        const QUrl targetUrl = job->reply()->url().adjusted(QUrl::RemoveFilename);

        // TODO: still needed?
        if (targetUrl.scheme() == QLatin1String("https")
            && job->reply()->sslConfiguration().sessionTicket().isEmpty()
            && job->reply()->error() == QNetworkReply::NoError) {
            qCWarning(lcCheckServerJob) << "No SSL session identifier / session ticket is used, this might impact sync performance negatively.";
        }

        // I'm not sure this can even happen when using ManualRedirectPolicy
        if (!Utility::urlEqual(url, targetUrl)) {
            qCWarning(lcCheckServerJob) << "We got a redirect from " << url << " to " << targetUrl << ", aborting";
            setJobError(job, QStringLiteral("Redirect from server"));
        }

        const int httpStatus = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (job->reply()->error() == QNetworkReply::TooManyRedirectsError) {
            qCWarning(lcCheckServerJob) << "error:" << job->reply()->errorString();
            setJobError(job, job->reply()->errorString());
        } else if (httpStatus != 200 || job->reply()->bytesAvailable() == 0) {
            qCWarning(lcCheckServerJob) << "error: status.php replied" << httpStatus;
            setJobError(job, QStringLiteral("Invalid HTTP status code received for status.php: %1").arg(httpStatus));
        } else {
            const QByteArray body = job->reply()->peek(4 * 1024);
            QJsonParseError error;
            auto status = QJsonDocument::fromJson(body, &error);
            // empty or invalid response
            if (error.error != QJsonParseError::NoError || status.isNull()) {
                qCWarning(lcCheckServerJob) << "status.php from server is not valid JSON!" << body << job->reply()->request().url() << error.errorString();
            }

            qCInfo(lcCheckServerJob) << "status.php returns: " << status << " " << job->reply()->error() << " Reply: " << job->reply();

            if (status.object().contains(QStringLiteral("installed"))) {
                CheckServerJobResult result(status.object(), url);
                setJobResult(job, QVariant::fromValue(result));
            } else {
                qCWarning(lcCheckServerJob) << "No proper answer on " << job->reply()->url();
                setJobError(job, QStringLiteral("Did not receive expected reply from server"));
            }
        }
    });

    return job;
}

} // OCC

#include "checkserverjobfactory.moc"
