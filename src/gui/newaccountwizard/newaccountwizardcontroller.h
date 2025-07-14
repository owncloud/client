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

#include "oauthpagecontroller.h"
#include "urlpagecontroller.h"
#include <QObject>
#include <QPointer>
#include <QWizard>


namespace OCC {

class NewAccountModel;
class NewAccountWizard;
class AccessManager;


/**
 * @brief The NewAccountWizardController class is a top level controller for the new account wizard gui. It does not contain any business logic associated with
 * calculating values or retrieving information from the server, instead it orchestrates the behavior of the wizard, especially in relation to the results
 * obtained by the page controllers. This controller is not a candidate for unit testing because the gui is an integral part of it's operation.
 */
class NewAccountWizardController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief NewAccountWizardController
     * @param model must not be null.
     * @param view which should not be null. The creator of the view is responsible for parenting it correctly to some other gui element
     * @param parent normally this will be a pointer to whatever instantiated the controller, eg a manager or another controller, but since the
     * triad is short lived, we may also manage the controller by instantiating it on the stack with no parent.
     */
    explicit NewAccountWizardController(NewAccountModel *model, NewAccountWizard *view, QObject *parent);


protected Q_SLOTS:
    // slots for handling signals from the wizard:
    void onPageChanged(int newPageIndex);
    void showAdvancedSettingsPage();

    // slots for handling signals from the page controllers:
    void onUrlValidationCompleted(const OCC::UrlPageResults &result);
    // the failed slot is currently unused - I don't think we need to listen on this one for the wizard, will remove it if it's really
    // useless in the end
    void onUrlValidationFailed(const OCC::UrlPageResults &result);

    void onOAuthValidationCompleted(const OCC::OAuthPageResults &results);
    // we need to raise the gui when oauth validation fails
    void onOauthValidationFailed(const OCC::OAuthPageResults &results);


private:
    /** configures the wizard with proper settings */
    void setupWizard();

    /** builds the page controller/page widget pairs */
    void buildPages();

    void buildButtonLayouts();

    // also key to setting up the wizard is that if we make a field mandatory (eg the url QLineEdit) and add a validator, the wizard
    // will enable the "next" button only when the validator returns true for hasAcceptableInput. IMO this is mandatory as the next button is
    // currently enabled even if the url field is empty

    /** connects "top level" wizard signals to local slots as needed */
    void connectWizard();

    /** connects the model signals to local slots, as needed */
    void connectModel();

    /** updates the QPalette of the wizard to use the theme colors, if they are valid */
    void updateColors();

    /** this controller owns the access manager that will be used by the page controllers to run various network requests */
    AccessManager *_accessManager = nullptr;

    // using QPointer for the injected dependencies. For an impl like this it's more of a "best practice" formality as the whole
    // bundle of stuff has a shared lifetime.
    QPointer<NewAccountModel> _model = nullptr;
    QPointer<NewAccountWizard> _wizard = nullptr;

    // this is the only controller we need to explicitly keep our eye on as we need to give it values to start the oauth step
    OAuthPageController *_oauthController = nullptr;
    bool _autoValidateOAuthPage = false;

    QHash<int, QList<QWizard::WizardButton>> _buttonLayouts;

    int _urlPageIndex = -1;
    int _oauthPageIndex = -1;
    int _authSuccessPageIndex = -1;
    int _advancedSettingsPageIndex = -1;
};
}
