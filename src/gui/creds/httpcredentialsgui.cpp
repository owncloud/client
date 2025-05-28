/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#include "creds/httpcredentialsgui.h"
#include "account.h"
#include "accountmodalwidget.h"
#include "application.h"
#include "common/asserts.h"
#include "creds/qmlcredentials.h"
#include "gui/accountsettings.h"
#include "networkjobs.h"
#include "settingsdialog.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QTimer>


namespace OCC {

Q_LOGGING_CATEGORY(lcHttpCredentialsGui, "sync.credentials.http.gui", QtInfoMsg)


// with oauth the account builder actually passes the token in place of the password.
HttpCredentialsGui::HttpCredentialsGui(const QString &davUser, const QString &password, const QString &refreshToken)
    : HttpCredentials(DetermineAuthTypeJob::AuthType::OAuth, davUser, password)
{
    _refreshToken = refreshToken;
}

void HttpCredentialsGui::askFromUser()
{
    // This function can be called from AccountState::slotInvalidCredentials,
    // which (indirectly, through HttpCredentials::invalidateToken) schedules
    // a cache wipe of the qnam. We can only execute a network job again once
    // the cache has been cleared, otherwise we'd interfere with the job.
    QTimer::singleShot(0, this, [this] {
        if (isUsingOAuth()) {
            startOAuth();
        } /*all of this should go once the authentication type is moved to the credentials */
        else {
            // todo: #18 or #19
            // First, we will check what kind of auth we need.
            auto job = new DetermineAuthTypeJob(_account->sharedFromThis(), this);
            QObject::connect(job, &DetermineAuthTypeJob::authType, this, [this](DetermineAuthTypeJob::AuthType type) {
                _authType = type;
                if (type == DetermineAuthTypeJob::AuthType::OAuth) {
                    startOAuth();
                } else {
                    qCWarning(lcHttpCredentialsGui) << "Bad http auth type:" << type;
                    Q_EMIT fetched();
                }
            });
            job->start();
        }
    });
}

void HttpCredentialsGui::asyncAuthResult(OAuth::Result r, const QString &token, const QString &refreshToken)
{
    _asyncAuth.reset();
    switch (r) {
    case OAuth::NotSupported:
        // also should not happen after initial setup?
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::ErrorInsecureUrl:
        // should not happen after the initial setup
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::Error:
        Q_EMIT oAuthErrorOccurred();
        return;
    case OAuth::LoggedIn:
        Q_EMIT oAuthLoginAccepted();
        break;
    }

    _password = token;
    _refreshToken = refreshToken;
    _ready = true;
    persist();
    Q_EMIT fetched();
}

void HttpCredentialsGui::startOAuth()
{
    qCDebug(lcHttpCredentialsGui) << "showing modal dialog asking user to log in again via OAuth2";
    if (_asyncAuth) {
        return;
    }
    if (_modalWidget) {
        _modalWidget->deleteLater();
    }
    _asyncAuth.reset(new AccountBasedOAuth(_account->sharedFromThis(), this));
    connect(_asyncAuth.data(), &OAuth::result, this, &HttpCredentialsGui::asyncAuthResult);

    auto *oauthCredentials = new QmlOAuthCredentials(_asyncAuth.data(), _account->url(), _account->davDisplayName());
    _modalWidget = new AccountModalWidget(tr("Login required"), QUrl(QStringLiteral("qrc:/qt/qml/org/ownCloud/gui/qml/credentials/OAuthCredentials.qml")),
        oauthCredentials, ocApp()->gui()->settingsDialog());

    connect(oauthCredentials, &QmlOAuthCredentials::logOutRequested, _modalWidget, [this] {
        _modalWidget->reject();
        _modalWidget.clear();
        _asyncAuth.reset();
        Q_EMIT requestLogout();
    });
    connect(oauthCredentials, &QmlOAuthCredentials::requestRestart, this, [this] {
        _modalWidget->reject();
        _modalWidget.clear();
        _asyncAuth.reset();
        startOAuth();
    });
    connect(this, &HttpCredentialsGui::oAuthLoginAccepted, _modalWidget, &AccountModalWidget::accept);
    connect(this, &HttpCredentialsGui::oAuthErrorOccurred, oauthCredentials, [this]() {
        Q_ASSERT(!ready());
        ownCloudGui::raise();
    });

    ocApp()->gui()->settingsDialog()->accountSettings(_account)->addModalWidget(_modalWidget);
    _asyncAuth->startAuthentication();
}

} // namespace OCC
