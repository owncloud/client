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

#include <QAbstractButton>

namespace OCC {

NewAccountWizardController::NewAccountWizardController(NewAccountModel *model, NewAccountWizard *view, QObject *parent)
    : QObject{parent}
    , _model(model)
    , _wizard(view)
{
    _accessManager = new AccessManager(this);
    setupWizard();
    buildPages();
    buildButtonLayouts();
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
    //   _wizard->setButtonText(QWizard::WizardButton::FinishButton, tr("Open %1").arg(appName));

    _wizard->setButtonText(QWizard::CustomButton1, tr("Advanced Settings"));
    _wizard->setOption(QWizard::HaveCustomButton1, true);
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
    authSuccessPage->setFinalPage(true);

    // todo: #26 - is this actually in play in real life or should it be deprecated?
    //  if (!Theme::instance()->wizardSkipAdvancedPage()) {
    connect(authSuccessController, &AuthSuccessPageController::requestAdvancedSettings, this, &NewAccountWizardController::showAdvancedSettingsPage);

    QWizardPage *advancedSettingsPage = new QWizardPage(_wizard);
    AdvancedSettingsPageController *advancedSettingsController = new AdvancedSettingsPageController(advancedSettingsPage, this);
    _advancedSettingsPageIndex = _wizard->addPage(advancedSettingsPage, advancedSettingsController);
    advancedSettingsPage->setFinalPage(true);
    //  }
}

void NewAccountWizardController::buildButtonLayouts()
{
    if (_wizard == nullptr)
        return;

    _buttonLayouts.insert(
        _urlPageIndex, QList<QWizard::WizardButton>{QWizard::Stretch, QWizard::WizardButton::NextButton, QWizard::WizardButton::CancelButton});
    _buttonLayouts.insert(_oauthPageIndex,
        QList<QWizard::WizardButton>{
            QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::NextButton, QWizard::WizardButton::CancelButton});
    _buttonLayouts.insert(_authSuccessPageIndex,
        QList<QWizard::WizardButton>{QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::CustomButton1,
            QWizard::WizardButton::FinishButton, QWizard::WizardButton::CancelButton});
    _buttonLayouts.insert(_advancedSettingsPageIndex,
        QList<QWizard::WizardButton>{
            QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::FinishButton, QWizard::WizardButton::CancelButton});
}

void NewAccountWizardController::connectWizard()
{
    if (!_wizard)
        return;

    connect(_wizard, &QWizard::currentIdChanged, this, &NewAccountWizardController::onPageChanged);
    // normally you'd want to check the index of the custom button passed by the signal to be sure it == 1 but we only have one custom
    // button so keep it dumb.
    connect(_wizard, &QWizard::customButtonClicked, this, &NewAccountWizardController::showAdvancedSettingsPage);
}

void NewAccountWizardController::onUrlValidationCompleted(const OCC::UrlPageResults &result)
{
    if (!_wizard || !_model)
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
}

// I think this can be removed. we don't really care as the page will not advance and we have no complete result to collect from the
// first page yet.
void NewAccountWizardController::onUrlValidationFailed(const OCC::UrlPageResults &result)
{
    Q_UNUSED(result);
}

void NewAccountWizardController::onOAuthValidationCompleted(const OCC::OAuthPageResults &results)
{
    if (!_wizard || !_model)
        return;
    _model->setAuthToken(results.token);
    _model->setRefreshToken(results.refreshToken);
    _model->setDisplayName(results.displayName);
    _model->setDavUser(results.userId);
    _model->setCapabilities(results.capabilities);

    _wizard->setCurrentId(_authSuccessPageIndex);
    ownCloudGui::raise();
}

void NewAccountWizardController::onOauthValidationFailed(const OCC::OAuthPageResults &results)
{
    if (!_wizard)
        return;
    Q_UNUSED(results);
    ownCloudGui::raise();
}

void NewAccountWizardController::showAdvancedSettingsPage()
{
    if (_advancedSettingsPageIndex > -1)
        _wizard->setCurrentId(_advancedSettingsPageIndex);
}

void NewAccountWizardController::onPageChanged(int newPageIndex)
{
    if (newPageIndex == _urlPageIndex) {
        _wizard->setButtonLayout(_buttonLayouts[_urlPageIndex]);
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Next"));
    } else if (newPageIndex == _oauthPageIndex) {
        _wizard->setButtonLayout(_buttonLayouts[_oauthPageIndex]);
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Open sign in again"));
    } else if (newPageIndex == _authSuccessPageIndex) {
        _wizard->setButtonLayout(_buttonLayouts[_authSuccessPageIndex]);
        // for some reason - probably because this is not the last, last page ever, the focus kept coming out on the
        // back button. this seems to fix it.
        _wizard->button(QWizard::WizardButton::FinishButton)->setFocus(Qt::OtherFocusReason);
    } else if (newPageIndex == _advancedSettingsPageIndex) {
        _wizard->setButtonLayout(_buttonLayouts[_advancedSettingsPageIndex]);
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
