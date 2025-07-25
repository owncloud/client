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
#include "propagateuploadcommon.h"

#include <unordered_set>

namespace OCC {

/**
 * @ingroup libsync
 *
 * Propagation job, simple PUT upload.
 *
 */
class PropagateUploadFile : public PropagateUploadCommon
{
    Q_OBJECT

public:
    PropagateUploadFile(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
        : PropagateUploadCommon(propagator, item)
    {
    }

    void doStartUpload() override;
public Q_SLOTS:
    void abort(PropagatorJob::AbortType abortType) override;
private Q_SLOTS:
    void startUpload();
    void slotPutFinished();
    void slotUploadProgress(qint64, qint64);
};
}
