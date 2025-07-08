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
#include "oauthpagecontroller.h"

#include "accessmanager.h"
#include "accountmanager.h"
#include "networkadapters/fetchcapabilitiesadapter.h"
#include "networkadapters/userinfoadapter.h"
#include "networkadapters/webfingerlookupadapter.h"
#include "resources.h"
#include "template.h"
#include "theme.h"

#include <QBuffer>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWizardPage>

namespace OCC {

OAuthPageController::OAuthPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent)
    : QObject{parent}
    , _page(page)
    , _accessManager(accessManager)
    , _oauth(nullptr)
{
    buildPage();
}

void OAuthPageController::buildPage()
{
    if (!_page)
        return;

    QString appName = Theme::instance()->appNameGUI();

    QLabel *logoLabel = new QLabel({}, _page);
    logoLabel->setPixmap(Theme::instance()->wizardHeaderLogo().pixmap(200, 100));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logoLabel->setAccessibleName(tr("Application Name Logo %1").arg(appName));

    QLabel *instructionLabel =
        new QLabel(tr("Leave this screen open. A sign in prompt will appear in your web browser to connect you to the following address."), _page);
    QFont font = instructionLabel->font();
    font.setPixelSize(14);
    instructionLabel->setFont(font);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    _urlField = new QLineEdit(_page);
    _urlField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _urlField->setEnabled(false);
    _urlField->setAccessibleDescription(tr("Login URL"));

    //    QIcon copyIcon = copyIcon();
    QPushButton *copyButton = new QPushButton(copyIcon(), QString(), _page);
    copyButton->setFlat(true);
    copyButton->setContentsMargins(0, 0, 0, 0);
    copyButton->setFixedSize(_urlField->height(), _urlField->height());
    copyButton->setAccessibleDescription(tr("Copy the login URL to the clipboard"));
    connect(copyButton, &QPushButton::clicked, this, &OAuthPageController::copyUrlClicked);

    _errorField = new QLabel(QString(), _page);
    QPalette errorPalette = _errorField->palette();
    errorPalette.setColor(QPalette::Text, Qt::red);
    _errorField->setPalette(errorPalette);
    _errorField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _errorField->setWordWrap(true);
    _errorField->setAlignment(Qt::AlignLeft);

    QLabel *footerLogoLabel = nullptr;
    if (!Theme::instance()->wizardFooterLogo().isNull()) {
        footerLogoLabel = new QLabel({}, _page);
        footerLogoLabel->setPixmap(Theme::instance()->wizardFooterLogo().pixmap(100, 52));
        footerLogoLabel->setAlignment(Qt::AlignCenter);
        footerLogoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // this is not the same logo as the header logo but we have no idea what this logo could be.
        // this is the designer's suggestion so far
        footerLogoLabel->setAccessibleName(tr("Additional logo defined by the organization"));
    }

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 0, 50, 0);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(logoLabel, Qt::AlignCenter);
    layout->addSpacing(16);
    layout->addWidget(instructionLabel, Qt::AlignCenter);

    QHBoxLayout *urlAreaLayout = new QHBoxLayout();
    urlAreaLayout->setContentsMargins(0, 0, 0, 0);
    urlAreaLayout->setSpacing(0);
    urlAreaLayout->addWidget(_urlField, Qt::AlignLeft);
    urlAreaLayout->addWidget(copyButton);
    layout->addLayout(urlAreaLayout, Qt::AlignCenter);
    layout->addWidget(_errorField, Qt::AlignLeft);
    if (footerLogoLabel)
        layout->addWidget(footerLogoLabel, Qt::AlignCenter);
    layout->addStretch(1);
    _page->setLayout(layout);
}

void OAuthPageController::handleError(const QString &error)
{
    _errorField->setText(error);
    _results.error = error;
    Q_EMIT failure(_results);
}

QIcon OAuthPageController::copyIcon()
{
    // this is required because the background color of the wizard pages can be set via theme and we can't pick an icon
    // color that "fits" based on the current
    // normally we would want to cache this but for this very special one off case I don't think it makes sense
    const QString iconPath = QStringLiteral(":/client/resources/core/copy.svg");
    QPalette pagePalette = _page->palette();
    const QString color = pagePalette.color(QPalette::Text).name();
    QByteArray data = Resources::Template::renderTemplateFromFile(iconPath, {{QStringLiteral("color"), color}}).toUtf8();
    QBuffer buffer(&data);
    QImageReader iconReader(&buffer, "svg");
    return QIcon(QPixmap::fromImageReader(&iconReader));
}

