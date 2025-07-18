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
#pragma once

#include "networkjobs.h"

#include <QElapsedTimer>

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcPutJob)

/**
 * @brief The PUTFileJob class
 * @ingroup libsync
 */
class PUTFileJob : public AbstractNetworkJob
{
    Q_OBJECT

private:
    QIODevice *_device;
    QMap<QByteArray, QByteArray> _headers;
    QString _errorString;
    QElapsedTimer _requestTimer;

public:
    explicit PUTFileJob(AccountPtr account, const QUrl &url, const QString &path, std::unique_ptr<QIODevice> &&device,
        const HeaderMap &headers, QObject *parent = nullptr);
    ~PUTFileJob() override;

    void start() override;

    void finished() override;

    QIODevice *device()
    {
        return _device;
    }

    QString errorString()
    {
        return _errorString.isEmpty() ? AbstractNetworkJob::errorString() : _errorString;
    }

    std::chrono::milliseconds msSinceStart() const
    {
        return std::chrono::milliseconds(_requestTimer.elapsed());
    }

protected:
    void newReplyHook(QNetworkReply *reply) override;

Q_SIGNALS:
    void uploadProgress(qint64, qint64);

};

}
