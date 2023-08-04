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
#include "application.h"
#include "basicloginwidget.h"
#include "common/asserts.h"
#include "loginrequireddialog.h"
#include "networkjobs.h"
#include "settingsdialog.h"
#include "theme.h"

#include <QBuffer>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkReply>
#include <QTimer>

namespace {
auto isOAuthC()
{
    return QStringLiteral("oauth");
}

const QString userC()
{
    return QStringLiteral("user");
}
}


namespace OCC {

Q_LOGGING_CATEGORY(lcHttpCredentialsGui, "sync.credentials.http.gui", QtInfoMsg)

HttpCredentialsGui::HttpCredentialsGui(Account *account)
    : HttpCredentials(account)
{
}

void HttpCredentialsGui::openBrowser()
{
    OC_ASSERT(isUsingOAuth());
    if (isUsingOAuth()) {
        if (_asyncAuth) {
            _asyncAuth->openBrowser();
        } else {
            qCWarning(lcHttpCredentialsGui) << "There is no authentication job running, did the previous attempt fail?";
            askFromUserAsync();
        }
    }
}

void HttpCredentialsGui::askFromUser()
{
    // This function can be called from AccountState::slotInvalidCredentials,
    // which (indirectly, through HttpCredentials::invalidateToken) schedules
    // a cache wipe of the qnam. We can only execute a network job again once
    // the cache has been cleared, otherwise we'd interfere with the job.
    QTimer::singleShot(0, this, &HttpCredentialsGui::askFromUserAsync);
}

void HttpCredentialsGui::askFromUserAsync()
{
    // TODO are we logged out
    //_account
    if (isUsingOAuth()) {
        restartOAuth();
    } else {
        // First, we will check what kind of auth we need.
        auto job = new DetermineAuthTypeJob(account()->sharedFromThis(), this);
        QObject::connect(job, &DetermineAuthTypeJob::authType, this, [this](DetermineAuthTypeJob::AuthType type) {
            _authType = type;
            if (type == DetermineAuthTypeJob::AuthType::OAuth) {
                restartOAuth();
            } else if (type == DetermineAuthTypeJob::AuthType::Basic) {
                showDialog();
            } else {
                qCWarning(lcHttpCredentialsGui) << "Bad http auth type:" << type;
                emit fetched();
            }
        });
        job->start();
    }
}

void HttpCredentialsGui::asyncAuthResult(OAuth::Result r, const QString &token, const QString &refreshToken)
{
    _asyncAuth.reset();
    switch (r) {
    case OAuth::NotSupported:
        showDialog();
        return;
    case OAuth::ErrorInsecureUrl:
        // should not happen after the initial setup
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::Error:
        Q_EMIT oAuthErrorOccurred();
        return;
    case OAuth::LoggedIn:
        break;
    }

    _password = token;
    _refreshToken = refreshToken;
    _ready = true;
    persist();
    emit fetched();
}

void HttpCredentialsGui::showDialog()
{
    if (_loginRequiredDialog != nullptr) {
        return;
    }

    auto *dialog = new LoginRequiredDialog(LoginRequiredDialog::Mode::Basic, ocApp()->gui()->settingsDialog());

    // make sure it's cleaned up since it's not owned by the account settings (also prevents memory leaks)
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    dialog->setTopLabelText(tr("Please enter your password to log in to the account %1.").arg(account()->displayName()));

    auto *contentWidget = qobject_cast<BasicLoginWidget *>(dialog->contentWidget());
    contentWidget->forceUsername(user());

    // in this case, we want to use the login button
    dialog->addLogInButton();

    connect(dialog, &LoginRequiredDialog::finished, ocApp()->gui()->settingsDialog(), [this, contentWidget](const int result) {
        if (result == QDialog::Accepted) {
            _password = contentWidget->password();
            _refreshToken.clear();
            _ready = true;
            persist();
        } else {
            Q_EMIT requestLogout();
        }
        emit fetched();
    });

    dialog->open();
    ownCloudGui::raiseDialog(dialog);

    QTimer::singleShot(0, contentWidget, [contentWidget]() {
        contentWidget->setFocus(Qt::OtherFocusReason);
    });

    _loginRequiredDialog = dialog;
}

QUrl HttpCredentialsGui::authorisationLink() const
{
    OC_ASSERT(isUsingOAuth());
    if (isUsingOAuth()) {
        if (_asyncAuth) {
            return _asyncAuth->authorisationLink();
        } else {
            qCWarning(lcHttpCredentialsGui) << "There is no authentication job running, did the previous attempt fail?";
            return {};
        }
    }
    return {};
}

void HttpCredentialsGui::restartOAuth()
{
    _asyncAuth.reset(new AccountBasedOAuth(account()->sharedFromThis(), this));
    connect(_asyncAuth.data(), &OAuth::result,
        this, &HttpCredentialsGui::asyncAuthResult);
    connect(_asyncAuth.data(), &OAuth::destroyed,
        this, &HttpCredentialsGui::authorisationLinkChanged);
    _asyncAuth->startAuthentication();
    emit authorisationLinkChanged();
}

HttpCredentialsGui::HttpCredentialsGui(AccountState *accountState, const QString &loginUser, const QString &password)
    : HttpCredentials(accountState->account().get(), DetermineAuthTypeJob::AuthType::Basic, loginUser, password)
{
}

HttpCredentialsGui::HttpCredentialsGui(AccountState *accountState, const QString &davUser, const QString &password, const QString &refreshToken)
    : HttpCredentials(accountState->account().get(), DetermineAuthTypeJob::AuthType::OAuth, davUser, password)
{
    _refreshToken = refreshToken;
}

HttpCredentialsGui *HttpCredentialsGui::fromSettings(AccountState *accountState)
{
    auto *out = new HttpCredentialsGui(accountState->account().get());
    out->_user = accountState->account()->credentialSetting(out, userC()).toString();
    Q_ASSERT(!out->_user.isEmpty());

    const auto isOauth = accountState->account()->credentialSetting(out, isOAuthC()).toBool();
    out->_authType = isOauth ? DetermineAuthTypeJob::AuthType::OAuth : DetermineAuthTypeJob::AuthType::Basic;

    out->fetchFromKeychain();
    return out;
}
void HttpCredentialsGui::persist()
{
    account()->setCredentialSetting(this, userC(), _user);
    account()->setCredentialSetting(this, isOAuthC(), isUsingOAuth());
    HttpCredentials::persist();
}
} // namespace OCC
