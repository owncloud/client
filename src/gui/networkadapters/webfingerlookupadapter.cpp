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

    const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
        result.error = tr("Failed to retrieve user info");
    }

    const auto data = reply->readAll();
    QJsonParseError error = {};
    const auto json = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        result.error = error.errorString();
    }

    const auto links = json.object().value(QStringLiteral("links")).toArray();

    for (const auto &link : links) {
        const auto linkObject = link.toObject();

        const QString rel = linkObject.value(QStringLiteral("rel")).toString();
        const QString href = linkObject.value(QStringLiteral("href")).toString();

        if (rel != QStringLiteral("http://webfinger.owncloud/rel/server-instance")) {
            continue;
        }

        result.urls.append(QUrl::fromUserInput(href));
    }
    if (result.urls.isEmpty())
        result.error = tr("WebFinger lookup returned no links");

    delete reply;
    return result;
}
}
