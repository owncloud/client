/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#include "owncloudlib.h"
#include "common/filesystembase.h"
#include "common/vfs.h"

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

    /** If remotely deleted files are needed to move to trash */
    bool _moveFilesToTrash = false;


    /** The maximum number of active jobs in parallel  */
    int _parallelNetworkJobs = 6;

    /** Reads settings from env vars where available. */
    void fillFromEnvironmentVariables();

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
