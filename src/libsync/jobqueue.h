/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "owncloudlib.h"

#include <QPointer>
#include <vector>

namespace OCC {

class AbstractNetworkJob;
class Account;

class OWNCLOUDSYNC_EXPORT JobQueue
{
public:
    JobQueue(Account *account);

    /**
     * whether jobs need to be enqued
     */
    bool isBlocked() const;

    /**
     * Retry a job if the job allows it,
     * if blocked the job will be queued untill we are unblocked
     * Returns whether the job will be retired
     */
    bool retry(AbstractNetworkJob *job);
    /**
     * Enque if blocked
     * Returns whether the job was enqueued
     */
    bool enqueue(AbstractNetworkJob *job);

    size_t size() const;

    /**
     * Clear the queue and abort all jobs
     */
    void clear();

private:
    void block();
    void unblock();

    Account *_account;
    uint _blocked = 0;
    std::vector<QPointer<AbstractNetworkJob>> _jobs;

    friend class JobQueueGuard;
};

class OWNCLOUDSYNC_EXPORT JobQueueGuard
{
public:
    JobQueueGuard(JobQueue *queue);
    ~JobQueueGuard();

    bool block();
    bool unblock();
    bool clear();

    JobQueue *queue() const;

private:
    JobQueue *_queue;
    bool _blocked = false;
};
}
