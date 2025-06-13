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

#include <QObject>
#include <QSslCertificate>

#include "wizardpagevalidator.h"

class QWizardPage;
class QLineEdit;
class QLabel;

namespace OCC {

class AccessManager;

/**
 * @brief The UrlPageController class is responsible for doing the basic validation of an authentication url, and it
 * also determines whether a webfinger service is available.
 * the url may come from the user, or it may be pre-defined via the theme.
 *
 */
class UrlPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT

public:
    /**
     * @brief UrlPageController
     * @param page may be null
     * @param accessManager is required to run network jobs to complete the validation
     * @param parent should normally be the parent controller when on the heap
     */
    explicit UrlPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent);

    // the wizard will call this when the user presses the next button. so far this method will also signal success() to the main controller
    // and pass the values collected/verified but that may be replaced with a dedicated model down the road.
    bool validate() override;

    // ideally we should have a QRegExValidator on the url line edit and only when that passes,
    // the wizard will enable the next button. The url evaluation may still fail but "next" should not be enabled on an
    // obviously bogus url. this is future as a final polishing step.

Q_SIGNALS:
    // eh - this probably won't work
    // the "normal" way to do it would be to create a model for this controller that contains these values
    // and let the main controller create it/pass it so it has direct access to the results. We should not use the main
    // controller model imo since we don't want to accept the values until all have been successfully obtained.
    // a dedicated model will also make testing easier. this is tbd once we have some basic functional impl for the wizard
    // and first page.
    /**
     * @brief success is emitted when the validation has passed without errors
     * @param serverUrl is the "original" server url (https://somehost/<optional path> that appears in the gui.
     * @param webfingerUrl is the url for a webfinger service discovered during validation. If this is empty, no webfinger service is
     * available
     * @param trustedCertificates holds any certificates the user accepted in the course of validation. This will only be non-empty in
     * exceptional cases (eg with very poorly configured hosts that do not provide legit, signed certs)
     *
     * if OAuth is not supported by the target server the whole validation will fail.
     */
    void success(const QUrl &serverUrl, const QUrl &webfingerUrl, QSet<QSslCertificate> trustedCertificates);

private:
    QWizardPage *_page;
    AccessManager *_accessManager;

    QLineEdit *_urlField;
    QLabel *_errorField;

    // need to factor in any url val that comes from the theme and set the url to that
    // consider: make the url field read only if the url comes from the theme, and make sure "next" is enabled.
    // docs say that the QWizard checks changes/diffs to mandatory fields since initializePage was called so it's probably better
    // to buildPage then set the url value in separate step
    /** sets up the wizard page with appropriate content and connect any signals to eg the url QLineEdit */
    void buildPage();
    void setUrl(const QString &urlText);
    void setErrorMessage(const QString &error);
    QUrl checkUrl();
};
}
