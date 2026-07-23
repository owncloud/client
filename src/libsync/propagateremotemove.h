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
 * @brief The MoveJob class
 * @ingroup libsync
 */
class MoveJob : public AbstractNetworkJob
{
    Q_OBJECT
    const QString _destination;
    HeaderMap _extraHeaders;

public:
    explicit MoveJob(
        Account *account, const QUrl &url, const QString &path, const QString &destination, const HeaderMap &_extraHeaders, QObject *parent = nullptr);

    void start() override;
    void finished() override;
};

/**
 * @brief Propagate a local move (or rename) to the server
 * @ingroup libsync
 */
class PropagateRemoteMove : public PropagateItemJob
{
    Q_OBJECT
    QPointer<MoveJob> _job;

public:
    PropagateRemoteMove(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
    {
    }
    void start() override;
    void abort(PropagatorJob::AbortType abortType) override;
    JobParallelism parallelism() override { return _item->isDirectory() ? WaitForFinished : FullParallelism; }

    /**
     * Rename the directory in the selective sync list
     */
    static bool adjustSelectiveSync(SyncJournalDb *journal, const QString &from, const QString &to);

private Q_SLOTS:
    void slotMoveJobFinished();
    void finalize();
};
}
