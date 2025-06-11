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

#include "resolveurladapter.h"

#include "abstractcorejob.h"
#include "common/utility.h"
#include "gui/application.h"
#include "gui/owncloudgui.h"
#include "gui/settingsdialog.h"
#include "gui/tlserrordialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>


namespace OCC {

Q_LOGGING_CATEGORY(lcResolveUrlAdapter, "gui.networkadapters.resolveurladapter")

ResolveUrlAdapter::ResolveUrlAdapter(QNetworkAccessManager *nam, const QUrl &url, QObject *parent)
    : QObject(parent)
    , _nam(nam)
    , _url(url)
{
}

ResolveUrlResult ResolveUrlAdapter::getResult()
{
    ResolveUrlResult result;

    if (!_nam) {
        return result;
    }

    QNetworkRequest req = AbstractCoreJobFactory::makeRequest(Utility::concatUrlPath(_url, QStringLiteral("status.php")));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    QEventLoop waitLoop;
    QNetworkReply *reply = _nam->get(req);
    connect(reply, &QNetworkReply::sslErrors, this, &ResolveUrlAdapter::handleSslErrors);
    connect(reply, &QNetworkReply::finished, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();

    if (!_sslErrors.isEmpty()) {
        result.error = tr("SSL failure when connecting to server at %1").arg(_url.toDisplayString());
        QString allErrors = _sslErrors.join(QStringLiteral("\n"));
        qCritical(lcResolveUrlAdapter) << QStringLiteral("Attempt to resolve Url: %1 failed with SSL errors: %2.").arg(_url.toDisplayString(), allErrors);
    } else if (reply->error() != QNetworkReply::NoError) {
        result.error = tr("Could not detect compatible server at %1").arg(_url.toDisplayString());
        qCritical(lcResolveUrlAdapter) << QStringLiteral("Network error when resolving Url %1: %2.").arg(_url.toDisplayString(), reply->errorString());
    } else {
        auto newUrl = reply->url();
        if (!newUrl.isValid()) {
            result.error = tr("Resolved url is invalid %1").arg(newUrl.toDisplayString());
        } else if (!newUrl.matches(req.url(), QUrl::RemoveFilename)) {
            qCWarning(lcResolveUrlAdapter) << _url << " redirect to " << newUrl << " is rejected";
            result.error = tr("Rejected redirect from %1 to %2").arg(_url.toDisplayString(), newUrl.toDisplayString());
        } else {
            result.resolvedUrl = newUrl.adjusted(QUrl::RemoveFilename);
        }

        if (!_certificates.isEmpty()) {
            result.acceptedCertificates = _certificates;
        }
    }
    delete reply;
    return result;
}

void ResolveUrlAdapter::handleSslErrors(const QList<QSslError> &errors)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply);
    // the tls error dialog can only handle untrusted certificates not general ssl errors
    auto filtered = errors;
    filtered.erase(std::remove_if(filtered.begin(), filtered.end(), [](const QSslError &e) { return e.certificate().isNull(); }), filtered.end());
    if (filtered.isEmpty()) {
        for (const auto &error : errors) {
            _sslErrors.append(error.errorString());
        }
    } else {
        auto *tlsErrorDialog = new TlsErrorDialog(filtered, reply->url().host(), ocApp()->gui()->settingsDialog());

        ownCloudGui::raise();
        // we have to exec here or the request finishes too fast
        int res = tlsErrorDialog->exec();
        if (res == QDialog::DialogCode::Accepted) {
            for (const auto &err : std::as_const(filtered))
                _certificates.insert(err.certificate());
            // let the reply finish!
            reply->ignoreSslErrors();
        } else {
            // it appears there is no need to abort the current reply when there are ssl errors - it emits finished even though there were un-ignored errors
            // I'm not sure we can use the reply after finished is received in this case, though! It should still exist, but the docs suggest it is "torn down"
            // (not sure what that means exactly) if errors are not ignored.
            _sslErrors.append(QStringLiteral("User rejected invalid SSL certificate"));
        }
    }
}
}
