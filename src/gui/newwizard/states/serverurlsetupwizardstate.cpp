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

#include "serverurlsetupwizardstate.h"
#include "gui/networkadapters/determineauthtypeadapter.h"
#include "gui/networkadapters/discoverwebfingerserviceadapter.h"
#include "gui/networkadapters/resolveurladapter.h"
#include "theme.h"


#include <QDebug>
#include <QMessageBox>


namespace OCC::Wizard {

Q_LOGGING_CATEGORY(lcSetupWizardServerUrlState, "gui.setupwizard.states.serverurl");

ServerUrlSetupWizardState::ServerUrlSetupWizardState(SetupWizardContext *context)
    : AbstractSetupWizardState(context)
{
    // how can the context already have a server url if it has not been set up?
    QUrl serverUrl = _context->accountBuilder().serverUrl();
    _page = new ServerUrlSetupWizardPage(serverUrl);
}

SetupWizardState ServerUrlSetupWizardState::state() const
{
    return SetupWizardState::ServerUrlState;
}

QUrl ServerUrlSetupWizardState::calculateUrl() const
{
    if (!_page)
        return QUrl();

    auto serverUrlSetupWizardPage = qobject_cast<ServerUrlSetupWizardPage *>(_page);
    Q_ASSERT(serverUrlSetupWizardPage != nullptr);

    QString userProvidedUrl = serverUrlSetupWizardPage->userProvidedUrl();

    // we need to check the scheme to be sure it's not there, but using the old "string matching" technique was not great
    // because it added https:// to urls that had valid schemes but which did not match http or https. That was more complicating
    // than useful so now we only add https if there really is no valid scheme to start with.
    // the caller should decide which schemes are allowed and take whatever action it sees fit

    // strict mode really does not appear to be useful wrt catching malformed urls but hey, why not.
    QUrl testUrl(userProvidedUrl, QUrl::StrictMode);
    QString scheme = testUrl.scheme();
    if (scheme.isEmpty()) {
        qInfo(lcSetupWizardServerUrlState) << "no URL scheme provided, prepending default URL scheme: https://";
        userProvidedUrl.prepend(QStringLiteral("https://"));
        testUrl.setScheme(QStringLiteral("https"));
        // so far I don't see testUrl coming back as invalid even if it should be but it seems reasonable to set the scheme anyway before testing validity.
        // we catch the invalid condition later using a new url after reconstructing it with the default scheme added to the input string. see comment below.
        // no I can't explain this behavior - it's a Qt mystery
    }
    if (!testUrl.isValid()) {
        // if it's already dead, stop trying
        return testUrl;
    }

    // we can't recycle the testUrl because for reasons completely unknown to me, if you try to adjust it or set the path it destroys
    // the host so we get https:///kwdav/ every time, even if the testUrl was valid
    // solution = instantiate a new url <shrug>
    QUrl finalUrl(userProvidedUrl);

    if (finalUrl.isValid()) {
        finalUrl = finalUrl.adjusted(QUrl::RemoveUserInfo | QUrl::RemoveQuery);
        const QString serverPathOverride = Theme::instance()->overrideServerPath();
        if (!serverPathOverride.isEmpty()) {
            finalUrl.setPath(serverPathOverride);
        }
    }
    return finalUrl;
}

void ServerUrlSetupWizardState::evaluatePage()
{
    // we don't want to store any unnecessary certificates for this account when the user returns to the first page
    // the easiest way is to just reset the account builder
    _context->resetAccountBuilder();

    const QUrl serverUrl = calculateUrl();


    // TODO: perform some better validation
    // todo: #21
    if (!serverUrl.isValid()) {
        QString fullError = serverUrl.errorString();
        QStringList parts = fullError.split(QStringLiteral(";"), Qt::SkipEmptyParts);
        // it might be too much to print both the error and the source string - eval with input from others.
        Q_EMIT evaluationFailed(tr("Invalid server URL: %1").arg(parts[0] + parts[1]));
        return;
    }

    // (ab)use the account builder as temporary storage for the URL we are about to probe (after sanitation)
    // in case of errors, the user can just edit the previous value

    // this only works if the url is valid. If it's invalid the original query text is wiped out from the QUrl instance and I don't
    // see an easy way to put it back because...erm...it wants to take the text from a url but it's invalid so the original text is gone.
    // I improved the error message to extract the original input in case it helps, but this new behavior should be reviewed.
    _context->accountBuilder().setServerUrl(serverUrl, AuthenticationType::Unknown);

    if (serverUrl.scheme() == QStringLiteral("http")) {
        auto *messageBox = new QMessageBox(QMessageBox::Warning, tr("Insecure connection"),
            tr("The connection to %1 is insecure.\nAre you sure you want to proceed?").arg(serverUrl.toString()), QMessageBox::NoButton, _context->window());

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->addButton(QMessageBox::Cancel);
        messageBox->addButton(tr("Confirm"), QMessageBox::YesRole);
        int messageResult = messageBox->exec();
        if (messageResult == QMessageBox::Cancel) {
            Q_EMIT evaluationFailed(tr("Insecure server rejected by user"));
            return;
        }
    } else if (serverUrl.scheme().isEmpty() || serverUrl.scheme() != QStringLiteral("https")) // scheme should not be empty but who knows
    {
        Q_EMIT evaluationFailed(tr("Invalid URL scheme. Only http and https are accepted."));
        return;
    }

    // when moving back to this page (or retrying a failed credentials check), we need to make sure existing cookies
    // and certificates are deleted from the access manager
    _context->resetAccessManager();

    // since classic WebFinger is not enabled, we need to check whether modern (oCIS) WebFinger is available
    // therefore, we run the corresponding discovery job
    DiscoverWebFingerServiceAdapter webfingerServiceAdapter(_context->accessManager(), serverUrl);
    const DiscoverWebFingerServiceResult webfingerServiceResult = webfingerServiceAdapter.getResult();

    // in case any kind of error occurs, we assume the WebFinger service is not available
    if (!webfingerServiceResult.success()) {
        // first, we must resolve the actual server URL
        ResolveUrlAdapter urlResolver(_context->accessManager(), serverUrl);
        const ResolveUrlResult resolveUrlResult = urlResolver.getResult();

        if (!resolveUrlResult.success()) {
            Q_EMIT evaluationFailed(resolveUrlResult.error);
            return;
        }
        const QUrl finalUrl = resolveUrlResult.resolvedUrl;

        if (finalUrl.hasQuery()) {
            QString errorMsg = tr("The requested URL failed with query value: %1").arg(finalUrl.query());
            Q_EMIT evaluationFailed(errorMsg);
            return;
        }

        if (!resolveUrlResult.acceptedCertificates.isEmpty()) {
            // future requests made through this access manager should accept the certificate
            _context->accessManager()->addCustomTrustedCaCertificates(resolveUrlResult.acceptedCertificates);

            // the account maintains a list, too, which is also saved in the config file
            for (const auto &cert : resolveUrlResult.acceptedCertificates)
                _context->accountBuilder().addCustomTrustedCaCertificate(cert);
        }

        // next, we need to find out which kind of authentication page we have to present to the user
        // todo: #18
        // I so far do not see any totally reliable method of determining whether user is trying to connect to an
        // owncloud server so I think we need an extra step to check the server capabilities before creating the account -
        // eg check the name and if it isn't kiteworks or ocis fail it. I think this is probably risky in case the name changes
        // but it's a place to start
        DetermineAuthTypeAdapter authTypeAdapter(_context->accessManager(), finalUrl);
        const DetermineAuthTypeResult authResult = authTypeAdapter.getResult();
        if (!authResult.success()) {
            Q_EMIT evaluationFailed(authResult.error);
            return;
        }

        Q_ASSERT(authResult.type == AuthenticationType::OAuth);
        _context->accountBuilder().setServerUrl(finalUrl, authResult.type);
        Q_EMIT evaluationSuccessful();


    } else {
        _context->accountBuilder().setWebFingerAuthenticationServerUrl(QUrl(webfingerServiceResult.href));
        Q_EMIT evaluationSuccessful();
    }
}

} // OCC::Wizard
