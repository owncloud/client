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

#include <QIcon>
#include <QWidget>

class QLabel;
class QPushButton;

namespace OCC {


class RequestAuthenticationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RequestAuthenticationWidget(QWidget *parent = nullptr);

    void setAuthUrl(const QString &url);
    void setErrorMessage(const QString &error);

Q_SIGNALS:
    void connectClicked();
    void stayLoggedOutClicked();

private:
    void onClipboardChanged();
    void onCopyUrl();

    void updateColors();

    QString _authUrl;
    QLabel *_urlField = nullptr;
    QLabel *_errorField = nullptr;
    QPushButton *_copyButton = nullptr;
    QIcon _copyIcon;
};
}
