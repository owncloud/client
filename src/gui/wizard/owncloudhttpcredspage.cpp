/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
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

#include "QProgressIndicator.h"

#include "creds/httpcredentialsgui.h"
#include "theme.h"
#include "account.h"
#include "configfile.h"
#include "wizard/owncloudhttpcredspage.h"
#include "wizard/owncloudwizardcommon.h"
#include "wizard/owncloudwizard.h"

namespace OCC {

OwncloudHttpCredsPage::OwncloudHttpCredsPage(QWidget *parent)
    : AbstractCredentialsWizardPage()
    , _progressIndi(new QProgressIndicator(this))
{
    _ui.setupUi(this);


    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(Theme::instance()->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Enter user credentials")));

    _ui.resultLayout->addWidget(_progressIndi);
    stopSpinner();
}

void OwncloudHttpCredsPage::initializePage()
{
    WizardCommon::initErrorLabel(_ui.errorLabel);

    Theme *theme = Theme::instance();
    _ui.usernameLabel->setText(theme->enumToDisplayName(theme->userIDType()));
    _ui.leUsername->setPlaceholderText(theme->userIDHint());

    _ui.leUsername->setText(owncloudWizard()->account()->davUser());

    _ui.tokenLabel->setText(HttpCredentialsGui::requestAppPasswordText(owncloudWizard()->account().data()));
    _ui.tokenLabel->setVisible(!_ui.tokenLabel->text().isEmpty());
    _ui.leUsername->setFocus();
}

void OwncloudHttpCredsPage::cleanupPage()
{
    _ui.lePassword->clear();
}

bool OwncloudHttpCredsPage::validatePage()
{
    if (_ui.lePassword->text().isEmpty()) {
        return false;
    }

    if (!_connected) {
        _ui.errorLabel->setVisible(false);
        startSpinner();

        // Reset cookies to ensure the username / password is actually used
        owncloudWizard()->account()->clearCookieJar();

        emit completeChanged();
        emit connectToOCUrl(field("OCUrl").toString().simplified());

        if (Theme::instance()->wizardSkipAdvancedPage()) {
            emit owncloudWizard()->createLocalAndRemoteFolders();
        }

        return false;
    } else {
        // Reset, to require another connection attempt next time
        _connected = false;

        emit completeChanged();
        stopSpinner();
        return true;
    }
    return true;
}

int OwncloudHttpCredsPage::nextId() const
{
    if (Theme::instance()->wizardSkipAdvancedPage()) {
        return -1;
    }
    return WizardCommon::Page_AdvancedSetup;
}

void OwncloudHttpCredsPage::setConnected()
{
    _connected = true;
    stopSpinner();
}

void OwncloudHttpCredsPage::startSpinner()
{
    _ui.resultLayout->setEnabled(true);
    _progressIndi->setVisible(true);
    _progressIndi->startAnimation();
}

void OwncloudHttpCredsPage::stopSpinner()
{
    _ui.resultLayout->setEnabled(false);
    _progressIndi->setVisible(false);
    _progressIndi->stopAnimation();
}

void OwncloudHttpCredsPage::setErrorString(const QString &err)
{
    if (err.isEmpty()) {
        _ui.errorLabel->setVisible(false);
    } else {
        _ui.errorLabel->setVisible(true);
        _ui.errorLabel->setText(err);
    }
    emit completeChanged();
    stopSpinner();
}

AbstractCredentials *OwncloudHttpCredsPage::getCredentials() const
{
    return new HttpCredentialsGui(_ui.leUsername->text(), _ui.lePassword->text());
}


} // namespace OCC
