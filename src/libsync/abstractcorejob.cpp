/*
 * Copyright (C) Hannah von Reth <hannah.vonreth@owncloud.com>
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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

#include "abstractcorejob.h"

using namespace OCC;

AbstractCoreJobFactory::AbstractCoreJobFactory(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , _nam(nam)
{
}

AbstractCoreJobFactory::~AbstractCoreJobFactory()
{
}

QNetworkAccessManager *AbstractCoreJobFactory::nam() const
{
    return _nam;
}

const QVariant &Job::result() const
{
    return _result;
}

const QString &Job::errorMessage() const
{
    return _errorMessage;
}

QNetworkReply::NetworkError Job::networkError() const
{
    return _networkError;
}

bool Job::success() const
{
    return _success;
}

void Job::finishWithResult(const QVariant &result)
{
    _success = true;
    _result = result;
    Q_EMIT finished();
}

void Job::finishWithError(const QString &errorMessage, const QNetworkReply::NetworkError networkError)
{
    _errorMessage = errorMessage;
    _networkError = networkError;
    Q_EMIT finished();
}

Job::Job(QObject *parent)
    : QObject(parent)
{
}
