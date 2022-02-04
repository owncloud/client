#include "determineauthtypejob2.h"

#include "common/utility.h"
#include "creds/httpcredentials.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcDetermineAuthTypeJob, "sync.networkjob.determineauthtype2", QtInfoMsg);

using namespace OCC;

DetermineAuthTypeJobFactory::DetermineAuthTypeJobFactory(QNetworkAccessManager *nam, QObject *parent)
    : AbstractCoreJobFactory(nam, parent)
{
}

DetermineAuthTypeJobFactory::~DetermineAuthTypeJobFactory() = default;

Job *DetermineAuthTypeJobFactory::startJob(const QUrl &url)
{
    auto job = new Job;

    QNetworkRequest req(Utility::concatUrlPath(url, QStringLiteral("remote.php/dav/files/")));
    req.setAttribute(HttpCredentials::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    auto *reply = nam()->sendCustomRequest(req, "PROPFIND");

    connect(reply, &QNetworkReply::finished, job, [reply, job] {
        reply->deleteLater();

        const auto authChallenge = reply->rawHeader(QByteArrayLiteral("WWW-Authenticate")).toLower();

        // we fall back to basic in any case
        if (authChallenge.contains(QByteArrayLiteral("bearer "))) {
            finishJobWithSuccess(job, qVariantFromValue(AuthType::OAuth));
        } else {
            if (authChallenge.isEmpty()) {
                qCWarning(lcDetermineAuthTypeJob) << "Did not receive WWW-Authenticate reply to auth-test PROPFIND";
            }

            finishJobWithSuccess(job, qVariantFromValue(AuthType::Basic));
        }

        qCInfo(lcDetermineAuthTypeJob) << "Auth type for" << reply->url() << "is" << job->result();
    });

    return job;
}
