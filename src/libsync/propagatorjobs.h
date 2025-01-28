/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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
#include <QFile>

namespace OCC {

/**
 * Tags for checksum header.
 * It's here for being shared between Upload- and Download Job
 */
static const char checkSumHeaderC[] = "OC-Checksum";
static const char contentMd5HeaderC[] = "Content-MD5";

/**
 * @brief Remove (or move to the trash) a file or folder that was removed remotely
 * @ingroup libsync
 */
class PropagateLocalRemove : public PropagateItemJob
{
    Q_OBJECT
public:
    PropagateLocalRemove(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
    {
    }
    void start() override;

private:
    bool removeRecursively(const QString &absolute);
    bool _moveToTrash;
};

/**
 * @brief Make a local directory after discovering it on the server
 * @ingroup libsync
 */
class PropagateLocalMkdir : public PropagateItemJob
{
    Q_OBJECT
public:
    PropagateLocalMkdir(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
        , _deleteExistingFile(false)
    {
    }
    void start() override;

    /**
     * Whether an existing file with the same name may be deleted before
     * creating the directory.
     *
     * Default: false.
     */
    void setDeleteExistingFile(bool enabled);

private:
    bool _deleteExistingFile;
};

/**
 * @brief Rename a local file/directory after discovering a rename on the server
 * @ingroup libsync
 */
class PropagateLocalRename : public PropagateItemJob
{
    Q_OBJECT
public:
    PropagateLocalRename(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateItemJob(propagator, item)
    {
    }
    void start() override;
    JobParallelism parallelism() override { return _item->isDirectory() ? WaitForFinished : FullParallelism; }
};
}
