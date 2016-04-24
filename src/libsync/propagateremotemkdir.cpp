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
#include "syncjournalfilerecord.h"
#include "propagateremotedelete.h"
#include <QFile>

namespace OCC {

void PropagateRemoteMkdir::start()
{
    if (_propagator->_abortRequested.fetchAndAddRelaxed(0))
        return;

    qDebug() << Q_FUNC_INFO << _item->_file;

    _propagator->_activeJobList.append(this);

    if (!_deleteExisting) {
        return slotStartMkcolJob();
    }

    _job = new DeleteJob(_propagator->account(),
                         _propagator->_remoteFolder + _item->_file,
                         this);
    connect(_job, SIGNAL(finishedSignal()), SLOT(slotStartMkcolJob()));
    _job->start();
}

void PropagateRemoteMkdir::slotStartMkcolJob()
{
    if (_propagator->_abortRequested.fetchAndAddRelaxed(0))
        return;

    qDebug() << Q_FUNC_INFO << _item->_file;

    _job = new MkColJob(_propagator->account(),
                        _propagator->_remoteFolder + _item->_file,
                        this);
    connect(_job, SIGNAL(finished(QNetworkReply::NetworkError)), this, SLOT(slotMkcolJobFinished()));
    _job->start();
}

void PropagateRemoteMkdir::abort()
{
    if (_job)
        _job->abortNetworkReply();
}

void PropagateRemoteMkdir::setDeleteExisting(bool enabled)
{
    _deleteExisting = enabled;
}

void PropagateRemoteMkdir::slotMkcolJobFinished()
{
    _propagator->_activeJobList.removeOne(this);

    Q_ASSERT(_job);

    QNetworkReply::NetworkError err = _job->getError();

    qDebug() << Q_FUNC_INFO << _job->getUrl() << "FINISHED WITH STATUS"
        << err
        << (err == QNetworkReply::NoError ? QLatin1String("") : _job->getErrorString());

    _item->_httpErrorCode = _job->getHttpStatusCode();

    if (_item->_httpErrorCode == 405) {
        // This happens when the directory already exists. Nothing to do.
    } else if (err != QNetworkReply::NoError) {
        SyncFileItem::Status status = classifyError(err, _item->_httpErrorCode,
                                                    &_propagator->_anotherSyncNeeded);
        auto errorString = _job->getErrorString();
        if (_job->getHasOCErrorString()) {
            errorString = _job->getOCErrorString();
        }
        done(status, errorString);
        return;
    } else if (_item->_httpErrorCode != 201) {
        // Normally we expect "201 Created"
        // If it is not the case, it might be because of a proxy or gateway intercepting the request, so we must
        // throw an error.
        done(SyncFileItem::NormalError, tr("Wrong HTTP code returned by server. Expected 201, but received \"%1 %2\".")
            .arg(_item->_httpErrorCode).arg(_job->getHttpReasonPhrase()));
        return;
    }

    _item->_requestDuration = _job->duration();
    _item->_responseTimeStamp = _job->responseTimestamp();
    _item->_fileId = _job->getOCFileID();

    if (_item->_fileId.isEmpty()) {
        // Owncloud 7.0.0 and before did not have a header with the file id.
        // (https://github.com/owncloud/core/issues/9000)
        // So we must get the file id using a PROPFIND
        // This is required so that we can detect moves even if the folder is renamed on the server
        // while files are still uploading
        _propagator->_activeJobList.append(this);
        auto propfindJob = new PropfindJob(_job->account(), _job->path(), this);
        propfindJob->setProperties(QList<QByteArray>() << "getetag" << "http://owncloud.org/ns:id");
        QObject::connect(propfindJob, SIGNAL(result(QVariantMap)), this, SLOT(propfindResult(QVariantMap)));
        QObject::connect(propfindJob, SIGNAL(finishedWithError()), this, SLOT(propfindError()));
        propfindJob->start();
        _job = propfindJob;
        return;
    }
    success();
}

void PropagateRemoteMkdir::propfindResult(const QVariantMap &result)
{
    _propagator->_activeJobList.removeOne(this);
    if (result.contains("getetag")) {
        _item->_etag = result["getetag"].toByteArray();
    }
    if (result.contains("id")) {
        _item->_fileId = result["id"].toByteArray();
    }
    success();
}

void PropagateRemoteMkdir::propfindError()
{
    // ignore the PROPFIND error
    _propagator->_activeJobList.removeOne(this);
    done(SyncFileItem::Success);
}

void PropagateRemoteMkdir::success()
{
    // save the file id already so we can detect rename or remove
    SyncJournalFileRecord record(*_item, _propagator->_localDir + _item->destination());
    if (!_propagator->_journal->setFileRecord(record)) {
        done(SyncFileItem::FatalError, tr("Error writing metadata to the database"));
        return;
    }

    done(SyncFileItem::Success);
}



}

