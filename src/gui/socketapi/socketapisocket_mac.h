/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SOCKETAPISOCKET_OSX_H
#define SOCKETAPISOCKET_OSX_H

#include <QAbstractSocket>
#include <QIODevice>

class SocketApiServerPrivate;
class SocketApiSocketPrivate;

class SocketApiSocket : public QIODevice
{
    Q_OBJECT
public:
    SocketApiSocket(QObject *parent, SocketApiSocketPrivate *p);
    ~SocketApiSocket() override;

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;
    bool canReadLine() const override;

Q_SIGNALS:
    void disconnected();

private:
    // Use Qt's p-impl system to hide objective-c types from C++ code including this file
    Q_DECLARE_PRIVATE(SocketApiSocket)
    QScopedPointer<SocketApiSocketPrivate> d_ptr;
    friend class SocketApiServerPrivate;
};

class SocketApiServer : public QObject
{
    Q_OBJECT
public:
    SocketApiServer();
    ~SocketApiServer() override;

    void close();
    bool listen(const QString &name);
    SocketApiSocket *nextPendingConnection();

    static bool removeServer(const QString &) { return false; }

Q_SIGNALS:
    void newConnection();

private:
    Q_DECLARE_PRIVATE(SocketApiServer)
    QScopedPointer<SocketApiServerPrivate> d_ptr;
};

#endif // SOCKETAPISOCKET_OSX_H
