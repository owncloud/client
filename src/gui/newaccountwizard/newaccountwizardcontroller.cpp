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
#include "common/utility.h"
#include "newaccountmodel.h"
#include "newaccountwizard.h"
#include "oauthpagecontroller.h"
#include "owncloudgui.h"
#include "resources/template.h"
#include "theme.h"
#include "urlpagecontroller.h"

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

    _wizard->setFixedSize(600, 450);
    _wizard->setWizardStyle(QWizard::ModernStyle);
    _wizard->setWindowTitle(tr("Add account..."));

    QWizard::WizardOptions origOptions = _wizard->options();
    _wizard->setOptions(origOptions | QWizard::IndependentPages | QWizard::NoBackButtonOnStartPage);
    // no cancel button is set by default on mac with the original options. just remove it to bring the cancel button back
    _wizard->setOption(QWizard::NoCancelButton, false);

    _wizard->setButtonText(QWizard::BackButton, tr("Back"));
    _wizard->setButtonText(QWizard::CustomButton1, tr("Advanced"));
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

    QWizardPage *advancedSettingsPage = new QWizardPage(_wizard);
    AdvancedSettingsPageController *advancedSettingsController = new AdvancedSettingsPageController(advancedSettingsPage, this);
    connect(advancedSettingsController, &AdvancedSettingsPageController::success, this, &NewAccountWizardController::onAdvancedSettingsCompleted);
    _advancedSettingsPageIndex = _wizard->addPage(advancedSettingsPage, advancedSettingsController);
    advancedSettingsPage->setFinalPage(true);
    // capture the default advanced settings in case the user never runs the advanced settings page
    AdvancedSettingsResult defaultAdvancedSettings = advancedSettingsController->defaultResult();
    _model->setDefaultSyncRoot(defaultAdvancedSettings.syncRoot);
    _model->setSyncType(defaultAdvancedSettings.syncType);
}

void NewAccountWizardController::buildButtonLayouts()
{
    if (_wizard == nullptr)
        return;
    if (Utility::isMac()) {
        _buttonLayouts.insert(
            _urlPageIndex, QList<QWizard::WizardButton>{QWizard::Stretch, QWizard::WizardButton::CancelButton, QWizard::WizardButton::NextButton});
        _buttonLayouts.insert(_oauthPageIndex,
            QList<QWizard::WizardButton>{
                QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::CancelButton, QWizard::WizardButton::NextButton});
        _buttonLayouts.insert(_authSuccessPageIndex,
            QList<QWizard::WizardButton>{QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::CustomButton1,
                QWizard::WizardButton::CancelButton, QWizard::WizardButton::FinishButton});
        _buttonLayouts.insert(_advancedSettingsPageIndex,
            QList<QWizard::WizardButton>{
                QWizard::WizardButton::BackButton, QWizard::Stretch, QWizard::WizardButton::CancelButton, QWizard::WizardButton::FinishButton});
    } else {
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

    // this is also a bit tricky: at this stage the page index is still == _urlPageIndex. However it is in the process of changing to
    // the oauth page. We can't do the auto-validate on the oauth page here.
    // note we should also not advance the page here as that would trigger the url page to validate AGAIN which is definitely not what we want -
    // it already succeeded!
    // so we just set a flag to auto-validate the oauth page and handle that when we are notified that the page actually changed -> see onPageChanged
    _autoValidateOAuthPage = true;
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
    _model->setWebfingerUserInfoUrl(results.webfingerUserUrl);

    _wizard->setCurrentId(_authSuccessPageIndex);
    ownCloudGui::raise();
    // on mac, for unknown reasons, the main window is active after raise
    _wizard->activateWindow();
}

void NewAccountWizardController::onOauthValidationFailed(const OCC::OAuthPageResults &results)
{
    if (!_wizard)
        return;
    Q_UNUSED(results);
    ownCloudGui::raise();
    _wizard->activateWindow();
}

void NewAccountWizardController::onAdvancedSettingsCompleted(const OCC::AdvancedSettingsResult &result)
{
    _model->setDefaultSyncRoot(result.syncRoot);
    _model->setSyncType(result.syncType);
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
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Sign in"));
    } else if (newPageIndex == _oauthPageIndex) {
        _wizard->setButtonLayout(_buttonLayouts[_oauthPageIndex]);
        _wizard->setButtonText(QWizard::WizardButton::NextButton, tr("Open sign in again"));
        if (_autoValidateOAuthPage) {
            _autoValidateOAuthPage = false;
            _wizard->validateCurrentPage();
        }
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
