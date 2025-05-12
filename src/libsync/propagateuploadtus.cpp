/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "propagateuploadtus.h"
#include "account.h"
#include "capabilities.h"
#include "common/asserts.h"
#include "common/checksums.h"
#include "common/syncjournaldb.h"
#include "common/syncjournalfilerecord.h"
#include "common/utility.h"
#include "filesystem.h"
#include "httplogger.h"
#include "networkjobs.h"
#include "owncloudpropagator_p.h"
#include "propagateupload.h"
#include "propagatorjobs.h"
#include "syncengine.h"

#include <QFileInfo>

#include <cmath>
#include <memory>

namespace {

QByteArray uploadOffset()
{
    return QByteArrayLiteral("Upload-Offset");
}

void setTusVersionHeader(QNetworkRequest &req){
    req.setRawHeader(QByteArrayLiteral("Tus-Resumable"), QByteArrayLiteral("1.0.0"));
}
}

namespace OCC {
// be very verbose for now
Q_LOGGING_CATEGORY(lcPropagateUploadTUS, "sync.propagator.upload.tus", QtDebugMsg)


UploadDevice *PropagateUploadFileTUS::prepareDevice(const quint64 &chunkSize)
{
    const QString localFileName = propagator()->fullLocalPath(_item->_file);
    // If the file is currently locked, we want to retry the sync
    // when it becomes available again.
    if (FileSystem::isFileLocked(localFileName, FileSystem::LockMode::SharedRead)) {
        Q_EMIT propagator()->seenLockedFile(localFileName, FileSystem::LockMode::SharedRead);
        abortWithError(SyncFileItem::SoftError, tr("%1 the file is currently in use").arg(localFileName));
        return nullptr;
    }
    auto device = std::make_unique<UploadDevice>(localFileName, _currentOffset, chunkSize, propagator()->_bandwidthManager);
    if (!device->open(QIODevice::ReadOnly)) {
        qCWarning(lcPropagateUploadTUS) << "Could not prepare upload device: " << device->errorString();

        // Soft error because this is likely caused by the user modifying his files while syncing
        abortWithError(SyncFileItem::SoftError, device->errorString());
        return nullptr;
    }
    return device.release();
}


SimpleNetworkJob *PropagateUploadFileTUS::makeCreationWithUploadJob(QNetworkRequest *request, UploadDevice *device)
{
    Q_ASSERT(propagator()->account()->capabilities().tusSupport().extensions.contains(QStringLiteral("creation-with-upload")));

    const auto checksumHeader = ChecksumHeader::parseChecksumHeader(_transmissionChecksumHeader);

    QByteArrayList encodedMetaData;
    auto addMetaData = [&encodedMetaData](const QByteArray &key, const QByteArray &value) { encodedMetaData << key + ' ' + value.toBase64(); };
    addMetaData(QByteArrayLiteral("filename"), propagator()->fullRemotePath(_item->_file).toUtf8());
    // in difference to the old protocol the algrithm and the value are space seperated
    addMetaData(QByteArrayLiteral("checksum"), Utility::enumToString(checksumHeader.type()).toUtf8() + ' ' + checksumHeader.checksum());
    addMetaData(QByteArrayLiteral("mtime"), QByteArray::number(static_cast<int64_t>(_item->_modtime)));

    request->setRawHeader(QByteArrayLiteral("Upload-Metadata"), encodedMetaData.join(','));
    request->setRawHeader(QByteArrayLiteral("Upload-Length"), QByteArray::number(_item->_size));
    return new SimpleNetworkJob(propagator()->account(), propagator()->webDavUrl(), {}, "POST", device, *request, this);
}

QNetworkRequest PropagateUploadFileTUS::prepareRequest(const quint64 &chunkSize)
{
    QNetworkRequest request;
    const auto headers = PropagateUploadFileCommon::headers();
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        request.setRawHeader(it.key(), it.value());
    }

    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/offset+octet-stream"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(chunkSize));
    request.setRawHeader(uploadOffset(), QByteArray::number(_currentOffset));
    setTusVersionHeader(request);
    return request;
}

