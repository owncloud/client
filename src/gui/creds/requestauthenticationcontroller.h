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
#pragma once

#include <QObject>

#include "oauth.h"

namespace OCC {

class RequestAuthenticationWidget;
class AccountBasedOAuth;
class Account;

class RequestAuthenticationController : public QObject
{
    Q_OBJECT
public:
    explicit RequestAuthenticationController(RequestAuthenticationWidget *widget, QObject *parent);

    void startAuthentication(const AccountPtr account);

Q_SIGNALS:
    void requestLogout();
    void authenticationSucceeded(const QString &token, const QString &refreshToken);
    // this is only emitted if the controller is running without a widget.
    void authenticationFailed(const QString &errorMessage);

private:
    void handleSignIn();
    void handleLogOut();
    void authUrlReady();
    void handleOAuthResult(OAuth::Result, const QString &accessToken, const QString &refreshToken);

    RequestAuthenticationWidget *_widget = nullptr;
    AccountBasedOAuth *_oauth;
};
}
