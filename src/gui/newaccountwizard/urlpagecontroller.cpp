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
#include "urlpagecontroller.h"

#include "accessmanager.h"
#include "networkadapters/determineauthtypeadapter.h"
#include "networkadapters/discoverwebfingerserviceadapter.h"
#include "networkadapters/resolveurladapter.h"
#include "theme.h"

#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWizardPage>

namespace OCC {

UrlPageController::UrlPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent)
    : QObject(parent)
    , _page(page)
    , _accessManager(accessManager)
{
    buildPage();

    QString themeUrl = Theme::instance()->overrideServerUrlV2();
    if (_urlField && !themeUrl.isEmpty()) {
        setUrl(themeUrl);
        // I think this is the right thing to do: if the theme provides the url, don't let the user change it!
        _urlField->setEnabled(false);
    }
}

void UrlPageController::buildPage()
{
    if (!_page)
        return;

    QString appName = Theme::instance()->appNameGUI();

    QLabel *logoLabel = new QLabel({}, _page);
    logoLabel->setPixmap(Theme::instance()->wizardHeaderLogo().pixmap(200, 100));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logoLabel->setAccessibleName(tr("%1 logo").arg(appName));

    QLabel *welcomeLabel = new QLabel(tr("Welcome to %1").arg(appName), _page);
    QFont welcomeFont = welcomeLabel->font();
    welcomeFont.setWeight(QFont::DemiBold);
    welcomeFont.setPixelSize(20);
    welcomeLabel->setFont(welcomeFont);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *instructionLabel = new QLabel(tr("Enter your server address to get started. Your web browser will be opened to complete sign in."), _page);
    QFont font = instructionLabel->font();
    font.setPixelSize(14);
    instructionLabel->setFont(font);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    _urlField = new QLineEdit(_page);
    _urlField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _urlField->setPlaceholderText(Theme::instance()->wizardUrlPlaceholder());

    _errorField = new QLabel(QStringLiteral("errors live here"), _page);
    QPalette errorPalette = _errorField->palette();
    errorPalette.setColor(QPalette::Text, Qt::red);
    _errorField->setPalette(errorPalette);
    _errorField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _errorField->setWordWrap(true);
    _errorField->setAlignment(Qt::AlignLeft);

    QLabel *footerLogoLabel = nullptr;
    if (!Theme::instance()->wizardFooterLogo().isNull()) {
        footerLogoLabel = new QLabel({}, _page);
        footerLogoLabel->setPixmap(Theme::instance()->wizardFooterLogo().pixmap(100, 52));
        footerLogoLabel->setAlignment(Qt::AlignCenter);
        footerLogoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // logoLabel->setAccessibleName(tr("%1 logo").arg(appName));
    }

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 0, 50, 0);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(logoLabel, Qt::AlignCenter);
    layout->addSpacing(16);
    layout->addWidget(welcomeLabel, Qt::AlignCenter);
    layout->addWidget(instructionLabel, Qt::AlignCenter);
    layout->addWidget(_urlField, Qt::AlignCenter);
    layout->addWidget(_errorField, Qt::AlignLeft);
    if (footerLogoLabel)
        layout->addWidget(footerLogoLabel, Qt::AlignCenter);
    layout->addStretch(1);
    _page->setLayout(layout);
}

void UrlPageController::setUrl(const QString &urlText)
{
    QSignalBlocker blocker(_urlField);
    _urlField->setText(urlText);
}

void UrlPageController::handleError(const QString &error)
{
    _errorField->setText(error);
    _results.error = error;
    Q_EMIT failure(_results);
}

