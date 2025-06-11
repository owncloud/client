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

class QWizardPage;

namespace OCC {

class AccessManager;

/**
 * @brief The UrlPageController class is responsible for doing the basic validation of an authentication url.
 * the url may come from the user, or it may be pre-defined via the theme.
 *
 */
class UrlPageController : public QObject
{
    Q_OBJECT

public:
    explicit UrlPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent = nullptr);
    // we call this when the user presses the next button. not sure yet how to communicate the result - the signals below are probably
    // going away but let's see.
    bool validate();
    // ideally we should have a QRegExValidator on the url line edit and only when that passes,
    // the wizard will enable the next button. The url evaluation may still fail but "next" should not be enabled on an
    // obviously bogus url

Q_SIGNALS:
    // eh - this probably won't work
    // the "normal" way to do it would be to create a model for this controller that contains these values
    // and let the main controller create it/pass it so it has direct access to the results. We should not use the main
    // controller model imo since we don't want to accept the values until all have been successfully obtained.
    void success(const QUrl &url, bool urlIsWebfinger, QSet<QSslCertificate> trustedCertificates);

private:
    QWizardPage *_page;
    AccessManager *_accessManager;

    // need to factor in any url val that comes from the theme and set the url to that
    // consider: make the url field read only if the url comes from the theme, and make sure "next" is enabled.
    // docs say that the QWizard checks changes/diffs to mandatory fields since initializePage was called so it's probably better
    // to buildPage then set the url value in separate step
    void buildPage();
};

}
