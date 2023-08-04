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

#pragma once
#include "creds/httpcredentials.h"
#include "creds/oauth.h"
#include <QPointer>
#include <QTcpServer>

namespace OCC {

/**
 * @brief The HttpCredentialsGui class
 * @ingroup gui
 */
class HttpCredentialsGui : public HttpCredentials
{
    Q_OBJECT
public:
    static HttpCredentialsGui *fromSettings(AccountState *accountState);

    HttpCredentialsGui(AccountState *accountState, const QString &loginUser, const QString &password);

    HttpCredentialsGui(AccountState *accountState, const QString &davUser, const QString &password, const QString &refreshToken);

    void openBrowser();


    void persist() override;

    QUrl authorisationLink() const;

    /**
     * This will query the server and either uses OAuth via _asyncAuth->start()
     * or call showDialog to ask the password
     */
    void askFromUser() override;

    /**
     * Reset OAuth object and restart authorization process.
     */
    void restartOAuth();

private slots:
    void asyncAuthResult(OAuth::Result, const QString &accessToken, const QString &refreshToken);
    void showDialog();
    void askFromUserAsync();

signals:
    void authorisationLinkChanged();

    void oAuthErrorOccurred();

protected:
    HttpCredentialsGui(Account *account);
    ;

private:
    QScopedPointer<AccountBasedOAuth, QScopedPointerObjectDeleteLater<AccountBasedOAuth>> _asyncAuth;
    QPointer<QWidget> _loginRequiredDialog;
};

} // namespace OCC
