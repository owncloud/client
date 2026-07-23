/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "abstractcorejob.h"

#include <QJsonObject>

namespace OCC {

class OWNCLOUDSYNC_EXPORT CheckServerJobResult
{

public:
    CheckServerJobResult();
    CheckServerJobResult(const QJsonObject &statusObject, const QUrl &serverUrl);

    QJsonObject statusObject() const;
    QUrl serverUrl() const;

private:
    const QJsonObject _statusObject;
    const QUrl _serverUrl;
};


class OWNCLOUDSYNC_EXPORT CheckServerJobFactory : public AbstractCoreJobFactory
{
public:
    using AbstractCoreJobFactory::AbstractCoreJobFactory;

    /**
     * clearCookies: Whether to clear the cookies before we start the CheckServerJob job
     */
    static CheckServerJobFactory createFromAccount(const Account *account, bool clearCookies);

    // the parent arg should be a fallback - we should be deleting jobs explicitly as soon as they are finished
    // also note we create a "one off" nam for each checkServerJob so that is parented by the job for ensured cleanup
    CoreJob *startJob(const QUrl &url, QObject *parent) override;

};

} // OCC

Q_DECLARE_METATYPE(OCC::CheckServerJobResult)
