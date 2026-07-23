/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
