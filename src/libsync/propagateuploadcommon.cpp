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

#include "account.h"
#include "filesystem.h"
#include "networkjobs.h"
#include "owncloudpropagator_p.h"
#include "propagateremotedelete.h"
#include "propagateuploadfile.h"
#include "syncengine.h"

#include "common/asserts.h"
#include "common/checksums.h"
#include "common/syncjournaldb.h"
#include "common/syncjournalfilerecord.h"
#include "common/utility.h"

#include "libsync/theme.h"

#include <QDir>
#include <QFileInfo>

#include <chrono>
#include <cmath>

using namespace std::chrono_literals;

namespace OCC {

Q_LOGGING_CATEGORY(lcPropagateUpload, "sync.propagator.upload", QtInfoMsg)

/**
 * We do not want to upload files that are currently being modified.
 * To avoid that, we don't upload files that have a modification time
 * that is too close to the current time.
 *
 * This interacts with the msBetweenRequestAndSync delay in the folder
 * manager. If that delay between file-change notification and sync
 * has passed, we should accept the file for upload here.
 */
static bool fileIsStillChanging(const SyncFileItem &item)
{
    const QDateTime modtime = Utility::qDateTimeFromTime_t(item._modtime);
    const auto secondsSinceMod = std::chrono::seconds(modtime.secsTo(QDateTime::currentDateTimeUtc()));

    return secondsSinceMod < SyncEngine::minimumFileAgeForUpload
        // if the mtime is too much in the future we *do* upload the file
        && secondsSinceMod > -1s;
}

const QString PropagateUploadCommon::fileChangedMessage()
{
    return tr("Local file changed during sync. It will be resumed.");
}

void PropagateUploadCommon::setDeleteExisting(bool enabled)
{
    _deleteExisting = enabled;
}


void PropagateUploadCommon::start()
{
    if (propagator()->_abortRequested) {
        return;
    }

    // Check if the specific file can be accessed
    if (propagator()->hasCaseClashAccessibilityProblem(_item->_file)) {
        done(SyncFileItem::NormalError, tr("File %1 cannot be uploaded because another file with the same name, differing only in case, exists").arg(QDir::toNativeSeparators(_item->_file)));
        return;
    }

    // Check if we believe that the upload will fail due to remote quota limits
    const qint64 quotaGuess = propagator()->_folderQuota.value(
        QFileInfo(_item->_file).path(), std::numeric_limits<qint64>::max());
    if (_item->_size > quotaGuess) {
        // Necessary for blacklisting logic
        _item->_httpErrorCode = 507;
        Q_EMIT propagator()->insufficientRemoteStorage();
        done(SyncFileItem::DetailError, tr("Upload of %1 exceeds the quota for the folder").arg(Utility::octetsToString(_item->_size)));
        return;
    }

    propagator()->_activeJobList.append(this);

    if (!_deleteExisting) {
        return slotComputeContentChecksum();
    }

    auto job = new DeleteJob(propagator()->account(), propagator()->webDavUrl(),
        propagator()->fullRemotePath(_item->_file),
        this);
    addChildJob(job);
    connect(job, &DeleteJob::finishedSignal, this, &PropagateUploadCommon::slotComputeContentChecksum);
    job->start();
}

void PropagateUploadCommon::slotComputeContentChecksum()
{
    if (propagator()->_abortRequested) {
        return;
    }

    const QString filePath = propagator()->fullLocalPath(_item->_file);

    // remember the modtime before computing the checksum to be able to detect a file
    // change during the checksum calculation
    _item->_modtime = FileSystem::getModTime(filePath);

    const auto checksumType = propagator()->account()->capabilities().preferredUploadChecksumType();

    // Maybe the discovery already computed the checksum?
    const auto checksumHeader = ChecksumHeader::parseChecksumHeader(_item->_checksumHeader);
    if (checksumHeader.type() == checksumType) {
        slotComputeTransmissionChecksum(checksumType, checksumHeader.checksum());
        return;
    }

    // we must be able to read the file
    if (FileSystem::isFileLocked(filePath, FileSystem::LockMode::SharedRead)) {
        Q_EMIT propagator()->seenLockedFile(filePath, FileSystem::LockMode::SharedRead);
        abortWithError(SyncFileItem::SoftError, tr("%1 the file is currently in use").arg(filePath));
        return;
    }

    // Compute the content checksum.
    auto computeChecksum = new ComputeChecksum(this);
    computeChecksum->setChecksumType(checksumType);

    connect(computeChecksum, &ComputeChecksum::done,
        this, &PropagateUploadCommon::slotComputeTransmissionChecksum);
    connect(computeChecksum, &ComputeChecksum::done,
        computeChecksum, &QObject::deleteLater);
    computeChecksum->start(filePath);
}

void PropagateUploadCommon::slotComputeTransmissionChecksum(CheckSums::Algorithm contentChecksumType, const QByteArray &contentChecksum)
{
    _item->_checksumHeader = ChecksumHeader(contentChecksumType, contentChecksum).makeChecksumHeader();

    // Reuse the content checksum as the transmission checksum if possible
    const auto supportedTransmissionChecksums =
        propagator()->account()->capabilities().supportedChecksumTypes();
    if (supportedTransmissionChecksums.contains(contentChecksumType)) {
        slotStartUpload(contentChecksumType, contentChecksum);
        return;
    }

    const QString filePath = propagator()->fullLocalPath(_item->_file);
    // we must be able to read the file
    if (FileSystem::isFileLocked(filePath, FileSystem::LockMode::SharedRead)) {
        Q_EMIT propagator()->seenLockedFile(filePath, FileSystem::LockMode::SharedRead);
        abortWithError(SyncFileItem::SoftError, tr("%1 the file is currently in use").arg(filePath));
        return;
    }

    // Compute the transmission checksum.
    auto computeChecksum = new ComputeChecksum(this);
    if (uploadChecksumEnabled()) {
        computeChecksum->setChecksumType(propagator()->account()->capabilities().uploadChecksumType());
    } else {
        computeChecksum->setChecksumType(CheckSums::Algorithm::PARSE_ERROR);
    }

    connect(computeChecksum, &ComputeChecksum::done,
        this, &PropagateUploadCommon::slotStartUpload);
    connect(computeChecksum, &ComputeChecksum::done,
        computeChecksum, &QObject::deleteLater);
    computeChecksum->start(filePath);
}

void PropagateUploadCommon::slotStartUpload(CheckSums::Algorithm transmissionChecksumType, const QByteArray &transmissionChecksum)
{
    // Remove ourselves from the list of active job, before any possible call to done()
    // When we start chunks, we will add it again, once for every chunk.
    propagator()->_activeJobList.removeOne(this);

    _transmissionChecksumHeader = ChecksumHeader(transmissionChecksumType, transmissionChecksum).makeChecksumHeader();

    // If no checksum header was not set, reuse the transmission checksum as the content checksum.
    if (_item->_checksumHeader.isEmpty()) {
        _item->_checksumHeader = _transmissionChecksumHeader;
    }

    const QString fullFilePath = propagator()->fullLocalPath(_item->_file);

    if (!FileSystem::fileExists(fullFilePath)) {
        done(SyncFileItem::SoftError, tr("File Removed"));
        return;
    }
    _item->_size = FileSystem::getSize(QFileInfo{fullFilePath});

    const time_t prevModtime = _item->_modtime; // the _item value was set in PropagateUploadFile::start()
    // but a potential checksum calculation could have taken some time during which the file could
    // have been changed again, so better check again here.

    // But skip the file if the mtime is too close to 'now'!
    // That usually indicates a file that is still being changed
    // or not yet fully copied to the destination.
    _item->_modtime = FileSystem::getModTime(fullFilePath);
    if (prevModtime != _item->_modtime || fileIsStillChanging(*_item)) {
        propagator()->_anotherSyncNeeded = true;
        done(SyncFileItem::Message, fileChangedMessage());
        return;
    }

    doStartUpload();
}

void PropagateUploadCommon::done(SyncFileItem::Status status, const QString &errorString)
{
    _finished = true;
    PropagateItemJob::done(status, errorString);
}

void PropagateUploadCommon::checkResettingErrors()
{
    if (_item->_httpErrorCode == 412
        || propagator()->account()->capabilities().httpErrorCodesThatResetFailingChunkedUploads().contains(_item->_httpErrorCode)) {
        auto uploadInfo = propagator()->_journal->getUploadInfo(_item->_file);
        uploadInfo._errorCount += 1;
        if (uploadInfo._errorCount > 3) {
            qCInfo(lcPropagateUpload) << "Reset transfer of" << _item->_file
                                      << "due to repeated error" << _item->_httpErrorCode;
            uploadInfo = SyncJournalDb::UploadInfo();
        } else {
            qCInfo(lcPropagateUpload) << "Error count for maybe-reset error" << _item->_httpErrorCode
                                      << "on file" << _item->_file
                                      << "is" << uploadInfo._errorCount;
        }
        propagator()->_journal->setUploadInfo(_item->_file, uploadInfo);
        propagator()->_journal->commit(QStringLiteral("Upload info"));
    }
}

void PropagateUploadCommon::commonErrorHandling(AbstractNetworkJob *job)
{
    QByteArray replyContent;
    QString errorString = job->errorStringParsingBody(&replyContent);
    qCDebug(lcPropagateUpload) << replyContent; // display the XML error in the debug

    if (_item->_httpErrorCode == 412) {
        // Precondition Failed: Either an etag or a checksum mismatch.

        // Maybe the bad etag is in the database, we need to clear the
        // parent folder etag so we won't read from DB next sync.
        propagator()->_journal->schedulePathForRemoteDiscovery(_item->_file);
        propagator()->_anotherSyncNeeded = true;
    }

    // Ensure errors that should eventually reset the chunked upload are tracked.
    checkResettingErrors();

    SyncFileItem::Status status = classifyError(job->reply()->error(), _item->_httpErrorCode,
        &propagator()->_anotherSyncNeeded, replyContent);

    // Insufficient remote storage.
    if (_item->_httpErrorCode == 507) {
        // Update the quota expectation
        const auto path = QFileInfo(_item->_file).path();
        auto quotaIt = propagator()->_folderQuota.find(path);
        if (quotaIt != propagator()->_folderQuota.end()) {
            quotaIt.value() = qMin(quotaIt.value(), _item->_size - 1);
        } else {
            propagator()->_folderQuota[path] = _item->_size - 1;
        }

        // Set up the error
        status = SyncFileItem::DetailError;
        errorString = tr("Upload of %1 exceeds the quota for the folder").arg(Utility::octetsToString(_item->_size));
        Q_EMIT propagator()->insufficientRemoteStorage();
    }

    abortWithError(status, errorString);
}

void PropagateUploadCommon::adjustLastJobTimeout(AbstractNetworkJob *job, qint64 fileSize)
{
    // Calculate 3 minutes for each gigabyte of data
    const auto timeout = std::chrono::minutes(static_cast<quint64>((3min).count() * fileSize / 1e9));
    // Maximum of 30 minutes
    // not less than the default/current val
    if (timeout > job->timeoutSec() && timeout < 30min) {
        job->setTimeout(timeout);
    }
}

// This function is used whenever there is an error occurring and jobs might be in progress
void PropagateUploadCommon::abortWithError(SyncFileItem::Status status, const QString &error)
{
    qCWarning(lcPropagateUpload) << Q_FUNC_INFO << _item->_file << error;
    if (!_aborting) {
        abort(AbortType::Synchronous);
        done(status, error);
    }
}

void PropagateUploadCommon::addChildJob(AbstractNetworkJob *job)
{
    _childJobs.insert(job);
    connect(
        job, &AbstractNetworkJob::aboutToFinishSignal, this, [job, this] {
            _childJobs.erase(job);
        },
        Qt::DirectConnection);
}

QMap<QByteArray, QByteArray> PropagateUploadCommon::headers()
{
    QMap<QByteArray, QByteArray> headers;
    headers[QByteArrayLiteral("Content-Type")] = QByteArrayLiteral("application/octet-stream");
    headers[QByteArrayLiteral("X-OC-Mtime")] = QByteArray::number(qint64(_item->_modtime));

    if (Q_UNLIKELY(Theme::instance()->enableCernBranding() && _item->_file.contains(QLatin1String(".sys.admin#recall#")))) {
        // This is a file recall triggered by the admin.  Note: the
        // recall list file created by the admin and downloaded by the
        // client (.sys.admin#recall#) also falls into this category
        // (albeit users are not supposed to mess up with it)

        // We use a special tag header so that the server may decide to store this file away in some admin stage area
        // And not directly in the user's area (which would trigger redownloads etc.).
        headers["OC-Tag"] = ".sys.admin#recall#";
    }

    if (!_item->_etag.isEmpty() && _item->_etag != QLatin1String("empty_etag")
        && (_item->instruction() & ~(CSYNC_INSTRUCTION_NEW | CSYNC_INSTRUCTION_TYPE_CHANGE)) // On new files never send an "If-Match"
        && !_deleteExisting) {
        // We add quotes because the owncloud server always adds quotes around the etag, and
        //  csync_owncloud.c's owncloud_file_id always strips the quotes.
        headers[QByteArrayLiteral("If-Match")] = QStringLiteral("\"%1\"").arg(_item->_etag).toUtf8();
    }

    // Set up a conflict file header pointing to the original file
    auto conflictRecord = propagator()->_journal->conflictRecord(_item->_file.toUtf8());
    if (conflictRecord.isValid()) {
        headers[QByteArrayLiteral("OC-Conflict")] = "1";
        if (!conflictRecord.initialBasePath.isEmpty())
            headers[QByteArrayLiteral("OC-ConflictInitialBasePath")] = conflictRecord.initialBasePath;
        if (!conflictRecord.baseFileId.isEmpty())
            headers[QByteArrayLiteral("OC-ConflictBaseFileId")] = conflictRecord.baseFileId;
        if (conflictRecord.baseModtime != -1)
            headers[QByteArrayLiteral("OC-ConflictBaseMtime")] = QByteArray::number(conflictRecord.baseModtime);
        if (!conflictRecord.baseEtag.isEmpty())
            headers[QByteArrayLiteral("OC-ConflictBaseEtag")] = conflictRecord.baseEtag;
    }

    return headers;
}

void PropagateUploadCommon::finalize()
{
    OC_ENFORCE(state() != Finished);

    // Update the quota, if known
    if (!_quotaUpdated) {
        auto quotaIt = propagator()->_folderQuota.find(QFileInfo(_item->_file).path());
        if (quotaIt != propagator()->_folderQuota.end()) {
            quotaIt.value() -= _item->_size;
        }
        _quotaUpdated = true;
    }

    if (_item->_remotePerm.isNull()) {
        qCWarning(lcPropagateUpload) << "PropagateUploadCommon::finalize: Missing permissions for" << propagator()->fullRemotePath(_item->_file);
        auto *permCheck =
            new PropfindJob(propagator()->account(), propagator()->webDavUrl(), propagator()->fullRemotePath(_item->_file), PropfindJob::Depth::Zero, this);
        addChildJob(permCheck);
        permCheck->setProperties({ "http://owncloud.org/ns:permissions" });
        connect(permCheck, &PropfindJob::directoryListingIterated, this, [this](const QString &, const QMap<QString, QString> &map) {
            _item->_remotePerm = RemotePermissions::fromServerString(map.value(QStringLiteral("permissions")));
            Q_ASSERT(!_item->_remotePerm.isNull());
            finalize();
        });
        connect(permCheck, &PropfindJob::finishedWithError, this,
            [this] { done(SyncFileItem::FatalError, tr("The server did not provide the file permissions")); });
        permCheck->start();
        return;
    }

    // Update the database entry
    const auto result = propagator()->updateMetadata(*_item);
    if (!result) {
        done(SyncFileItem::FatalError, tr("Error updating metadata: %1").arg(result.error()));
        return;
    }

    // Files that were new on the remote shouldn't have online-only pin state
    // even if their parent folder is online-only.
    if (_item->instruction() & (CSYNC_INSTRUCTION_NEW | CSYNC_INSTRUCTION_TYPE_CHANGE)) {
        auto &vfs = propagator()->syncOptions()._vfs;
        const auto pin = vfs->pinState(_item->_file);
        if (pin && *pin == PinState::OnlineOnly) {
            std::ignore = vfs->setPinState(_item->_file, PinState::Unspecified);
        }
    }

    // Remove from the progress database:
    propagator()->_journal->setUploadInfo(_item->_file, SyncJournalDb::UploadInfo());
    propagator()->_journal->commit(QStringLiteral("upload file start"));

    done(SyncFileItem::Success);
}

void PropagateUploadCommon::abortNetworkJobs(
    PropagatorJob::AbortType abortType,
    const std::function<bool(AbstractNetworkJob *)> &mayAbortJob)
{
    if (_aborting)
        return;
    _aborting = true;

    // Count the number of jobs that need aborting, and Q_EMIT the overall
    // abort signal when they're all done.
    QSharedPointer<int> runningCount(new int(0));
    auto oneAbortFinished = [this, runningCount]() {
        (*runningCount)--;
        if (*runningCount == 0) {
            Q_EMIT this->abortFinished();
        }
    };

    // Abort all running jobs, except for explicitly excluded ones
    // perform actions on a copy as aborted jobs will be removed from _childJobs
    const auto children = _childJobs;
    for (auto *job : children) {
        auto reply = job->reply();
        if (!reply || !reply->isRunning())
            continue;

        (*runningCount)++;

        // If a job should not be aborted that means we'll never abort before
        // the hard abort timeout signal comes as runningCount will never go to
        // zero.
        // We may however finish before that if the un-abortable job completes
        // normally.
        if (!mayAbortJob(job))
            continue;

        // Abort the job
        if (abortType == AbortType::Asynchronous) {
            // Connect to finished signal of job reply to asynchronously finish the abort
            connect(reply, &QNetworkReply::finished, this, oneAbortFinished);
        }
        job->abort();
    }

    if (*runningCount == 0 && abortType == AbortType::Asynchronous)
        Q_EMIT abortFinished();
}
}
