/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

protected:
    bool eventFilter(QObject *target, QEvent *event) override;

signals:
    void connectClicked();
    void stayLoggedOutClicked();

private:
    void onClipboardChanged();
    void onCopyUrl();
    QString elidedUrl(int targetWidth);

    void updateColors();

    QString _authUrl;
    QLabel *_urlField = nullptr;
    QLabel *_errorField = nullptr;
    QPushButton *_copyButton = nullptr;
    QIcon _copyIcon;
    QPushButton *_cancelButton = nullptr;
    QPushButton *_signInButton = nullptr;
};
}