PropagateUploadFileTUS::PropagateUploadFileTUS(OwncloudPropagator *propagator, const SyncFileItemPtr &item)
    : PropagateUploadFileCommon(propagator, item)
{
}

void PropagateUploadFileTUS::doStartUpload()
{
    if (_transmissionChecksumHeader.isEmpty()) {
        abortWithError(SyncFileItem::SoftError, tr("Checksum computation failed"));
        return;
    }
    propagator()->reportProgress(*_item, 0);
    propagator()->_activeJobList.append(this);

    const auto info = propagator()->_journal->getUploadInfo(_item->_file);
    if (info.validate(_item->_size, _item->_modtime, _item->_checksumHeader)) {
        _location = info._url;
        Q_ASSERT(_location.isValid());
        auto job = new SimpleNetworkJob(propagator()->account(), _location, {}, "HEAD", nullptr, {}, this);
        connect(job, &SimpleNetworkJob::finishedSignal, this, &PropagateUploadFileTUS::slotChunkFinished);
        job->start();
    } else {
        startNextChunk();
    }
}

void PropagateUploadFileTUS::startNextChunk()
{
    if (propagator()->_abortRequested)
        return;

    const quint64 chunkSize = [&] {
        auto chunkSize = _item->_size - _currentOffset;
        if (propagator()->account()->capabilities().tusSupport().max_chunk_size) {
            chunkSize = std::min<qint64>(chunkSize, propagator()->account()->capabilities().tusSupport().max_chunk_size);
        }
        return chunkSize;
    }();

    QNetworkRequest req = prepareRequest(chunkSize);
    auto device = prepareDevice(chunkSize);
    if (!device) {
        return;
    }

    SimpleNetworkJob *job;
    if (_currentOffset != 0) {
        qCDebug(lcPropagateUploadTUS) << "Starting to patch upload:" << propagator()->fullRemotePath(_item->_file);
        job = new SimpleNetworkJob(propagator()->account(), _location, {}, "PATCH", device, req, this);
    } else {
        Q_ASSERT(_location.isEmpty());
        qCDebug(lcPropagateUploadTUS) << "Starting creation with upload:" << propagator()->fullRemotePath(_item->_file);
        job = makeCreationWithUploadJob(&req, device);
    }

    job->setPriority(QNetworkRequest::LowPriority);
    qCDebug(lcPropagateUploadTUS) << "Offset:" << _currentOffset << _currentOffset  / (_item->_size + 1) * 100
                                  << "Chunk:" << chunkSize << chunkSize / (_item->_size + 1) * 100;

    addChildJob(job);
    connect(job, &SimpleNetworkJob::finishedSignal, this, &PropagateUploadFileTUS::slotChunkFinished);
    job->addNewReplyHook([device, this](QNetworkReply *reply) {
        connect(reply, &QNetworkReply::uploadProgress, device, &UploadDevice::slotJobUploadProgress);
        connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 bytesSent, qint64) {
            propagator()->reportProgress(*_item, _currentOffset + bytesSent);
        });
    });
    job->start();
}

