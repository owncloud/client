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
#include "putfilejob.h"
#include "syncengine.h"

#include "common/asserts.h"
#include "common/utility.h"

#include "libsync/theme.h"

#include <cmath>

using namespace std::chrono_literals;

namespace OCC {

Q_LOGGING_CATEGORY(lcPutJob, "sync.networkjob.put", QtInfoMsg)

PUTFileJob::PUTFileJob(
    Account *account, const QUrl &url, const QString &path, std::unique_ptr<QIODevice> &&device, const QMap<QByteArray, QByteArray> &headers, QObject *parent)
    : AbstractNetworkJob(account, url, path, parent)
    , _device(device.release())
    , _headers(headers)
{
    _device->setParent(this);
    // Long uploads must not block non-propagation jobs.
    setPriority(QNetworkRequest::LowPriority);
}

PUTFileJob::~PUTFileJob()
{
}

void PUTFileJob::start()
{
    QNetworkRequest req;
    for (auto it = _headers.cbegin(); it != _headers.cend(); ++it) {
        req.setRawHeader(it.key(), it.value());
    }
    sendRequest("PUT", req, _device);
    _requestTimer.start();
    AbstractNetworkJob::start();
}

void PUTFileJob::finished()
{
    _device->close();

    qCInfo(lcPutJob) << "PUT of" << reply()->request().url().toString() << "FINISHED WITH STATUS" << replyStatusString()
                     << reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                     << reply()->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
}

void PUTFileJob::newReplyHook(QNetworkReply *reply)
{
    connect(reply, &QNetworkReply::uploadProgress, this, &PUTFileJob::uploadProgress);
}
}
