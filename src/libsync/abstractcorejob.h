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

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "owncloudlib.h"

namespace OCC {

class OWNCLOUDSYNC_EXPORT Job : public QObject
{
    Q_OBJECT

    friend class AbstractCoreJobFactory;

public:
    explicit Job(QObject *parent = nullptr);

    [[nodiscard]] const QVariant &result() const;

    [[nodiscard]] const QString &errorMessage() const;
    [[nodiscard]] QNetworkReply::NetworkError networkError() const;

    [[nodiscard]] bool success() const;

protected:
    void finishWithResult(const QVariant &result);

    void finishWithError(const QString &errorMessage, QNetworkReply::NetworkError networkError);

Q_SIGNALS:
    void finished();

private:
    bool _success = false;
    QVariant _result;

    QString _errorMessage;
    QNetworkReply::NetworkError _networkError = QNetworkReply::NoError;
};

class OWNCLOUDSYNC_EXPORT AbstractCoreJobFactory : public QObject
{
    Q_OBJECT

public:
    explicit AbstractCoreJobFactory(QNetworkAccessManager *nam, QObject *parent = nullptr);
    ~AbstractCoreJobFactory() override;

    virtual Job *startJob(const QUrl &url) = 0;

protected:
    [[nodiscard]] QNetworkAccessManager *nam() const;

    static void finishJobWithSuccess(Job *job, const QVariant &result)
    {
        job->finishWithResult(result);
    }

    static void finishJobWithError(Job *job, const QString &errorMessage, const QNetworkReply::NetworkError networkError)
    {
        job->finishWithError(errorMessage, networkError);
    }

private:
    QNetworkAccessManager *_nam;
};

}
