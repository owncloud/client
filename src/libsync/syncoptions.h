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
#include "common/filesystembase.h"
#include "common/vfs.h"

#include <QPointer>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QString>

#include <chrono>


namespace OCC {

/**
 * Value class containing the options given to the sync engine
 */
class OWNCLOUDSYNC_EXPORT SyncOptions
{
public:
    // the VFS pointer must be stored in a QPointer as it may go out of scope from "above"
    // it's owned by the Folder so if the folder dies, so does the vfs pointer.
    // use isValid to see if the options should still be used, or test the vfs pointer directly to see if it's null or not.
    explicit SyncOptions(Vfs *vfs = nullptr);
    ~SyncOptions();

    Vfs *vfs() const { return _vfs; }

    /** The maximum number of active jobs in parallel  */
    int _parallelNetworkJobs = 6;

    /**
     * @brief isValid indicates if the options are complete
     * @return true if vfs is non-null, else false
     */
    bool isValid() const;

private:
    /** Create a virtual file for new files instead of downloading. If vfs is null, isValid will return false indicating you should not use it
     * implementation note: of course you can pass a nullptr to the ctr or it can be deleted from above so this is the "sanest" way to
     *  handle it.
     */
    QPointer<Vfs> _vfs;
};

}
