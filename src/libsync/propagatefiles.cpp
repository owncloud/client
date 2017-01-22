/*
 * Copyright (C) by Piotr Mrowczynski <piotr@owncloud.com>
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

#include "owncloudpropagator.h"

namespace OCC {


qint64 PropagateFiles::committedDiskSpace() const
{
    qint64 needed = 0;
    foreach (PropagatorJob* job, _subJobs) {
        needed += job->committedDiskSpace();
    }
    return needed;
}

void PropagateFiles::append(const SyncFileItemPtr &item)
{
    _totalItems++;
    if(item->_size <= propagator()->smallFileSize()){
        _syncDBItems.append(item);
    } else {
        _syncDataItems.append(item);
    }
}

bool PropagateFiles::scheduleNextJob()
{
    if (_state == Finished) {
        return false;
    }

    if (_state == NotYetStarted) {
        _state = Running;

        if (isEmpty()) {
            finalize();
            return true;
        }
    }

    if (_state == Running) {
        // This will ensure that all other jobs in the earlier PropagateDirectory are finished, so we can start with data transfers
        if (propagator()->runningAtRootJob() != 1){
            return false;
        }
    }

    // cache the value of first unfinished subjob
    int i = _firstUnfinishedSubJob;
    int subJobsCount = _subJobs.count();
    while (i < subJobsCount && _subJobs.at(i)->_state == Finished) {
      _firstUnfinishedSubJob = ++i;
    }
    for (int i = _firstUnfinishedSubJob; i < subJobsCount; ++i) {
        if (_subJobs.at(i)->_state == Finished) {
            continue;
        }

        if (possiblyRunNextJob(_subJobs.at(i))) {
            return true;
        }

        Q_ASSERT(_subJobs.at(i)->_state == Running);
    }

    return scheduleNextItem();
}

bool PropagateFiles::scheduleNewJob(QVector<SyncFileItemPtr> &syncJobs){
    // This function is used to schedule new job and lazily create job from sync items
    Q_ASSERT(!syncJobs.isEmpty());

    // Equivalent to Qt5 takeFirst()
    const SyncFileItemPtr &item = syncJobs.first();
    PropagateItemJob* job = propagator()->createJob(item);
    _subJobs.append(job);
    syncJobs.pop_front();
    return possiblyRunNextJob(job);
}

bool PropagateFiles::scheduleNextItem()
{
    /// This function holds the whole bookkeeping/data-transfers balance logic

    bool syncDBItemsEmpty = _syncDBItems.isEmpty();
    bool syncDataItemsEmpty = _syncDataItems.isEmpty();

    if (syncDBItemsEmpty && !syncDataItemsEmpty){
        // There are no more DB jobs, ensure to maximally parallelise Data Transfers now
        return scheduleNewJob(_syncDataItems);
    } else if (!syncDBItemsEmpty && syncDataItemsEmpty){
        // There are no more data transfer jobs, ensure to maximally parallelise DB jobs now
        return scheduleNewJob(_syncDBItems);
    } else if (!syncDBItemsEmpty && !syncDataItemsEmpty){
        // Both queues have items, ensure bookkeeping and data transfer balance
        if (_activeDataJobsNow < 2){
            // By default, we have max 3 connections available for bigger files
            // On the other hand, we have max 6 for isLikelyToFinishQuickly files (also small files)
            // It makes sense to use 2 queues for bigger data transfers, and leave the remaining 1-4 for faster operations
            return scheduleNewJob(_syncDataItems);
        } else {
            return scheduleNewJob(_syncDBItems);
        }
    }

    // This means that we have no more file-items to sync -> finish
    return false;
}

void PropagateFiles::slotSubJobFinished(SyncFileItem::Status status)
{
    if (status == SyncFileItem::FatalError) {
        abort();
        _state = Finished;
        emit finished(status);
        return;
    } else if (status == SyncFileItem::NormalError || status == SyncFileItem::SoftError) {
        _hasError = status;
    }

    PropagateItemJob *job = qobject_cast<PropagateItemJob *>(sender());
    if(job->_item->_size <= propagator()->smallFileSize()){
        _activeDBJobsNow--;
    } else {
        _activeDataJobsNow--;
    }
    Q_ASSERT(job);

    _jobsFinished++;

    // We finished processing all the jobs
    // check if we finished
    if (_jobsFinished >= _totalItems) {
        Q_ASSERT(!_activeDBJobsNow && !_activeDataJobsNow); // how can we be finished if there are still jobs running now
        finalize();
    } else {
        emit ready();
    }
}

void PropagateFiles::finalize()
{
    _state = Finished;
    emit finished(_hasError == SyncFileItem::NoStatus ?  SyncFileItem::Success : _hasError);
}

}
