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

#include "propagateuploadfile.h"
#include "account.h"
#include "common/checksums.h"
#include "common/syncjournaldb.h"
#include "filesystem.h"
#include "owncloudpropagator_p.h"
#include "propagatorjobs.h"
#include "putfilejob.h"
#include "syncengine.h"
#include "uploaddevice.h"

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

#include <memory>

namespace OCC {

void PropagateUploadFile::doStartUpload()
{
    const QString fileName = propagator()->fullLocalPath(_item->_file);
    // If the file is currently locked, we want to retry the sync
    // when it becomes available again.
    if (FileSystem::isFileLocked(fileName, FileSystem::LockMode::SharedRead)) {
        Q_EMIT propagator()->seenLockedFile(fileName, FileSystem::LockMode::SharedRead);
        abortWithError(SyncFileItem::SoftError, tr("%1 the file is currently in use").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const SyncJournalDb::UploadInfo progressInfo = propagator()->_journal->getUploadInfo(_item->_file);

    // If there is only one chunk, write the checksum in the database, so if the PUT is sent
    // to the server, but the connection drops before we get the etag, we can check the checksum
    // in reconcile (issue #5106)
    auto pi = _item->toUploadInfo();
    pi._chunk = 0;
    pi._transferid = 0; // We set a null transfer id because it is not chunked.
    pi._errorCount = 0;
    propagator()->_journal->setUploadInfo(_item->_file, pi);
    propagator()->_journal->commit(QStringLiteral("Upload info"));

    propagator()->reportProgress(*_item, 0);
    startUpload();
}

void PropagateUploadFile::startUpload()
{
    if (propagator()->_abortRequested)
        return;

    qint64 fileSize = _item->_size;
    auto headers = PropagateUploadCommon::headers();
    headers[QByteArrayLiteral("OC-Total-Length")] = QByteArray::number(fileSize);

    QString path = _item->_file;

    if (!_transmissionChecksumHeader.isEmpty()) {
        qCInfo(lcPropagateUpload) << propagator()->fullRemotePath(path) << _transmissionChecksumHeader;
        headers[checkSumHeaderC] = _transmissionChecksumHeader;
    }

    const QString fileName = propagator()->fullLocalPath(_item->_file);
    auto device = std::make_unique<UploadDevice>(fileName, 0, fileSize);
    if (!device->open(QIODevice::ReadOnly)) {
        qCWarning(lcPropagateUpload) << "Could not prepare upload device: " << device->errorString();
        // Soft error because this is likely caused by the user modifying his files while syncing
        abortWithError(SyncFileItem::SoftError, device->errorString());
        return;
    }

    // TODO: review usage of scoped pointer vs qt memory management
    // job takes ownership of device via a QScopedPointer. Job deletes itself when finishing
    PUTFileJob *job = new PUTFileJob(propagator()->account(), propagator()->webDavUrl(), propagator()->fullRemotePath(path), std::move(device), headers, this);
    addChildJob(job);
    connect(job, &PUTFileJob::finishedSignal, this, &PropagateUploadFile::slotPutFinished);
    connect(job, &PUTFileJob::uploadProgress, this, &PropagateUploadFile::slotUploadProgress);
    adjustLastJobTimeout(job, fileSize);
    job->start();
    propagator()->_activeJobList.append(this);

    propagator()->scheduleNextJob();
}

void PropagateUploadFile::slotPutFinished()
{
    PUTFileJob *job = qobject_cast<PUTFileJob *>(sender());
    Q_ASSERT(job);

    propagator()->_activeJobList.removeOne(this);

    if (_finished) {
        // We have sent the finished signal already. We don't need to handle any remaining jobs
        return;
    }

    _item->_httpErrorCode = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    _item->_responseTimeStamp = job->responseTimestamp();
    _item->_requestId = job->requestId();
    QNetworkReply::NetworkError err = job->reply()->error();
    if (err != QNetworkReply::NoError) {
        commonErrorHandling(job);
        return;
    }

    if (_item->_httpErrorCode == 202) {
        done(SyncFileItem::NormalError, tr("The server did ask for a removed legacy feature(polling)"));
        return;
    }

    // Check the file again post upload.
    // Two cases must be considered separately: If the upload is finished,
    // the file is on the server and has a changed ETag. In that case,
    // the etag has to be properly updated in the client journal, and because
    // of that we can bail out here with an error. But we can reschedule a
    // sync ASAP.
    // But if the upload is ongoing, because not all chunks were uploaded
    // yet, the upload can be stopped and an error can be displayed, because
    // the server hasn't registered the new file yet.
    QString etag = getEtagFromReply(job->reply());
    _finished = etag.length() > 0;

    // Check if the file still exists
    const QString fullFilePath(propagator()->fullLocalPath(_item->_file));
    if (!FileSystem::fileExists(fullFilePath)) {
        if (!_finished) {
            abortWithError(SyncFileItem::SoftError, tr("The local file was removed during sync."));
            return;
        }
        propagator()->_anotherSyncNeeded = true;
    }

    // Check whether the file changed since discovery.
    if (FileSystem::fileChanged(QFileInfo{fullFilePath}, _item->_size, _item->_modtime)) {
        propagator()->_anotherSyncNeeded = true;
        if (!_finished) {
            abortWithError(SyncFileItem::Message, fileChangedMessage());
            return;
        }
    }

    if (!_finished) {
        if (!childJobs().empty()) {
            // just wait for the other job to finish.
            return;
        }
        done(SyncFileItem::NormalError, tr("The server did not acknowledge the file upload. (No e-tag was present)"));
        return;
    }

    // the file id should only be empty for new files up- or downloaded
    QByteArray fid = job->reply()->rawHeader("OC-FileID");
    if (!fid.isEmpty()) {
        if (!_item->_fileId.isEmpty() && _item->_fileId != fid) {
            qCWarning(lcPropagateUpload) << "File ID changed!" << _item->_fileId << fid;
        }
        _item->_fileId = fid;
    }

    _item->_etag = etag;

    if (job->reply()->rawHeader("X-OC-MTime") != "accepted") {
        // X-OC-MTime is supported since owncloud 5.0.   But not when chunking.
        // Normally Owncloud 6 always puts X-OC-MTime
        qCWarning(lcPropagateUpload) << "Server does not support X-OC-MTime" << job->reply()->rawHeader("X-OC-MTime");
        // Well, the mtime was not set
    }

    finalize();
}


void PropagateUploadFile::slotUploadProgress(qint64 sent, qint64 total)
{
    // Completion is signaled with sent=0, total=0; avoid accidentally
    // resetting progress due to sent bytes being zero by ignoring it.
    // finishedSignal() is bound to be emitted soon anyway.
    // See https://bugreports.qt.io/browse/QTBUG-44782.
    if (sent == 0 && total == 0) {
        return;
    }

    propagator()->reportProgress(*_item, sent);
}

void PropagateUploadFile::abort(PropagatorJob::AbortType abortType)
{
    abortNetworkJobs(abortType, [abortType](AbstractNetworkJob *job) {
        if (PUTFileJob *putJob = qobject_cast<PUTFileJob *>(job)) {
            if (abortType == AbortType::Asynchronous && putJob->device()->atEnd()) {
                return false;
            }
        }
        return true;
    });
}

}
