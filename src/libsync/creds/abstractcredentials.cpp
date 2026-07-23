/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QLoggingCategory>

#include "account.h"
#include "common/asserts.h"
#include "creds/abstractcredentials.h"

namespace OCC {

Q_LOGGING_CATEGORY(lcCredentials, "sync.credentials", QtInfoMsg)

AbstractCredentials::AbstractCredentials(Account *account, QObject *parent)
    : QObject(parent)
    , _account(account)
    , _wasEverFetched(false)
{
}


} // namespace OCC
