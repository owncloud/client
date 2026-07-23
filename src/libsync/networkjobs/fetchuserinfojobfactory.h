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
namespace OCC {

class OWNCLOUDSYNC_EXPORT FetchUserInfoResult
{
public:
    FetchUserInfoResult() = default;

    FetchUserInfoResult(const QString &userName, const QString &displayName)
    {
        _userName = userName;
        _displayName = displayName;
    }

    QString userName() const
    {
        return _userName;
    }

    QString displayName() const
    {
        return _displayName;
    };

private:
    QString _userName;
    QString _displayName;
};

class OWNCLOUDSYNC_EXPORT FetchUserInfoJobFactory : public AbstractCoreJobFactory
{
public:
    static FetchUserInfoJobFactory fromOAuth2Credentials(QNetworkAccessManager *nam, const QString &bearerToken);

    CoreJob *startJob(const QUrl &url, QObject *parent) override;

private:
    FetchUserInfoJobFactory(QNetworkAccessManager *nam, const QString &authHeaderValue);

    QString _authorizationHeader;
};

} // OCC

Q_DECLARE_METATYPE(OCC::FetchUserInfoResult)
