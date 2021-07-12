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

#include "propagateremotemkdir.h"
#include "owncloudpropagator_p.h"
#include "account.h"
#include "common/syncjournalfilerecord.h"
#include "propagateremotedelete.h"
#include "common/asserts.h"

#include <QFile>
#include <QLoggingCategory>
#include <QtConcurrent>
namespace {
auto UpdateMetaDataRetyTimeOut = std::chrono::seconds(30);
}
namespace OCC {

Q_LOGGING_CATEGORY(lcPropagateRemoteMkdir, "sync.propagator.remotemkdir", QtInfoMsg)

void PropagateRemoteMkdir::start()
{
    if (propagator()->_abortRequested)
        return;

    qCDebug(lcPropagateRemoteMkdir) << _item->_file;

    propagator()->_activeJobList.append(this);

    if (!_deleteExisting) {
        return slotStartMkcolJob();
    }

    _job = new DeleteJob(propagator()->account(),
        propagator()->fullRemotePath(_item->_file),
        this);
    connect(_job, SIGNAL(finishedSignal()), SLOT(slotStartMkcolJob()));
    _job->start();
}

void PropagateRemoteMkdir::slotStartMkcolJob()
{
    if (propagator()->_abortRequested)
        return;

    qCDebug(lcPropagateRemoteMkdir) << _item->_file;

    _job = new MkColJob(propagator()->account(),
        propagator()->fullRemotePath(_item->_file),
        this);
    connect(_job, SIGNAL(finished(QNetworkReply::NetworkError)), this, SLOT(slotMkcolJobFinished()));
    _job->start();
}

void PropagateRemoteMkdir::abort(PropagatorJob::AbortType abortType)
{
    if (_job && _job->reply())
        _job->reply()->abort();

    if (abortType == AbortType::Asynchronous) {
        emit abortFinished();
    }
}

void PropagateRemoteMkdir::setDeleteExisting(bool enabled)
{
    _deleteExisting = enabled;
}

void PropagateRemoteMkdir::slotMkcolJobFinished()
{
    propagator()->_activeJobList.removeOne(this);

    OC_ASSERT(_job);

    QNetworkReply::NetworkError err = _job->reply()->error();
    _item->_httpErrorCode = _job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    _item->_responseTimeStamp = _job->responseTimestamp();
    _item->_requestId = _job->requestId();

    if (_item->_httpErrorCode == 405) {
        // This happens when the directory already exists. Nothing to do.
    } else if (err != QNetworkReply::NoError) {
        SyncFileItem::Status status = classifyError(err, _item->_httpErrorCode,
            &propagator()->_anotherSyncNeeded);
        done(status, _job->errorString());
        return;
    } else if (_item->_httpErrorCode != 201) {
        // Normally we expect "201 Created"
        // If it is not the case, it might be because of a proxy or gateway intercepting the request, so we must
        // throw an error.
        done(SyncFileItem::NormalError,
            tr("Wrong HTTP code returned by server. Expected 201, but received \"%1 %2\".")
                .arg(_item->_httpErrorCode)
                .arg(_job->reply()->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
        return;
    }

    _item->_fileId = _job->reply()->rawHeader("OC-FileId");

    if (_item->_fileId.isEmpty()) {
        // Owncloud 7.0.0 and before did not have a header with the file id.
        // (https://github.com/owncloud/core/issues/9000)
        // So we must get the file id using a PROPFIND
        // This is required so that we can detect moves even if the folder is renamed on the server
        // while files are still uploading
        propagator()->_activeJobList.append(this);
        auto propfindJob = new PropfindJob(_job->account(), _job->path(), this);
        propfindJob->setProperties(QList<QByteArray>() << "http://owncloud.org/ns:id");
        QObject::connect(propfindJob, &PropfindJob::result, this, &PropagateRemoteMkdir::propfindResult);
        QObject::connect(propfindJob, &PropfindJob::finishedWithError, this, &PropagateRemoteMkdir::propfindError);
        propfindJob->start();
        _job = propfindJob;
        return;
    }
    success();
}

void PropagateRemoteMkdir::propfindResult(const QVariantMap &result)
{
    propagator()->_activeJobList.removeOne(this);
    if (result.contains(QStringLiteral("id"))) {
        _item->_fileId = result[QStringLiteral("id")].toByteArray();
    }
    success();
}

void PropagateRemoteMkdir::propfindError()
{
    // ignore the PROPFIND error
    propagator()->_activeJobList.removeOne(this);
    done(SyncFileItem::Success);
}

void PropagateRemoteMkdir::success()
{
    // Never save the etag on first mkdir.
    // Only fully propagated directories should have the etag set.
    auto itemCopy = *_item;
    itemCopy._etag.clear();

    // save the file id already so we can detect rename or remove
    // also convert to a placeholder for the proper behaviour of the file
    Q_ASSERT(thread() == qApp->thread());
    const auto result = propagator()->updateMetadata(itemCopy);
    if (!result) {
        done(SyncFileItem::FatalError, tr("Error writing metadata to the database: %1").arg(result.error()));
        return;
    }
#ifdef Q_OS_WIN
    else if (*result == Vfs::ConvertToPlaceholderResult::Locked) {
        // updateMetadata invokes convertToPlaceholder
        // On Windows convertToPlaceholder can be blocked by the OS
        // in case the file lock is active.
        // Retry the conversion for UpdateMetaDataRetyTimeOut
        // TODO: make update updateMetadata async in general

        // QtConcurrent does not support moves
        // use shared_ptr to manage the Result object
        using resultType = decltype(result);
        using resultTypePtr = decltype(std::shared_ptr<resultType>());

        auto *watcher = new QFutureWatcher<resultTypePtr>(this);
        connect(watcher, &QFutureWatcherBase::finished, this, [watcher, this] {
            const auto *result = watcher->result().get();
            if (*result) {
                switch (**result) {
                case Vfs::ConvertToPlaceholderResult::Ok:
                    done(SyncFileItem::Success);
                    return;
                case Vfs::ConvertToPlaceholderResult::Locked:
                    done(SyncFileItem::SoftError, tr("Setting file status failed due to file lock"));
                    return;
                case Vfs::ConvertToPlaceholderResult::Error:
                    Q_UNREACHABLE();
                }
            }
            done(SyncFileItem::FatalError, result->error());
        });

        watcher->setFuture(QtConcurrent::run([p = propagator(), itemCopy] {
            // Try to update the meta data with a 30s timeout
            const auto start = std::chrono::steady_clock::now();
            while ((std::chrono::steady_clock::now() - start) < UpdateMetaDataRetyTimeOut) {
                QThread::sleep(1);
                auto result = std::make_shared<resultType>(p->updateMetadata(itemCopy));
                if (*result) {
                    if (**result == Vfs::ConvertToPlaceholderResult::Locked) {
                        continue;
                    } else {
                        return result;
                    }
                } else {
                    return result;
                }
            }
            return std::make_shared<resultType>(Vfs::ConvertToPlaceholderResult::Locked);
        }));
        return;
    }
#endif
    done(SyncFileItem::Success);
}
}
