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


#include "socketapi.h"

#include "../mirallconfigfile.h"
#include "../folderman.h"
#include "../folder.h"
#include "../owncloudfolder.h"

#include <QDebug>
#include <QUrl>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMetaObject>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <sys/socket.h>

using namespace Mirall;

#define DEBUG qDebug() << "SocketApi: "

SocketApi::SocketApi(QObject* parent, const QUrl& localFile, FolderMan* folderMan)
    : QObject(parent)
    , _localServer(0)
    , _folderMan(folderMan)
{
    QString socketPath = localFile.toLocalFile();
    DEBUG << "ctor: " << socketPath;

    // setup socket
    _localServer = new QLocalServer(this);
    QLocalServer::removeServer(socketPath);
    if(!_localServer->listen(socketPath))
        DEBUG << "can't start server";
    else
        DEBUG << "server started";
    connect(_localServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    // folder watcher
    connect(_folderMan, SIGNAL(folderSyncStateChange(QString)), SLOT(onSyncStateChanged(QString)));
}

SocketApi::~SocketApi()
{
    DEBUG << "dtor";
    _localServer->close();
}

void SocketApi::onNewConnection()
{
    QLocalSocket* socket = _localServer->nextPendingConnection();
    DEBUG << "New connection " << socket;
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(onLostConnection()));
    Q_ASSERT(socket->readAll().isEmpty());

    _listeners.append(socket);
}

void SocketApi::onLostConnection()
{
    DEBUG << "Lost connection " << sender();

    QLocalSocket* socket = qobject_cast< QLocalSocket* >(sender());
    _listeners.removeAll(socket);
}


void SocketApi::onReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    Q_ASSERT(socket);

    while(socket->canReadLine())
    {
        QString line = QString(socket->readLine()).trimmed();
        QString command = line.split(":").first();
        QString function = QString(QLatin1String("command_")).append(command);

        QString functionWithArguments = function + QLatin1String("(QString,QLocalSocket*)");
        int indexOfMethod = this->metaObject()->indexOfMethod(functionWithArguments.toAscii());

        QString argument = line.remove(0, command.length()+1).trimmed();
        if(indexOfMethod != -1)
        {
            QMetaObject::invokeMethod(this, function.toAscii(), Q_ARG(QString, argument), Q_ARG(QLocalSocket*, socket));
        }
        else
        {
            DEBUG << "The command is not supported by this version of the client:" << command << "with argument:" << argument;
        }
    }
}

void SocketApi::onSyncStateChanged(const QString&)
{
    DEBUG << "Sync state changed";

    broadcastMessage("UPDATE_VIEW");
}


void SocketApi::sendMessage(QLocalSocket* socket, const QString& message)
{
    DEBUG << "Sending message: " << message;
    QString localMessage = message;
    socket->write(localMessage.append("\n").toUtf8());
}

void SocketApi::broadcastMessage(const QString& message)
{
    DEBUG << "Broadcasting to" << _listeners.count() << "listeners: " << message;
    foreach(QLocalSocket* current, _listeners)
    {
        sendMessage(current, message);
    }
}
