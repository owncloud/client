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
#include <QPointer>
#include <QSslCertificate>
#include <QUrl>

#include "wizardpagevalidator.h"

class QWizardPage;
class QLineEdit;
class QLabel;

namespace OCC {

/**
 * @brief The UrlPageResults class contains all the possible values this controller can collect.
 *
 *   baseServerUrl is the "original" server url (https://somehost/<optional path> that appears in the gui either via user input or
 *   provided by the theme
 *   webfingerServiceUrl is the url for a webfinger service discovered during validation. If this is empty, no webfinger service is
 *   available
 *   trustedCertificates holds any certificates the user accepted in the course of validation. This will only be non-empty in
 *   exceptional cases (eg with very poorly configured hosts that do not provide legit, signed certs)
 *   error holds a beautified error message. This may go away as I don't think the owner of the controller needs to do anything with it.
 */
struct UrlPageResults
{
    QUrl baseServerUrl;
    QUrl webfingerServiceUrl;
    QSet<QSslCertificate> certificates;

    QString error;
};

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

    /**
     * @brief validate evaluates the given url, performs various checks, and signals the UrlPageResults to interested parties
     * @return true of the validation succeeded, false if it failed
     */
    bool validate() override;

    // ideally we should have a QRegExValidator on the url line edit and only when that passes,
    // the wizard will enable the next button. The url evaluation may still fail but "next" should not be enabled on an
    // obviously bogus url. this is future as a final polishing step.

Q_SIGNALS:

    /**
     * @brief success is emitted when the validation has succeeded.
     * note we do not track the authentication method anymore since there is only one. If OAuth is not supported by the target server the
     * whole validation will fail.
     */
    void success(const OCC::UrlPageResults &);
    /**
     * @brief failure is emitted when a validation error has occurred. Some of the values in the result may be valid, depending on
     * how far the validation got before it failed.
     */
    void failure(const OCC::UrlPageResults &);

private:
    QPointer<QWizardPage> _page;
    QPointer<AccessManager> _accessManager;

    QLabel *_instructionLabel = nullptr;
    QLineEdit *_urlField = nullptr;
    QLabel *_errorField = nullptr;

    UrlPageResults _results;
    bool _urlValidated = false;

    /** sets up the wizard page with appropriate content and connect any signals to eg the url QLineEdit */
    void buildPage();
    /** sets the url value in the line edit */
    void setUrl(const QString &urlText);
    /** displays the error in the page, sets the error value in the results and emits failure(error) */
    void handleError(const QString &error);
    /** performs some rudimentary checks on the url format, adds https:// if there is no scheme provided
     *  we should replace this with a QValidator that uses a more stringent regex
     *  the validator::fixup method can be used to add the https scheme
     */
    QUrl checkUrl();
};
}
Q_DECLARE_METATYPE(OCC::UrlPageResults)
// this type id is throwaway, we just use it to ensure we declare the meta type only ONCE
// also this is only required to use the type cross thread, or with QSignalSpy during testing
static const int urlPageResultsTypeId = qRegisterMetaType<OCC::UrlPageResults>();
