#include "webfingerlookupadapter.h"

#include "abstractcorejob.h"
#include "common/utility.h"
#include "creds/httpcredentials.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

#include <QApplication>
#include <QStringLiteral>

namespace OCC {

WebFingerLookupAdapter::WebFingerLookupAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent)
    : QObject{parent}
    , _nam(nam)
    , _url(url)
    , _authorizationHeader(QStringLiteral("Bearer %1").arg(authToken))
{
}

// AFAIK we can run this function using QtConcurrent if we want avoid blocking
WebFingerLookupResult WebFingerLookupAdapter::getResult()
{
    const QString resource = QStringLiteral("acct:me@%1").arg(_url.host());

    auto req =
        AbstractCoreJobFactory::makeRequest(Utility::concatUrlPath(_url, QStringLiteral("/.well-known/webfinger"), {{QStringLiteral("resource"), resource}}));

    // we are not connected yet, so we need to handle the authentication manually
    req.setRawHeader("Authorization", _authorizationHeader.toUtf8());

    // we just added the Authorization header, don't let HttpCredentialsAccessManager tamper with it
    req.setAttribute(OCC::HttpCredentials::DontAddCredentialsAttribute, true);
    req.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);

    QEventLoop waitLoop;
    QNetworkReply *reply = _nam->get(req);
    //_job = _factory.startJob(_url, this);
    connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    Q_ASSERT(reply->isFinished());
    WebFingerLookupResult result;
    // OCC::Result<QVector<QUrl>, QString> result{};
    const auto data = reply->readAll();
    // qCDebug(lcWebFingerUserInfoJob) << data;
    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
        result.error = QApplication::translate("WebFingerLookupAdapter", "Failed to retrieve user info");
    }

    QJsonParseError error = {};
    const auto json = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        result.error = error.errorString();
    }

    //   qCDebug(lcWebFingerUserInfoJob) << "retrieved instances list for user" << json.object().value(QStringLiteral("subject")).toString();

    const auto links = json.object().value(QStringLiteral("links")).toArray();

    //  qCDebug(lcWebFingerUserInfoJob) << "found links:" << links;

    for (const auto &link : links) {
        const auto linkObject = link.toObject();

        const QString rel = linkObject.value(QStringLiteral("rel")).toString();
        const QString href = linkObject.value(QStringLiteral("href")).toString();

        if (rel != QStringLiteral("http://webfinger.owncloud/rel/server-instance")) {
            // qCDebug(lcWebFingerUserInfoJob) << "skipping invalid link" << href << "with rel" << rel;
            continue;
        }

        result.urls.append(QUrl::fromUserInput(href));
    }
    if (result.urls.isEmpty())
        result.error = QApplication::translate("WebFingerLookupAdapter", "WebFinger lookup returned no links");

    delete reply;
    return result;
}
}
