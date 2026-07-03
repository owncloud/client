/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "abstractcorejob.h"
#include "common/asserts.h"

using namespace OCC;

AbstractCoreJobFactory::AbstractCoreJobFactory(QNetworkAccessManager *nam)
    : _nam(nam)
{
}

AbstractCoreJobFactory::~AbstractCoreJobFactory()
{
}

void AbstractCoreJobFactory::setJobResult(CoreJob *job, const QVariant &result)
{
    job->setResult(result);
}

void AbstractCoreJobFactory::setJobError(CoreJob *job, const QString &errorMessage)
{
    job->setError(errorMessage);
}

const QVariant &CoreJob::result() const
{
    return _result;
}

const QString &CoreJob::errorMessage() const
{
    return _errorMessage;
}

QNetworkReply *CoreJob::reply() const
{
    return _reply;
}

bool CoreJob::success() const
{
    return _success;
}

void CoreJob::setResult(const QVariant &result)
{
    if (OC_ENSURE(assertNotFinished())) {
        _success = true;
        _result = result;

        Q_EMIT finished();
    }
}

void CoreJob::setError(const QString &errorMessage)
{
    if (OC_ENSURE(assertNotFinished())) {
        _errorMessage = errorMessage;

        Q_EMIT finished();
    }
}

CoreJob::CoreJob(QNetworkReply *reply, QObject *parent)
    : QObject(parent)
    , _reply(reply)
{
    _reply->setParent(this);
}

bool CoreJob::assertNotFinished() const
{
    OC_ASSERT(_result.isNull());
    OC_ASSERT(_errorMessage.isEmpty());
    return _result.isNull() && _errorMessage.isEmpty();
}
