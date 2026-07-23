/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
