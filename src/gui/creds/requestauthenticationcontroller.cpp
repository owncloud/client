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

#include "accountsettings.h"
#include "accountview.h"
#include "application.h"
#include "requestauthenticationwidget.h"

#include "accountmodalwidget.h"
#include "settingsdialog.h"

#include <config/systemconfig.h>

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
    if (_modalWidget) {
        // this calls finished which deletes the modal widget.
        // our widget should also be deleted as it has been added to the layout of the modal
        // widget, which effectively reparents it to the modal widget.
        _modalWidget->reject();
    }
    // this ultimately needs to be chained to creds::requestLogout signal
    Q_EMIT requestLogout();
}

void RequestAuthenticationController::startAuthentication(Account *account)
{
    Q_ASSERT(account);
    if (_oauth) {
        delete _oauth;
        _oauth = nullptr;
    }
    SystemConfig systemConfig;
    _account = account;
    _oauth = new AccountBasedOAuth(_account, systemConfig.openIdConfig(), this);
    connect(_oauth, &OAuth::authorisationLinkChanged, this, &RequestAuthenticationController::authUrlReady);
    connect(_oauth, &OAuth::result, this, &RequestAuthenticationController::handleOAuthResult);
    if (_widget && _modalWidget == nullptr) { // first show of the gui
        connect(_widget, &RequestAuthenticationWidget::connectClicked, this, &RequestAuthenticationController::handleSignIn);
        connect(_widget, &RequestAuthenticationWidget::stayLoggedOutClicked, this, &RequestAuthenticationController::handleLogOut);
#ifdef USE_NEW_FOLDER_LIST
        AccountView *accountView = ocApp()->gui()->settingsDialog()->accountView(_account);
        _modalWidget = new AccountModalWidget(QString(), _widget, accountView);
        accountView->addModalAccountWidget(_modalWidget);
#else
        AccountSettings *accountSettings = ocApp()->gui()->settingsDialog()->accountSettings(_account);
        _modalWidget = new AccountModalWidget(QString(), _widget, accountSettings);
        accountSettings->addModalAccountWidget(_modalWidget);
#endif
    }
    _oauth->startAuthentication();
}

void RequestAuthenticationController::authUrlReady()
{
    if (_widget) {
        _widget->setAuthUrl(_oauth->authorisationLink().toString(QUrl::FullyEncoded));
    } else {
        // just run with it
        _oauth->openBrowser();
    }
}

void RequestAuthenticationController::handleOAuthResult(OAuth::Result result, const QString &accessToken, const QString &refreshToken)
{
    QString errString;

    switch (result) {
    case OAuth::ErrorIdPUnreachable:
        errString = tr("IdP is unreachable. Contact your system administrator or try again later.");
        break;
    case OAuth::NotSupported:
        // also should not happen after initial setup?
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::ErrorInsecureUrl:
        // should not happen after the initial setup
        Q_ASSERT(false);
        [[fallthrough]];
    case OAuth::Error:
        errString = tr("Authentication failed.");
        break;
    case OAuth::LoggedIn:
        errString.clear();
        break;
    }


    if (!errString.isEmpty() && _account) {
        if (_widget) {
            _widget->setErrorMessage(errString);
            startAuthentication(_account);
        } else
            Q_EMIT authenticationFailed(errString);
    } else {
        if (_modalWidget)
            _modalWidget->accept();
        Q_EMIT authenticationSucceeded(accessToken, refreshToken);
    }

    ownCloudGui::raise();
}
}
