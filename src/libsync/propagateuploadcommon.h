/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
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

#include "owncloudpropagator.h"
#include "networkjobs.h"

#include <unordered_set>

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcPropagateUpload)

/**
 * @brief The PropagateUploadFileCommon class is the code common between all chunking algorithms
 * @ingroup libsync
 *
 * State Machine:
 *
 *   +---> start()  --> (delete job) -------+
 *   |                                      |
 *   +--> slotComputeContentChecksum()  <---+
 *                   |
 *                   v
 *    slotComputeTransmissionChecksum()
 *         |
 *         v
 *    slotStartUpload()  -> doStartUpload()
 *                                  .
 *                                  .
 *                                  v
 *        finalize() or abortWithError()
 */
class PropagateUploadCommon : public PropagateItemJob
{
    Q_OBJECT

protected:
    static const QString fileChangedMessage();

    QVector<AbstractNetworkJob *> _jobs; /// network jobs that are currently in transit
    bool _finished; /// Tells that all the jobs have been finished
    bool _deleteExisting;

    /** Whether an abort is currently ongoing.
     *
     * Important to avoid duplicate aborts since each finishing PUTFileJob might
     * trigger an abort on error.
     */
    bool _aborting;

    QByteArray _transmissionChecksumHeader;

public:
    PropagateUploadCommon(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
        , _finished(false)
        , _deleteExisting(false)
        , _aborting(false)
    {
    }

    /**
     * Whether an existing entity with the same name may be deleted before
     * the upload.
     *
     * Default: false.
     */
    void setDeleteExisting(bool enabled);

    void start() override;

    bool isLikelyFinishedQuickly() override { return _item->_size < propagator()->smallFileSize(); }

private Q_SLOTS:
    void slotComputeContentChecksum();
    // Content checksum computed, compute the transmission checksum
    void slotComputeTransmissionChecksum(CheckSums::Algorithm contentChecksumType, const QByteArray &contentChecksum);
    // transmission checksum computed, prepare the upload
    void slotStartUpload(CheckSums::Algorithm transmissionChecksumType, const QByteArray &transmissionChecksum);

public:
    virtual void doStartUpload() = 0;

    void finalize();
    void abortWithError(SyncFileItem::Status status, const QString &error);

    /***
     * Add job to the list of children
     * The job is automatically removed from the children once it's done.
     * It is important that this function is called before any connects.
     */
    void addChildJob(AbstractNetworkJob *job);

    const auto &childJobs() const
    {
        return _childJobs;
    }


protected:
    void done(SyncFileItem::Status status, const QString &errorString = QString()) override;

    /**
     * Aborts all running network jobs, except for the ones that mayAbortJob
     * returns false on and, for async aborts, emits abortFinished when done.
     */
    void abortNetworkJobs(
        AbortType abortType,
        const std::function<bool(AbstractNetworkJob *job)> &mayAbortJob);

    /**
     * Checks whether the current error is one that should reset the whole
     * transfer if it happens too often. If so: Bump UploadInfo::errorCount
     * and maybe perform the reset.
     */
    void checkResettingErrors();

    /**
     * Error handling functionality that is shared between jobs.
     */
    void commonErrorHandling(AbstractNetworkJob *job);

    /**
     * Increases the timeout for the final MOVE/PUT for large files.
     *
     * This is an unfortunate workaround since the drawback is not being able to
     * detect real disconnects in a timely manner. Shall go away when the server
     * response starts coming quicker, or there is some sort of async api.
     *
     * See #6527, enterprise#2480
     */
    static void adjustLastJobTimeout(AbstractNetworkJob *job, qint64 fileSize);

    /** Bases headers that need to be sent on the PUT, or in the MOVE for chunking-ng */
    QMap<QByteArray, QByteArray> headers();

private:
    bool _quotaUpdated = false;

    std::unordered_set<AbstractNetworkJob *> _childJobs; /// network jobs that are currently in transit
};
}
