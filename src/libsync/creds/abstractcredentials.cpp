/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
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
