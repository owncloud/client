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
#include "urlpagecontroller.h"
#include <qabstractbutton.h>


namespace OCC {

NewAccountWizardController::NewAccountWizardController(NewAccountModel *model, NewAccountWizard *view, QObject *parent)
    : QObject{parent}
    , _model(model)
    , _wizard(view)
{
    _accessManager = new AccessManager(this);
    setupWizard();
    buildPages();
}

void NewAccountWizardController::setupWizard()
{
    if (!_wizard) {
        return;
    }
    _wizard->setWizardStyle(QWizard::ModernStyle);
    // todo: try to get the app name from the theme - not sure if this always works
    _wizard->setWindowTitle(tr("Welcome to Kiteworks"));
    // this presumes we want different options on mac vs others.
    QWizard::WizardOptions origOptions = _wizard->options();

    _wizard->setOptions(origOptions | QWizard::IndependentPages | QWizard::NoBackButtonOnStartPage /*| QWizard::IgnoreSubTitles*/);

    _wizard->setButtonText(QWizard::WizardButton::FinishButton, tr("Open Kiteworks"));

    connectWizard();
}

void NewAccountWizardController::buildPages()
{
    if (!_wizard)
        return;

    QWizardPage *urlPage = new QWizardPage(_wizard);
    UrlPageController *urlController = new UrlPageController(urlPage, _accessManager, this);
    _urlPageIndex = _wizard->addPage(urlPage, urlController);

    QWizardPage *oauthPage = new QWizardPage(_wizard);
    OAuthPageController *oauthController = new OAuthPageController(oauthPage, this);
    _oauthPageIndex = _wizard->addPage(oauthPage, oauthController);

    QWizardPage *authSuccessPage = new QWizardPage(_wizard);
    AuthSuccessPageController *authSuccessController = new AuthSuccessPageController(authSuccessPage, this);
    _authSuccessPageIndex = _wizard->addPage(authSuccessPage, authSuccessController);

    QWizardPage *advancedSettingsPage = new QWizardPage(_wizard);
    AdvancedSettingsPageController *advancedSettingsController = new AdvancedSettingsPageController(advancedSettingsPage, this);
    _advancedSettingsPageIndex = _wizard->addPage(advancedSettingsPage, advancedSettingsController);
}

void NewAccountWizardController::connectWizard()
{
    if (!_wizard)
        return;

    connect(_wizard, &QWizard::currentIdChanged, this, &NewAccountWizardController::onPageChanged);
}

void NewAccountWizardController::onPageChanged(int newPageIndex)
{
    if (newPageIndex == _urlPageIndex) {
        _wizard->setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
        // I think we should rename next to "sign in" on the first page
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
}