void OAuthPageController::setUrl(const QUrl &url)
{
    _serverUrl = url;
    _urlField->setText(url.toDisplayString());
}

void OAuthPageController::setLookupWebfingerUrls(bool lookup)
{
    _lookupWebfingerUrls = lookup;
}

void OAuthPageController::copyUrlClicked() { }

bool OAuthPageController::validate()
{
    // this should have already been cleaned up after the auth routine but who knows. Never hurts to be sure.
    if (_oauth) {
        delete _oauth;
        _oauth = nullptr;
    }
    // this is a little bit tricky. Because we "auto advance" the wizard to the next page when the oauth step has succeeded, validate
    // will be called *twice*
    // first when the page change triggers the authentication step in the browser, then a second time when we auto-advance after the main controller has
    // received the success signal.
    // In the first call to validate, we always return false because we don't yet know if the asynchronous oauth routine has succeeded.
    // the slot for handling the authentication result sets _oauthCompleted to true when all checks have passed,
    // so the second call to validate triggered by programmatically advancing the page, returns true.
    // we always reset the _oauthCompleted flag after success in case the user wants to go back to reauthenticate for whatever reason, they will start from
    // scratch.
    if (_oauthCompleted) {
        _oauthCompleted = false;
        return true;
    }

    _results = {};
    _errorField->clear();
    _oauth = new OAuth(_serverUrl, {}, _accessManager.get(), this);
    connect(_oauth, &OAuth::result, this, &OAuthPageController::handleOauthResult);
    connect(_oauth, &OAuth::authorisationLinkChanged, this, &OAuthPageController::showBrowser);
    _oauth->startAuthentication();
    return false;
}

void OAuthPageController::showBrowser()
{
    _oauth->openBrowser();
}

void OAuthPageController::handleOauthResult(OAuth::Result result, const QString &token, const QString &refreshToken)
{
    switch (result) {
    case OAuth::Result::LoggedIn: {
        _results.token = token;
        _results.refreshToken = refreshToken;
        if (_lookupWebfingerUrls) {
            WebFingerLookupAdapter lookup(_accessManager, token, _serverUrl);
            const WebFingerLookupResult webfingerResult = lookup.getResult();
            if (!webfingerResult.success()) {
                handleError(tr("Failed to look up webfinger instances: %1").arg(webfingerResult.error));
                return;
            } else {
                if (!webfingerResult.urls.isEmpty())
                    _results.webfingerUserUrl = webfingerResult.urls[0];
            }
        }

        const QUrl userInfoUrl = _results.webfingerUserUrl.isEmpty() ? _serverUrl : _results.webfingerUserUrl;
        UserInfoAdapter infoAdapter(_accessManager, _results.token, userInfoUrl);
        UserInfoResult infoResult = infoAdapter.getResult();

        if (infoResult.success()) {
            if (AccountManager::instance()->accountForLoginExists(_serverUrl, infoResult.userId)) {
                handleError(tr("You are already connected to an account with these credentials."));
                return;
            } else {
                _results.displayName = infoResult.displayName;
                _results.userId = infoResult.userId;
            }
        } else {
            handleError(infoResult.error);
            return;
        }

        // Finally, get the capabilities so we can block oc10 accounts. Checking for spaces support is not
        // great and this should be refined, but for now it's effective.
        FetchCapabilitiesAdapter fetchCapabilities(_accessManager, _results.token, userInfoUrl);
        FetchCapabilitiesResult capabilitiesResult = fetchCapabilities.getResult();
        if (!capabilitiesResult.success()) {
            // I don't think we want to display the core error message as it's stuff like json errors and not
            // useful to the user but we can change this after we have the discussion about error messages
            handleError(tr("Unable to retrieve capabilities from server"));
            break;
        }
        if (!capabilitiesResult.capabilities.spacesSupport().enabled) {
            handleError(tr("The server is not supported by this client"));
            break;
        } else
            _results.capabilities = capabilitiesResult.capabilities;

        _oauthCompleted = true;
        Q_EMIT success(_results);
        break;
    }
    case OAuth::Result::Error: {
        handleError(tr("Error while trying to log in to OAuth2-enabled server."));
        break;
    }
    case OAuth::Result::NotSupported: {
        // should never happen
        handleError(tr("Server reports that OAuth2 is not supported."));
        break;
    }
    case OAuth::Result::ErrorInsecureUrl: {
        handleError(tr("Oauth2 authentication requires a secured connection."));
        break;
    }
    };
    delete _oauth;
    _oauth = nullptr;
}
}
