/*
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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

#include "oauthcredentialssetupwizardstate.h"
#include "gui/networkadapters/webfingerlookupadapter.h"

namespace OCC::Wizard {

OAuthCredentialsSetupWizardState::OAuthCredentialsSetupWizardState(SetupWizardContext *context)
    : AbstractSetupWizardState(context)
{
    const QUrl authServerUrl = _context->accountBuilder().effectiveAuthenticationServerUrl();

    auto oAuth = new OAuth(authServerUrl, {}, _context->accessManager(), this);
    _page = new OAuthCredentialsSetupWizardPage(oAuth, authServerUrl);

    connect(oAuth, &OAuth::result, this, &OAuthCredentialsSetupWizardState::handleOAuthResult);

    // the implementation moves to the next state automatically once ready, no user interaction needed
    _context->window()->disableNextButton();

    oAuth->startAuthentication();
}

void OAuthCredentialsSetupWizardState::handleOAuthResult(OAuth::Result result, const QString &token, const QString &refreshToken)
{
    _context->window()->slotStartTransition();

    // bring window up top again, as the browser may have been raised in front of it
    _context->window()->raise();

    if (!_context->accountBuilder().webFingerAuthenticationServerUrl().isEmpty()) {
        // Lisa todo: why do we use the serverUrl here instead of the effective url?!
        WebFingerLookupAdapter lookup(_context->accessManager(), token, _context->accountBuilder().serverUrl());
        const WebFingerLookupResult webfingerResult = lookup.getResult();
        if (!webfingerResult.success()) {
            Q_EMIT evaluationFailed(QStringLiteral("Failed to look up instances: %1").arg(webfingerResult.error));
        } else {
            _context->accountBuilder().setWebFingerInstances(webfingerResult.urls);
        }
    }
    switch (result) {
    case OAuth::Result::LoggedIn: {
        _context->accountBuilder().setAuthenticationStrategy(new OAuthAuthenticationStrategy(token, refreshToken));
        Q_EMIT evaluationSuccessful();
        break;
    }
    case OAuth::Result::Error: {
        Q_EMIT evaluationFailed(tr("Error while trying to log in to OAuth2-enabled server."));
        break;
    }
    case OAuth::Result::NotSupported: {
        // should never happen
        Q_EMIT evaluationFailed(tr("Server reports that OAuth2 is not supported."));
        break;
    }
    case OAuth::Result::ErrorInsecureUrl: {
        Q_EMIT evaluationFailed(tr("OAuth2 authentication requires a secured connection."));
        break;
    }
    case OAuth::Result::ErrorIdPUnreachable: {
        Q_EMIT evaluationFailed(tr("Authorization server unreachable."));
        break;
    }
    };
}

SetupWizardState OAuthCredentialsSetupWizardState::state() const
{
    return SetupWizardState::CredentialsState;
}

void OAuthCredentialsSetupWizardState::evaluatePage()
{
    // the next button is disabled anyway, since moving forward is controlled by the OAuth object signal handlers
    // therefore, this method should never ever be called
    Q_UNREACHABLE();
}

} // OCC::Wizard
