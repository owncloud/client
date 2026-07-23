/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "propagateremotedelete.h"
#include "account.h"
#include "common/asserts.h"
#include "owncloudpropagator_p.h"

#include <QLoggingCategory>

namespace OCC {

Q_LOGGING_CATEGORY(lcDeleteJob, "sync.networkjob.delete", QtInfoMsg)
Q_LOGGING_CATEGORY(lcPropagateRemoteDelete, "sync.propagator.remotedelete", QtInfoMsg)

DeleteJob::DeleteJob(Account *account, const QUrl &url, const QString &path, QObject *parent)
    : AbstractNetworkJob(account, url, path, parent)
{
}

void DeleteJob::start()
{
    sendRequest("DELETE");
    AbstractNetworkJob::start();
}

void DeleteJob::finished()
{
    qCInfo(lcDeleteJob) << "DELETE of" << reply()->request().url() << "FINISHED WITH STATUS"
                       << replyStatusString();
}

void PropagateRemoteDelete::start()
{
    if (propagator()->_abortRequested)
        return;

    qCDebug(lcPropagateRemoteDelete) << _item->_file;

    _job = new DeleteJob(propagator()->account(), propagator()->webDavUrl(), propagator()->fullRemotePath(_item->_file), this);
    connect(_job.data(), &DeleteJob::finishedSignal, this, &PropagateRemoteDelete::slotDeleteJobFinished);
    propagator()->_activeJobList.append(this);
    _job->start();
}

void PropagateRemoteDelete::abort(PropagatorJob::AbortType abortType)
{
    if (_job) {
        _job->abort();
    }
    if (abortType == AbortType::Asynchronous) {
        Q_EMIT abortFinished();
    }
}

void PropagateRemoteDelete::slotDeleteJobFinished()
{
    propagator()->_activeJobList.removeOne(this);

    OC_ASSERT(_job);

    QNetworkReply::NetworkError err = _job->reply()->error();
    const int httpStatus = _job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    _item->_httpErrorCode = httpStatus;
    _item->_responseTimeStamp = _job->responseTimestamp();
    _item->_requestId = _job->requestId();

    if (err != QNetworkReply::NoError && err != QNetworkReply::ContentNotFoundError) {
        SyncFileItem::Status status = classifyError(err, _item->_httpErrorCode,
            &propagator()->_anotherSyncNeeded);
        done(status, _job->errorString());
        return;
    }

    // A 404 reply is also considered a success here: We want to make sure
    // a file is gone from the server. It not being there in the first place
    // is ok. This will happen for files that are in the DB but not on
    // the server or the local file system.
    if (httpStatus != 204 && httpStatus != 404) {
        // Normally we expect "204 No Content"
        // If it is not the case, it might be because of a proxy or gateway intercepting the request, so we must
        // throw an error.
        done(SyncFileItem::NormalError,
            tr("Wrong HTTP code returned by server. Expected 204, but received \"%1 %2\".")
                .arg(_item->_httpErrorCode)
                .arg(_job->reply()->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
        return;
    }

    propagator()->_journal->deleteFileRecord(_item->_originalFile, _item->isDirectory());
    propagator()->_journal->commit(QStringLiteral("Remote Remove"));
    done(SyncFileItem::Success);
}
}
