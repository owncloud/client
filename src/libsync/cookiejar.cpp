/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cookiejar.h"

// todo: #31

#include <QLoggingCategory>
#include <QNetworkCookie>

namespace OCC {

Q_LOGGING_CATEGORY(lcCookieJar, "sync.cookiejar", QtInfoMsg)

CookieJar::CookieJar(QObject *parent)
    : QNetworkCookieJar(parent)
{
}

CookieJar::~CookieJar()
{
}

CookieJar *CookieJar::clone(QObject *parent)
{
    auto newJar = new CookieJar(parent);
    newJar->setAllCookies(allCookies());
    return newJar;
}

} // namespace OCC
