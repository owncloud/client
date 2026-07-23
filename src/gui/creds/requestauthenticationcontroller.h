/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

#include "oauth.h"

namespace OCC {

class RequestAuthenticationWidget;
class AccountBasedOAuth;
class Account;
class AccountModalWidget;

class RequestAuthenticationController : public QObject
{
    Q_OBJECT
public:
    explicit RequestAuthenticationController(QObject *parent);

    ~RequestAuthenticationController() override;

    void startAuthentication(Account *account);

Q_SIGNALS:
    void requestAccountModal(OCC::AccountModalWidget *widget);
    void requestLogout();
    void authenticationSucceeded(const QString &token, const QString &refreshToken);
    // this is only emitted if the controller is running without a widget.
    void authenticationFailed(const QString &errorMessage);

private:
    void handleSignIn();
    void handleLogOut();
    void authUrlReady();
    void handleOAuthResult(OAuth::Result, const QString &accessToken, const QString &refreshToken);

    AccountBasedOAuth *_oauth = nullptr;
    QPointer<Account> _account = nullptr;

    // these will be cleaned up by the account gui automatically when the AccountModalWidget is "finished"
    // that is handled internally when accept() or reject() is called
    QPointer<RequestAuthenticationWidget> _widget = nullptr;
    QPointer<AccountModalWidget> _modalWidget = nullptr;
};
}
