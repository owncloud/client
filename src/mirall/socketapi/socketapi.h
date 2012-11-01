/*
 * Copyright (C) by Dominik Schmidt <dev@dominik-schmidt.de>
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


#ifndef SOCKETAPI_H
#define SOCKETAPI_H

#include <attica/provider.h>
#include <QWeakPointer>

namespace Attica {
    class ProviderManager;
};

class QUrl;
class QLocalSocket;
class QLocalServer;
class QStringList;

namespace Mirall {

class FolderMan;

class SocketApi : public QObject
{
Q_OBJECT

public:
    SocketApi(QObject* parent, const QUrl& localFile, FolderMan* folderMan);
    virtual ~SocketApi();

private slots:
    void onNewConnection();
    void onLostConnection();
    void onReadyRead();
    void onSyncStateChanged(const QString&);

    void onProviderAdded(const Attica::Provider& provider);
    void onGotPublicShareLink(Attica::BaseJob*);

private:
    void sendMessage(QLocalSocket* socket, const QString& message);
    void broadcastMessage(const QString& message);

    Q_INVOKABLE void command_RETRIEVE_FOLDER_STATUS(const QString& argument, QLocalSocket* socket);
    Q_INVOKABLE void command_PUBLIC_SHARE_LINK(const QString& argument, QLocalSocket* socket);

private:
    QLocalServer* _localServer;
    FolderMan* _folderMan;
    QList< QLocalSocket* > _listeners;

    QString _remotePath;
    Attica::ProviderManager* _atticaManager;
    Attica::Provider _atticaProvider;
};

}
#endif // SOCKETAPI_H
