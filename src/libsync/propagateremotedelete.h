/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "owncloudpropagator.h"
#include "networkjobs.h"

namespace OCC {

/**
 * @brief The DeleteJob class
 * @ingroup libsync
 */
class DeleteJob : public AbstractNetworkJob
{
    Q_OBJECT
public:
    explicit DeleteJob(Account *account, const QUrl &url, const QString &path, QObject *parent = nullptr);

    void start() override;
    void finished() override;
};

/**
 * @brief Propagate a local delete to the server
 * @ingroup libsync
 */
class PropagateRemoteDelete : public PropagateItemJob
{
    Q_OBJECT
    QPointer<DeleteJob> _job;

public:
    PropagateRemoteDelete(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
    {
    }
    void start() override;
    void abort(PropagatorJob::AbortType abortType) override;

    bool isLikelyFinishedQuickly() override { return !_item->isDirectory(); }

private Q_SLOTS:
    void slotDeleteJobFinished();
};
}
