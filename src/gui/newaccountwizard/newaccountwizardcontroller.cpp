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

#include "newaccountwizardcontroller.h"

#include "accessmanager.h"
#include "advancedsettingspagecontroller.h"
#include "authsuccesspagecontroller.h"
#include "newaccountmodel.h"
#include "newaccountwizard.h"
#include "oauthpagecontroller.h"
#include "resources/template.h"
#include "theme.h"
#include "urlpagecontroller.h"

#include "owncloudgui.h"


namespace OCC {

NewAccountWizardController::NewAccountWizardController(NewAccountModel *model, NewAccountWizard *view, QObject *parent)
    : QObject{parent}
    , _model(model)
    , _wizard(view)
{
    _accessManager = new AccessManager(this);
    setupWizard();
    buildPages();
    connectWizard();
}

void NewAccountWizardController::setupWizard()
{
    if (!_wizard) {
        return;
    }

    updateColors();

    QString appName = Theme::instance()->appNameGUI();
    _wizard->setFixedSize(600, 450);
    _wizard->setWizardStyle(QWizard::ModernStyle);
    _wizard->setWindowTitle(tr("Welcome to %1").arg(appName));

    QWizard::WizardOptions origOptions = _wizard->options();
    _wizard->setOptions(origOptions | QWizard::IndependentPages | QWizard::NoBackButtonOnStartPage);
    // no cancel button is set by default on mac with the original options. just remove it to bring the cancel button back
    _wizard->setOption(QWizard::NoCancelButton, false);
    _wizard->setButtonText(QWizard::WizardButton::FinishButton, tr("Open %1").arg(appName));
}

void NewAccountWizardController::buildPages()
{
    if (!_wizard)
        return;

    QWizardPage *urlPage = new QWizardPage(_wizard);
    UrlPageController *urlController = new UrlPageController(urlPage, _accessManager, this);
    connect(urlController, &UrlPageController::success, this, &NewAccountWizardController::onUrlValidationCompleted);
    _urlPageIndex = _wizard->addPage(urlPage, urlController);

    QWizardPage *oauthPage = new QWizardPage(_wizard);
    _oauthController = new OAuthPageController(oauthPage, _accessManager, this);
    connect(_oauthController, &OAuthPageController::success, this, &NewAccountWizardController::onOAuthValidationCompleted);
    connect(_oauthController, &OAuthPageController::failure, this, &NewAccountWizardController::onOauthValidationFailed);
    _oauthPageIndex = _wizard->addPage(oauthPage, _oauthController);

    QWizardPage *authSuccessPage = new QWizardPage(_wizard);
    AuthSuccessPageController *authSuccessController = new AuthSuccessPageController(authSuccessPage, this);
    _authSuccessPageIndex = _wizard->addPage(authSuccessPage, authSuccessController);

    // todo: #26 - is this actually in play in real life or should it be deprecated?
    if (!Theme::instance()->wizardSkipAdvancedPage()) {
        QWizardPage *advancedSettingsPage = new QWizardPage(_wizard);
        AdvancedSettingsPageController *advancedSettingsController = new AdvancedSettingsPageController(advancedSettingsPage, this);
        _advancedSettingsPageIndex = _wizard->addPage(advancedSettingsPage, advancedSettingsController);
    }
}

void NewAccountWizardController::connectWizard()
{
    if (!_wizard)
        return;

    connect(_wizard, &QWizard::currentIdChanged, this, &NewAccountWizardController::onPageChanged);
}

void NewAccountWizardController::onUrlValidationCompleted(const OCC::UrlPageResults &result)
{
    if (!_model)
        return;

    _model->setServerUrl(result.baseServerUrl);
    _model->setWebfingerAuthenticationUrl(result.webfingerServiceUrl);
    _model->setTrustedCertificates(result.certificates);

    // and then we have to explicitly set login url on the controller for the next page...
    // yes this combination of values is gross however I do not want to pass the server and auth urls in the same function as it's way too easy to mix up the
    // args!
    _oauthController->setServerUrl(_model->serverUrl());
    _oauthController->setAuthenticationUrl(_model->effectiveAuthenticationServerUrl());
    _oauthController->setLookupWebfingerUrls(!_model->webfingerAuthenticationUrl().isEmpty());

    if (_wizard->currentId() != _oauthPageIndex)
        _wizard->setCurrentId(_oauthPageIndex);
    if (_wizard->currentId() == _oauthPageIndex)
        _wizard->validateCurrentPage();
}

// I think this can be removed. we don't really care as the page will not advance and we have no complete result to collect from the
// first page yet.
void NewAccountWizardController::onUrlValidationFailed(const OCC::UrlPageResults &result)
{
    Q_UNUSED(result);
}

void NewAccountWizardController::onOAuthValidationCompleted(const OCC::OAuthPageResults &results)
{
    _model->setAuthToken(results.token);
    _model->setRefreshToken(results.refreshToken);
    _model->setDisplayName(results.displayName);
    _model->setDavUser(results.userId);
    _model->setCapabilities(results.capabilities);

    _wizard->setCurrentId(_authSuccessPageIndex);
    ownCloudGui::raise();
    // on mac, for unknown reasons, the main window is active after raise
    _wizard->activateWindow();
}

void NewAccountWizardController::onOauthValidationFailed(const OCC::OAuthPageResults &results)
{
    Q_UNUSED(results);
    ownCloudGui::raise();
    _wizard->activateWindow();
}

void NewAccountWizardController::onPageChanged(int newPageIndex)
{
    if (newPageIndex == _urlPageIndex) {
        _wizard->setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Next"));
    } else if (newPageIndex == _oauthPageIndex) {
        _wizard->setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Open sign in again"));
    } else if (newPageIndex == _authSuccessPageIndex) {
        // bah! this sets the advanced settings (actually the next button) to be default.
        // QWizard::button returns an abstract button which does not carry the default prop so I can't set it that way.
        // so far I have not found a way to fix that but if I can't, will have to use a custom button and re-route
        // to pretend to be the next button. BAH!!!
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Advanced settings"));
        _wizard->page(_authSuccessPageIndex)->setFinalPage(true);
        _wizard->setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
    } else if (newPageIndex == _advancedSettingsPageIndex) {
        _wizard->page(_advancedSettingsPageIndex)->setFinalPage(true);
        _wizard->setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
    }
}

void NewAccountWizardController::updateColors()
{
    if (!_wizard)
        return;

    QPalette palette = _wizard->palette();
    QColor themeBackground = Theme::instance()->wizardHeaderBackgroundColor();
    QColor themeForeground = Theme::instance()->wizardHeaderTitleColor();
    if (themeBackground.isValid()) {
        palette.setColor(QPalette::Base, themeBackground);
    }
    if (themeForeground.isValid()) {
        palette.setColor(QPalette::Text, themeForeground);
    }

    _wizard->setPalette(palette);
    _wizard->setAutoFillBackground(true);

}
}
