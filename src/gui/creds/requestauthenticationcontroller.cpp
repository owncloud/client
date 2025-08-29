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
#include "requestauthenticationcontroller.h"

#include "requestauthenticationwidget.h"

namespace OCC {

/**
 * @brief RequestAuthenticationController::RequestAuthenticationController
 * @param widget - may be null, for example when using the "log in" function after user explicitly logged out
 * @param parent - the usual qt parenting scheme
 */
RequestAuthenticationController::RequestAuthenticationController(RequestAuthenticationWidget *widget, QObject *parent)
    : QObject{parent}
    , _widget(widget)
{
}

void RequestAuthenticationController::handleSignIn()
{
    _oauth->openBrowser();
}

void RequestAuthenticationController::handleLogOut()
{
    // this ultimately needs to be chained to creds::logoutRequested signal
    Q_EMIT requestLogout();
}

void RequestAuthenticationController::startAuthentication(const AccountPtr account)
{
    Q_ASSERT(account);
    _oauth = new AccountBasedOAuth(account, this);
    connect(_oauth, &OAuth::authorisationLinkChanged, this, &RequestAuthenticationController::authUrlReady);
    connect(_oauth, &OAuth::result, this, &RequestAuthenticationController::handleOAuthResult);
    if (_widget) {
        connect(_widget, &RequestAuthenticationWidget::connectClicked, this, &RequestAuthenticationController::handleSignIn);
        connect(_widget, &RequestAuthenticationWidget::stayLoggedOutClicked, this, &RequestAuthenticationController::handleLogOut);
    }
    _oauth->startAuthentication();
}

void RequestAuthenticationController::authUrlReady()
{
    if (_widget) {
        _widget->setAuthUrl(_oauth->authorisationLink().toString(QUrl::FullyEncoded));
    } else {
        // we're just running with it
        _oauth->openBrowser();
    }
}

void RequestAuthenticationController::handleOAuthResult(OAuth::Result result, const QString &accessToken, const QString &refreshToken)
{
    QString errString;

    switch (result) {
    case OAuth::NotSupported:
        // also should not happen after initial setup?
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::ErrorInsecureUrl:
        // should not happen after the initial setup
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::ErrorIdPUnreachable:
        errString = tr("IdP is unreachable. Contact your system administrator or try again later.");
        // TODO: add user facing error message that authentication is currently not possible - retry is the wrong advice
        [[fallthrough]];
    case OAuth::Error:
        errString = tr("Authentication failed.");
        return;
    case OAuth::LoggedIn:
        Q_EMIT authenticationSucceeded(accessToken, refreshToken);
        break;
    }

    if (!errString.isEmpty()) {
        if (_widget)
            _widget->setErrorMessage(errString);
        else
            Q_EMIT authenticationFailed(errString);
    }
}

}
