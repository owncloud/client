/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CLIENTPROXY_H
#define CLIENTPROXY_H

#include <QObject>
#include <QNetworkProxy>
#include <QRunnable>
#include <QUrl>

#include "common/utility.h"
#include "csync.h"

namespace OCC {

class ConfigFile;

/**
 * @brief The ClientProxy class
 * @ingroup libsync
 */
namespace ClientProxy {
    bool isUsingSystemDefault();
    void lookupSystemProxyAsync(const QUrl &url, QObject *dst, const char *slot);
    void setupQtProxyFromConfig(const QString &password);

    QString printQNetworkProxy(const QNetworkProxy &proxy);
};

class SystemProxyRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    SystemProxyRunnable(const QUrl &url);
    void run() override;
Q_SIGNALS:
    void systemProxyLookedUp(const QNetworkProxy &url);

private:
    QUrl _url;
};
}

#endif // CLIENTPROXY_H
