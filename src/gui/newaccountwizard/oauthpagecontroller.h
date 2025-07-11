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

/**
 * @brief The OAuthPageResults class is a simple container that carries the results of the page validation
 */
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

/**
 * @brief The OAuthPageController class controls the authentication step in the NewAccountWizard
 */
class OAuthPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT
public:
    /**
     * @brief OAuthPageController
     * @param page is the page this controller will build and manage. Nullptr is acceptable eg for testing
     * @param accessManager is the access manager that will be used for all network requests
     * @param parent implements the usual Qt parenting scheme.
     */
    explicit OAuthPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent);

    /**
     * @brief validate runs the OAuth routines to authenticate the user and gather the tokens for the account, and it also runs various
     * "final checks" on the data to ensure we can or should use these credentials to create an account
     * @return true if the all the checks pass, false if it fails.
     * This function also emits success or failure signals, below, to provide the OAuthPageResults to interested parties.
     */
    bool validate() override;

    /**
     * @brief setServerUrl is the original url used to kick off the account creation process. This url is not necessarily the same as the authentication url
     * (which may be a webfinger authentication url). Even if the authentication goes through webfinger, we still need to use the server url to eg do the
     * webfinger lookup among other things.
     */
    void setServerUrl(const QUrl &url);

    /**
     * @brief setAuthenticationUrl sets the authentication url. This may be equal to the server url or it may be a webfinger authentication url.
     * @param url
     */
    void setAuthenticationUrl(const QUrl &url);

    void setLookupWebfingerUrls(bool lookup);

Q_SIGNALS:
    /**
     * @brief success notifies interested parties that the validation routine succeeded
     * @param results contains the data collected during the validation
     */
    void success(const OCC::OAuthPageResults &results);
    // for this controller the failure signal is important, as the main wizard controller uses it to raise the client window after the user
    // has completed the browser login
    /**
     * @brief failure notifies interested parties that the validation failed
     * @param results contains the error that led to the failure, and also may contain other legitimate values depending on how far the validation
     * progressed before it failed
     */
    void failure(const OCC::OAuthPageResults &results);

protected Q_SLOTS:
    /**
     * @brief copyUrlClicked copies the content of the url field to the clipboard
     */
    void copyUrlClicked();
    /**
     * @brief clipboardChanged updates the tooltip on the copy button depending on whether the clipboard contains the current url or not
     */
    void clipboardChanged();

private:
    QPointer<QWizardPage> _page;
    QPointer<AccessManager> _accessManager;
    QUrl _serverUrl;
    QUrl _authUrl;
    OAuth *_oauth;
    bool _lookupWebfingerUrls = false;

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