void PropagateUploadFileTUS::slotChunkFinished()
{
    SimpleNetworkJob *job = qobject_cast<SimpleNetworkJob *>(sender());
    Q_ASSERT(job);
    qCDebug(lcPropagateUploadTUS) << propagator()->fullRemotePath(_item->_file) << HttpLogger::requestVerb(*job->reply());

    _item->_httpErrorCode = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    _item->_responseTimeStamp = job->responseTimestamp();
    _item->_requestId = job->requestId();

    QNetworkReply::NetworkError err = job->reply()->error();
    if (err != QNetworkReply::NoError) {
        // try to get the offset if possible, only try once
        if (err == QNetworkReply::TimeoutError && !_location.isEmpty() && HttpLogger::requestVerb(*job->reply())  != "HEAD")
        {
            qCWarning(lcPropagateUploadTUS) << propagator()->fullRemotePath(_item->_file) << "Encountered a timeout -> get progrss for" << _location;
            QNetworkRequest req;
            setTusVersionHeader(req);
            auto updateJob = new SimpleNetworkJob(propagator()->account(), propagator()->webDavUrl(), _location.path(), "HEAD", {}, req, this);
            addChildJob(updateJob);
            connect(updateJob, &SimpleNetworkJob::finishedSignal, this, &PropagateUploadFileTUS::slotChunkFinished);
            updateJob->start();
            return;

        }
        commonErrorHandling(job);
        return;
    }

    const qint64 offset = job->reply()->rawHeader(uploadOffset()).toLongLong();
    propagator()->reportProgress(*_item, offset);
    _currentOffset = offset;
    // first response after a POST request
    if (_location.isEmpty()) {
        _location = job->reply()->header(QNetworkRequest::LocationHeader).toUrl();
    }


    _finished = offset == _item->_size;

    // Check if the file still exists
    const QString fullFilePath(propagator()->fullLocalPath(_item->_file));
    if (!FileSystem::fileExists(fullFilePath)) {
        if (!_finished) {
            abortWithError(SyncFileItem::SoftError, tr("The local file was removed during sync."));
            return;
        } else {
            propagator()->_anotherSyncNeeded = true;
        }
    }

    // Check whether the file changed since discovery.
    if (FileSystem::fileChanged(QFileInfo{fullFilePath}, _item->_size, _item->_modtime)) {
        propagator()->_anotherSyncNeeded = true;
        if (!_finished) {
            abortWithError(SyncFileItem::Message, fileChangedMessage());
            // FIXME:  the legacy code was retrying for a few seconds.
            //         and also checking that after the last chunk, and removed the file in case of INSTRUCTION_NEW
            return;
        }
    }
    if (!_finished) {
        // we just started the upload
        if (HttpLogger::requestVerb(*job->reply()) == QByteArrayLiteral("POST")) {
            // add the new location
            auto info = _item->toUploadInfo();
            info._url = _location;
            propagator()->_journal->setUploadInfo(_item->_file, info);
        }
        startNextChunk();
        return;
    }

    // ==== handling when the upload is finished:
    const QString etag = getEtagFromReply(job->reply());
    const QByteArray remPerms = job->reply()->rawHeader("OC-Perm");
    if (!remPerms.isEmpty()) {
        _item->_remotePerm = RemotePermissions::fromServerString(QString::fromUtf8(remPerms));
    }

    _finished = !(etag.isEmpty() || _item->_remotePerm.isNull());
    if (!_finished) {
        // Either the ETag or the remote Permissions were not in the headers of the reply.
        // Start a PROPFIND to fetch these data from the server.
        auto check = new PropfindJob(propagator()->account(), propagator()->webDavUrl(), propagator()->fullRemotePath(_item->_file), PropfindJob::Depth::Zero, this);
        addChildJob(check);
        check->setProperties({ "http://owncloud.org/ns:fileid", "http://owncloud.org/ns:permissions", "getetag" });
        connect(check, &PropfindJob::directoryListingIterated, this, [this](const QString &, const QMap<QString, QString> &map) {
            _finished = true;
            _item->_remotePerm = RemotePermissions::fromServerString(map.value(QStringLiteral("permissions")));
            finalize(Utility::normalizeEtag(map.value(QStringLiteral("getetag"))), map.value(QStringLiteral("fileid")).toUtf8());
        });
        check->start();
        return;
    }
    _item->_remotePerm = RemotePermissions::fromServerString(QString::fromUtf8(job->reply()->rawHeader("Oc-Perm")));
    // the file id should only be empty for new files up- or downloaded
    finalize(etag, job->reply()->rawHeader("OC-FileID"));
}

void PropagateUploadFileTUS::finalize(const QString &etag, const QByteArray &fileId)
{
    OC_ASSERT(_finished);
    _item->_etag = etag;
    if (!fileId.isEmpty()) {
        if (!_item->_fileId.isEmpty() && _item->_fileId != fileId) {
            qCWarning(lcPropagateUploadTUS) << "File ID changed!" << _item->_fileId << fileId;
        }
        _item->_fileId = fileId;
    }
    propagator()->_activeJobList.removeOne(this);
    PropagateUploadFileCommon::finalize();
}

void PropagateUploadFileTUS::abort(PropagatorJob::AbortType abortType)
{
    abortNetworkJobs(
        abortType,
        [](AbstractNetworkJob *) {
            // TODO
            return true;
        });
}

}
