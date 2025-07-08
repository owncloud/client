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


namespace OCC {

class NewAccountModel;
class NewAccountWizard;
class AccessManager;

class NewAccountWizardController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief NewAccountWizardController
     * @param model must be non-null. Normally the model would be parented by whatever creates the triad, but in this scenario we might need to
     * take a different approach since the whole triad has a short life.
     * @param view which may be null (eg, to support testing). The creator of the view is responsible for parenting it correctly to some other gui element
     * @param parent normally this will be a pointer to whatever instantiated the controller - this is normally a manager or another controller but since the
     * triad is short lived, we may manage the controller lifetime more directly. Nevertheless, the parent should be non-null.
     */
    explicit NewAccountWizardController(NewAccountModel *model, NewAccountWizard *view, QObject *parent);


protected Q_SLOTS:
    void onPageChanged(int newPageIndex);
    // slots for "top level" wizard activity signals
    //  slots for model notifications if useful

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

    // also key to setting up the wizard is that if we make a field mandatory (eg the url QLineEdit) and add a validator, the wizard
    // will enable the "next" button only when the validator returns true for hasAcceptableInput. Not sure if this will be useful yet or not

    /** connects "top level" wizard signals to local slots as needed */
    void connectWizard();

    /** connects the model signals to local slots, as needed */
    void connectModel();

    /** updates the QPalette of the wizard to use the theme colors, if they are valid */
    void updateColors();

    AccessManager *_accessManager = nullptr;

    // using QPointer for the injected dependencies. For an impl like this it's more of a "best practice" formality as the whole
    // bundle of stuff has a shared lifetime.
    QPointer<NewAccountModel> _model = nullptr;
    QPointer<NewAccountWizard> _wizard = nullptr;

    // this is the only controller we need to explicitly keep our eye on as we
    // need to give it values to start the oauth step
    OAuthPageController *_oauthController = nullptr;

    int _urlPageIndex = -1;
    int _oauthPageIndex = -1;
    int _authSuccessPageIndex = -1;
    int _advancedSettingsPageIndex = -1;
};
}
