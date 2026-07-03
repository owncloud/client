/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "owncloudlib.h"

#include <QJsonObject>
#include <QMimeType>
#include <QObject>
#include <QUrl>
#include <QVariantMap>

namespace OCC {

class Account;

class OWNCLOUDSYNC_EXPORT AppProvider
{
public:
    struct OWNCLOUDSYNC_EXPORT Provider
    {
        // the server might provide multiple apps but no default
        // for now we only support default apps
        Provider() = default;
        explicit Provider(const QJsonObject &provider);
        QString mimeType;
        QString extension;
        QString name;
        QString description;
        QUrl icon;
        QString defaultApplication;
        bool allowCreation = false;

        bool isValid() const;
    };

    explicit AppProvider(const QJsonObject &apps = {});

    const Provider &app(const QMimeType &mimeType) const;
    const Provider &app(const QString &localPath) const;

    bool open(Account *account, const QString &localPath, const QByteArray &fileId) const;


private:
    QHash<QString, Provider> _providers;
};

}
