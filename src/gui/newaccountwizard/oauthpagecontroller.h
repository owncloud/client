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

#include "capabilities.h"
#include "creds/oauth.h"
#include "wizardpagevalidator.h"

#include <QObject>
#include <QPointer>

class QWizardPage;
class QLineEdit;
class QLabel;
class QPushButton;

namespace OCC {

class AccessManager;
class OAuth;

struct OAuthPageResults
{
    QString token;
    QString refreshToken;
    QUrl webfingerUserUrl;
    QString displayName;
    QString userId;
    Capabilities capabilities = {QUrl{}, {}};

    QString error;
};

class OAuthPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT
public:
    explicit OAuthPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent);
    bool validate() override;

    void setUrl(const QUrl &url);
    void setLookupWebfingerUrls(bool lookup);

Q_SIGNALS:
    void success(const OCC::OAuthPageResults &results);
    // for this controller the failure signal is important, as the main wizard controller uses it to raise the client window after the user
    // has completed the browser login
    void failure(const OCC::OAuthPageResults &results);

protected Q_SLOTS:
    void copyUrlClicked();
    void clipboardChanged();

private:
    QPointer<QWizardPage> _page;
    QPointer<AccessManager> _accessManager;
    QUrl _serverUrl;
    bool _lookupWebfingerUrls = false;
    OAuth *_oauth;

    QLineEdit *_urlField;
    QLabel *_errorField;
    QPushButton *_copyButton;

    bool _oauthCompleted = false;
    OAuthPageResults _results;


    void buildPage();
    QIcon copyIcon();
    /** displays the error in the page, sets the error value in the results and emits failure(results) */
    void handleError(const QString &error);
    void handleOauthResult(OAuth::Result result, const QString &token = QString(), const QString &refreshToken = QString());
    void showBrowser();
};
}
Q_DECLARE_METATYPE(OCC::OAuthPageResults)
// this type id is throwaway, we just use it to ensure we declare the meta type only ONCE
// also this is only required to use the type cross thread, or with QSignalSpy during testing
static const int oauthPageResultsTypeId = qRegisterMetaType<OCC::OAuthPageResults>();
