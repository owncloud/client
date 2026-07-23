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
 * @brief Propagate a local mkdir to the server
 * @ingroup libsync
 */
class PropagateRemoteMkdir : public PropagateItemJob
{
    Q_OBJECT
    QPointer<AbstractNetworkJob> _job;
    bool _deleteExisting;
    friend class PropagateDirectory; // So it can access the _item;
public:
    PropagateRemoteMkdir(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
        , _deleteExisting(false)
    {
    }
    void start() override;
    void abort(PropagatorJob::AbortType abortType) override;

    // Creating a directory should be fast.
    bool isLikelyFinishedQuickly() override { return true; }

    /**
     * Whether an existing entity with the same name may be deleted before
     * creating the directory.
     *
     * Default: false.
     */
    void setDeleteExisting(bool enabled);

private Q_SLOTS:
    void slotStartMkcolJob();
    void slotMkcolJobFinished();
    void success();

private:
#ifdef Q_OS_WIN
    void retryUpdateMetadata(std::chrono::steady_clock::time_point start, SyncFileItemPtr item);

#endif
};
}
