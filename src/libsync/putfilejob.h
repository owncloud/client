/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
    explicit PUTFileJob(
        Account *account, const QUrl &url, const QString &path, std::unique_ptr<QIODevice> &&device, const HeaderMap &headers, QObject *parent = nullptr);
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
