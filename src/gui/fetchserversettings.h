/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "account.h"
#include <QObject>
#include <QPointer>

namespace OCC {
class Capabilities;

class FetchServerSettingsJob : public QObject
{
    Q_OBJECT
public:
    enum class Result { Success, TimeOut, InvalidCredentials, UnsupportedServer, Undefined };
    Q_ENUM(Result);
    FetchServerSettingsJob(Account *account, QObject *parent);

    void start();

Q_SIGNALS:
    void finishedSignal(Result);

private:
    void runAsyncUpdates();

    // returns whether the started jobs should be excluded from the retry queue
    bool isAuthJob() const;

    QPointer<Account> _account;
};

}
