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
