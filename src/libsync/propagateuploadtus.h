/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "propagateuploadfile.h"
#include "uploaddevice.h"

namespace OCC {
Q_DECLARE_LOGGING_CATEGORY(lcPropagateUploadTUS)

class PropagateUploadFileTUS : public PropagateUploadCommon
{
    Q_OBJECT

private:
    SimpleNetworkJob *makeCreationWithUploadJob(QNetworkRequest *request, UploadDevice *device);
    QNetworkRequest prepareRequest(const quint64 &chunkSize);
    UploadDevice *prepareDevice(const quint64 &chunkSize);

    void startNextChunk();
    void slotChunkFinished();
    void finalize(const QString &etag, const QByteArray &fileId);

    quint64 _currentOffset = 0;
    QUrl _location;

public:
    PropagateUploadFileTUS(OwncloudPropagator *propagator, const SyncFileItemPtr &item);

    void doStartUpload() override;
public Q_SLOTS:
    void abort(PropagatorJob::AbortType abortType) override;
};

}
