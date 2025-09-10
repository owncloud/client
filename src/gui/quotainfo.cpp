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

#include "quotainfo.h"
#include "account.h"
#include "networkjobs.h"
#include "folderman.h"
#include "creds/abstractcredentials.h"
#include <theme.h>

#include <QTimer>

using namespace std::chrono_literals;

namespace OCC {

namespace {
    const auto defaultIntervalT = 30s;
    const auto failIntervalT = 5s;
}

QuotaInfo::QuotaInfo(AccountState *parent)
    : QObject(parent)
    , _accountState(parent)
    , _lastQuotaTotalBytes(0)
    , _lastQuotaUsedBytes(0)
    , _active(false)
{
    connect(parent, &AccountState::stateChanged, this, &QuotaInfo::slotAccountStateChanged);
    connect(&_jobRestartTimer, &QTimer::timeout, this, &QuotaInfo::slotCheckQuota);
    _jobRestartTimer.setSingleShot(true);
}

void QuotaInfo::setActive(bool active)
{
    _active = active;
    slotAccountStateChanged();
}


void QuotaInfo::slotAccountStateChanged()
{
    if (canGetQuota()) {
        const auto elapsed = std::chrono::seconds(_lastQuotaRecieved.secsTo(QDateTime::currentDateTime()));
        if (_lastQuotaRecieved.isNull() || elapsed >= defaultIntervalT) {
            slotCheckQuota();
        } else {
            _jobRestartTimer.start(defaultIntervalT - elapsed);
        }
    } else {
        _jobRestartTimer.stop();
    }
}

void QuotaInfo::slotRequestFailed()
{
    _lastQuotaTotalBytes = 0;
    _lastQuotaUsedBytes = 0;
    _jobRestartTimer.start(failIntervalT);
}

bool QuotaInfo::canGetQuota() const
{
        return false;
}

QString QuotaInfo::quotaBaseFolder() const
{
    return Theme::instance()->quotaBaseFolder();
}

void QuotaInfo::slotCheckQuota()
{
    if (!canGetQuota()) {
        return;
    }

    if (_job) {
        // The previous job was not finished?  Then we cancel it!
        _job->deleteLater();
    }

    const AccountPtr &account = _accountState->account();
    _job = new PropfindJob(account, account->davUrl(), quotaBaseFolder(), PropfindJob::Depth::Zero, this);
    _job->setProperties({ QByteArrayLiteral("quota-available-bytes"), QByteArrayLiteral("quota-used-bytes") });
    connect(_job.data(), &PropfindJob::directoryListingIterated, this, &QuotaInfo::slotUpdateLastQuota);
    connect(_job.data(), &AbstractNetworkJob::networkError, this, &QuotaInfo::slotRequestFailed);
    _job->start();
}

void QuotaInfo::slotUpdateLastQuota(const QString &, const QMap<QString, QString> &result)
{
    // The server can return fractional bytes (#1374)
    // <d:quota-available-bytes>1374532061.2</d:quota-available-bytes>
    qint64 avail = result[QStringLiteral("quota-available-bytes")].toDouble();
    _lastQuotaUsedBytes = result[QStringLiteral("quota-used-bytes")].toDouble();
    // negative value of the available quota have special meaning (#3940)
    _lastQuotaTotalBytes = avail >= 0 ? _lastQuotaUsedBytes + avail : avail;
    Q_EMIT quotaUpdated(_lastQuotaTotalBytes, _lastQuotaUsedBytes);
    _jobRestartTimer.start(defaultIntervalT);
    _lastQuotaRecieved = QDateTime::currentDateTime();
}
}
