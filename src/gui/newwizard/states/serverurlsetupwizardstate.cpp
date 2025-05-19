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
#include "determineauthtypejobfactory.h"
#include "gui/networkadapters/discoverwebfingerserviceadapter.h"
#include "gui/networkadapters/resolveurladapter.h"
#include "theme.h"


#include <QDebug>
#include <QMessageBox>

namespace {

const QString defaultUrlSchemeC = QStringLiteral("https");
const QStringList supportedUrlSchemesC({ defaultUrlSchemeC, QStringLiteral("http://") });

}

namespace OCC::Wizard {

Q_LOGGING_CATEGORY(lcSetupWizardServerUrlState, "gui.setupwizard.states.serverurl");

ServerUrlSetupWizardState::ServerUrlSetupWizardState(SetupWizardContext *context)
    : AbstractSetupWizardState(context)
{
    auto serverUrl = [this]() {
        if (Theme::instance()->wizardEnableWebfinger()) {
            return _context->accountBuilder().legacyWebFingerServerUrl();
        } else {
            return _context->accountBuilder().serverUrl();
        }
    }();

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

    QUrl testUrl(userProvidedUrl); // this really is not useful.
    QString scheme = testUrl.scheme();
    if (scheme.isEmpty()) {
        qInfo(lcSetupWizardServerUrlState) << "no URL scheme provided, prepending default URL scheme" << defaultUrlSchemeC;
        userProvidedUrl.prepend(QStringLiteral("https://"));
        testUrl.setScheme(defaultUrlSchemeC); // so far I don't see this coming back as invalid even if it should be but it seems reasonable to set it anyway.
        // we catch it later using a new url after reconstructing it with the default scheme added
    }
    if (!testUrl.isValid()) // let the caller figure out how to report this
        return testUrl;

    // we can't recycle the testUrl because for reasons completely unknown to me, if you try to adjust it or set the path it destroys
    // the host so we get https:///kwdav/ every time, even if the URL was completely valid from the start
    QUrl finalUrl(userProvidedUrl);

    if (finalUrl.isValid()) {
        finalUrl = finalUrl.adjusted(QUrl::RemoveUserInfo | QUrl::RemoveQuery);
        const QString serverPathOverride = Theme::instance()->overrideServerPath();
        if (!serverPathOverride.isEmpty()) {
            finalUrl.setPath(serverPathOverride);
            testUrl.setPath(serverPathOverride);
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

    // (ab)use the account builder as temporary storage for the URL we are about to probe (after sanitation)
    // in case of errors, the user can just edit the previous value

    // this only works if the url is valid but not found. if it's invalid the original query text is wiped out and I don't
    // see an easy way to put it back because...erm...it wants to take the text from a url but it's invalid. I'll check the gui
    // to see if we can just not delete the orig text if the account builder has an invalid url.
    _context->accountBuilder().setServerUrl(serverUrl, DetermineAuthTypeJob::AuthType::Unknown);

    // TODO: perform some better validation - indeed, we should be using a regex as QUrl SUCKS wrt identifying bad formatting
    if (!serverUrl.isValid()) {
        Q_EMIT evaluationFailed(tr("Invalid server URL: %1").arg(serverUrl.errorString()));
        return;
    }

    //  _context->accountBuilder().setServerUrl(serverUrl, DetermineAuthTypeJob::AuthType::Unknown);

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
    DiscoverWebFingerServiceAdapter webfingerServiceAdapter(_context->accessManager(), serverUrl, nullptr);
    const DiscoverWebFingerServiceResult webfingerServiceResult = webfingerServiceAdapter.getResult();

    // in case any kind of error occurs, we assume the WebFinger service is not available
    if (!webfingerServiceResult.success()) {
        // first, we must resolve the actual server URL
        ResolveUrlAdapter urlResolver(_context->accessManager(), serverUrl, nullptr);
        const ResolveUrlResult result = urlResolver.getResult();

        if (!result.success()) {
            Q_EMIT evaluationFailed(result.error);
            return;
        }

        if (result.resolvedUrl.hasQuery()) {
            QString errorMsg = tr("The requested URL failed with query value: %1").arg(result.resolvedUrl.query());
            Q_EMIT evaluationFailed(errorMsg);
            return;
        }

        if (!result.acceptedCertificates.isEmpty()) {
            // future requests made through this access manager should accept the certificate
            _context->accessManager()->addCustomTrustedCaCertificates(result.acceptedCertificates);

            // the account maintains a list, too, which is also saved in the config file
            for (const auto &cert : result.acceptedCertificates)
                _context->accountBuilder().addCustomTrustedCaCertificate(cert);
        }

        // classic WebFinger workflow: auth type determination is delegated to whatever server the WebFinger service points us to in a dedicated
        // step we can skip it here therefore
        if (Theme::instance()->wizardEnableWebfinger()) {
            _context->accountBuilder().setLegacyWebFingerServerUrl(result.resolvedUrl);
            Q_EMIT evaluationSuccessful();
            return;
        }

        // next, we need to find out which kind of authentication page we have to present to the user
        auto authTypeJob = DetermineAuthTypeJobFactory(_context->accessManager()).startJob(result.resolvedUrl, this);
        QUrl url = result.resolvedUrl;
        connect(authTypeJob, &CoreJob::finished, authTypeJob, [this, authTypeJob, url]() {
            if (authTypeJob->result().isNull()) {
                Q_EMIT evaluationFailed(authTypeJob->errorMessage());
                return;
            }

            _context->accountBuilder().setServerUrl(url, qvariant_cast<DetermineAuthTypeJob::AuthType>(authTypeJob->result()));
            Q_EMIT evaluationSuccessful();
        });


    } else {
        _context->accountBuilder().setWebFingerAuthenticationServerUrl(QUrl(webfingerServiceResult.href));
        Q_EMIT evaluationSuccessful();
    }
}

} // OCC::Wizard
