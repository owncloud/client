/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "clientproxy.h"

#include "configfile.h"
#include <QLoggingCategory>
#include <QUrl>
#include <QThreadPool>

namespace OCC {

Q_LOGGING_CATEGORY(lcClientProxy, "sync.clientproxy", QtInfoMsg)

namespace {

    QNetworkProxy proxyFromConfig(const QString &password, const ConfigFile &cfg)
    {
        QNetworkProxy proxy;

        if (cfg.proxyHostName().isEmpty())
            return QNetworkProxy();

        proxy.setHostName(cfg.proxyHostName());
        proxy.setPort(cfg.proxyPort());
        if (cfg.proxyNeedsAuth()) {
            proxy.setUser(cfg.proxyUser());
            proxy.setPassword(password);
        }
        return proxy;
    }

}

QString ClientProxy::printQNetworkProxy(const QNetworkProxy &proxy)
{
    return QStringLiteral("%1://%2:%3").arg(proxy.type()).arg(proxy.hostName()).arg(proxy.port());
}

bool ClientProxy::isUsingSystemDefault()
{
    OCC::ConfigFile cfg;

    // if there is no config file, default to system proxy.
    if (cfg.exists()) {
        return cfg.proxyType() == QNetworkProxy::DefaultProxy;
    }

    return false;
}

void ClientProxy::setupQtProxyFromConfig(const QString &password)
{
    OCC::ConfigFile cfg;
    int proxyType = QNetworkProxy::DefaultProxy;
    QNetworkProxy proxy;

    // if there is no config file, default to system proxy.
    if (cfg.exists()) {
        proxyType = cfg.proxyType();
        proxy = proxyFromConfig(password, cfg);
    }

    switch (proxyType) {
    case QNetworkProxy::NoProxy:
        qCInfo(lcClientProxy) << "Set proxy configuration to use NO proxy";
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;
    case QNetworkProxy::DefaultProxy:
        qCInfo(lcClientProxy) << "Set proxy configuration to use system configuration";
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        break;
    case QNetworkProxy::Socks5Proxy:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        qCInfo(lcClientProxy) << "Set proxy configuration to SOCKS5" << printQNetworkProxy(proxy);
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::setApplicationProxy(proxy);
        break;
    case QNetworkProxy::HttpProxy:
        proxy.setType(QNetworkProxy::HttpProxy);
        qCInfo(lcClientProxy) << "Set proxy configuration to HTTP" << printQNetworkProxy(proxy);
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::setApplicationProxy(proxy);
        break;
    default:
        break;
    }
}

void ClientProxy::lookupSystemProxyAsync(const QUrl &url, QObject *dst, const char *slot)
{
    SystemProxyRunnable *runnable = new SystemProxyRunnable(url);
    QObject::connect(runnable, SIGNAL(systemProxyLookedUp(QNetworkProxy)), dst, slot);
    QThreadPool::globalInstance()->start(runnable); // takes ownership and deletes
}

SystemProxyRunnable::SystemProxyRunnable(const QUrl &url)
    : QObject()
    , QRunnable()
    , _url(url)
{
}

void SystemProxyRunnable::run()
{
    qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
    QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(_url));

    if (proxies.isEmpty()) {
        Q_EMIT systemProxyLookedUp(QNetworkProxy(QNetworkProxy::NoProxy));
    } else {
        Q_EMIT systemProxyLookedUp(proxies.first());
        // FIXME Would we really ever return more?
    }
}
}
