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
    explicit UrlPageController(AccessManager *accessManager, QObject *parent = nullptr);
    // we call this when the user presses the next button. not sure yet how to communicate the result - the signals below are probably
    // going away but let's see.
    void evaluate();
    // ideally we should have a QRegExValidator on the url line edit and only when that passes, we can set the page isComplete to true
    // which will enable the next button. the url eval may still fail but "next" should not be enabled on an obviously bogus url

Q_SIGNALS:
    // eh - this probably won't work but let's see.
    void success(const QUrl &url, bool urlIsWebfinger, QSet<QSslCertificate> trustedCertificates);
    void failure(const QString &errorMsg);

private:
    AccessManager *_accessManager;
};