QUrl UrlPageController::checkUrl()
{
    if (!_accessManager)
        return QUrl();

    QString userUrl = _urlField->text();

    // we need to check the scheme to be sure it's not there, but using the old "string matching" technique was not great
    // because it added https:// to urls that had valid schemes but which did not match http or https. That was more complicating
    // than useful so now we only add https if there really is no valid scheme to start with.
    // the caller should decide which schemes are allowed and take whatever action it sees fit

    // strict mode really does not appear to be useful wrt catching malformed urls but hey, why not.
    QUrl testUrl(userUrl, QUrl::StrictMode);
    QString scheme = testUrl.scheme();
    if (scheme.isEmpty()) {
        userUrl.prepend(QStringLiteral("https://"));
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
    QUrl finalUrl(userUrl);

    if (finalUrl.isValid()) {
        finalUrl = finalUrl.adjusted(QUrl::RemoveUserInfo | QUrl::RemoveQuery);
        const QString serverPathOverride = Theme::instance()->overrideServerPath();
        if (!serverPathOverride.isEmpty()) {
            finalUrl.setPath(serverPathOverride);
        }
    }
    return finalUrl;
}

bool UrlPageController::validate()
{
    _results = {};

    if (!_accessManager) {
        handleError(QStringLiteral("No valid access manager is available"));
        return false;
    }

    // always clear the access manager data before revalidating the url
    _accessManager->reset();

    QUrl givenUrl = checkUrl();
    if (!givenUrl.isValid()) {
        QString fullError = givenUrl.errorString();
        QStringList parts = fullError.split(QStringLiteral(";"), Qt::SkipEmptyParts);
        // it might be too much to print both the error and the source string - eval with input from others.
        handleError(tr("Invalid server URL: %1").arg(parts[0] + parts[1]));
        return false;
    } else if (givenUrl.scheme().isEmpty() || givenUrl.scheme() != QStringLiteral("https")) // scheme should not be empty but who knows
    {
        handleError(tr("Invalid URL scheme. Only https is accepted."));
        return false;
    }

    setUrl(givenUrl.toDisplayString());

    DiscoverWebFingerServiceAdapter webfingerServiceAdapter(_accessManager, givenUrl);
    const DiscoverWebFingerServiceResult webfingerServiceResult = webfingerServiceAdapter.getResult();

    // in case any kind of error occurs, we assume the WebFinger service is not available
    if (webfingerServiceResult.success()) {
        _results.baseServerUrl = givenUrl;
        _results.webfingerServiceUrl = QUrl(webfingerServiceResult.href);
    } else {
        // first, we must resolve the actual server URL
        ResolveUrlAdapter urlResolver(_accessManager, givenUrl);
        const ResolveUrlResult resolveUrlResult = urlResolver.getResult();

        if (!resolveUrlResult.success()) {
            handleError(resolveUrlResult.error);
            return false;
        }

        const QUrl finalUrl = resolveUrlResult.resolvedUrl;
        // I don't know how likely it is that the resolved url will contain a query but from other code it is
        // evident that url queries are banned. so if it comes back with a query on it, it's a nogo
        if (finalUrl.hasQuery()) {
            QString errorMsg = tr("The requested URL failed with query value: %1").arg(finalUrl.query());
            handleError(errorMsg);
            return false;
        }

        if (!resolveUrlResult.acceptedCertificates.isEmpty()) {
            // future requests made through this access manager need to include any certificates
            _accessManager->addCustomTrustedCaCertificates(resolveUrlResult.acceptedCertificates);
            // save/return this also for the account setup, as the account maintains the set of certificates, too.
            _results.certificates = resolveUrlResult.acceptedCertificates;
        }

        // This is now a mere formality to be very very very sure that the base url uses oauth. In the unlikely event that it doesn't
        // the job simply fails.
        DetermineAuthTypeAdapter authTypeAdapter(_accessManager, finalUrl);
        const DetermineAuthTypeResult authResult = authTypeAdapter.getResult();
        if (!authResult.success()) {
            handleError(authResult.error);
            return false;
        }

        Q_ASSERT(authResult.type == AuthenticationType::OAuth);
        _results.baseServerUrl = finalUrl;
    }

    Q_EMIT success(_results);
    return true;
}

}
