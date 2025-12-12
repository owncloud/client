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

#include "spacesmanager.h"

#include "libsync/account.h"
#include "libsync/creds/abstractcredentials.h"
#include "libsync/graphapi/jobs/drives.h"


#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

using namespace OCC;
using namespace GraphApi;

namespace {
constexpr auto refreshTimeoutC = 30s;
}

SpacesManager::SpacesManager(Account *parent)
    : QObject(parent)
    , _account(parent)
    , _refreshTimer(new QTimer(this))
{
    connect(_account, &Account::credentialsFetched, this, &SpacesManager::refresh);

    _refreshTimer->setInterval(refreshTimeoutC);
    // the timer will be restarted once we received drives data
    _refreshTimer->setSingleShot(true);
    connect(_refreshTimer, &QTimer::timeout, this, &SpacesManager::refresh);
}

void SpacesManager::refresh()
{
    if (!_account || !_account->accessManager()) {
        return;
    }
    if (!_account->credentials()->ready()) {
        return;
    }

    // TODO: leak the job until we fixed the ownership https://github.com/owncloud/client/issues/11203
    auto drivesJob = new Drives(_account, nullptr);
    drivesJob->setTimeout(refreshTimeoutC);
    connect(drivesJob, &Drives::finishedSignal, this, [drivesJob, this] {
        drivesJob->deleteLater();

        // a system which provides multiple personal spaces the name of the drive is always used as display name
        auto hasManyPersonalSpaces = _account->capabilities().spacesSupport().hasMultiplePersonalSpaces;
        QList<Space *> newSpaces;
        QList<QString> deletedSpaces;

        if (drivesJob->httpStatusCode() == 200) {
            QList<QString> oldKeys = _spaces.keys();
            for (const auto &dr : drivesJob->drives()) {
                bool driveDisabled = dr.getRoot().getDeleted().getState() == QLatin1String("trashed");
                auto *space = _spaces.value(dr.getId(), nullptr);
                if (space && !driveDisabled) {
                    // we need to treat any newly disabled spaces as if they were deleted so leave it
                    // in the old key list for removal, below
                    oldKeys.removeOne(dr.getId());
                    bool changed = space->setDrive(dr);
                    if (changed)
                        emit spaceChanged(space);
                }
                // likewise, don't add newly discovered space if it's disabled
                else if (!space && !driveDisabled) {
                    space = new Space(this, dr, hasManyPersonalSpaces);
                    _spaces.insert(dr.getId(), space);
                    emit spaceAdded(_account->uuid(), space);
                    newSpaces.append(space);
                }
            }
            for (const QString &id : std::as_const(oldKeys)) {
                auto *oldSpace = _spaces.take(id);
                if (oldSpace) {
                    emit spaceAboutToBeRemoved(_account->uuid(), oldSpace);
                    deletedSpaces.append(id);
                    oldSpace->deleteLater();
                }
            }
            if (!_ready) {
                _ready = true;
                Q_EMIT ready();
            }
        }
        if (!newSpaces.isEmpty())
            emit spacesAdded(_account->uuid(), newSpaces, _spaces.count());
        if (!deletedSpaces.isEmpty())
            emit spacesRemoved(_account->uuid(), deletedSpaces, _spaces.count());
        // todo: remove this once the old accountSettings are gone
        Q_EMIT updated(_account);
        _refreshTimer->start();
    });
    _refreshTimer->stop();
    drivesJob->start();
}

Space *SpacesManager::space(const QString &id) const
{
    if (id.isEmpty())
        return nullptr;
    return _spaces.value(id, nullptr);
}

Account *SpacesManager::account() const
{
    return _account;
}

QVector<Space *> SpacesManager::spaces() const
{
    return {_spaces.begin(), _spaces.end()};
}
