/******************************************************************************
 *   Copyright (C) 2014 by Olivier Goffart <ogoffart@woboq.com                *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation; either version 2 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA               *
 ******************************************************************************/

#include <QtNetwork/QLocalSocket>
#include <qcoreevent.h>
#include <QStandardPaths>
#include <QFile>
#include <QLoggingCategory>
#include "ownclouddolphinpluginhelper.h"
#include "config.h"
#include <QJsonObject>
#include <QJsonDocument>

Q_LOGGING_CATEGORY(lcPluginHelper, "owncloud.dolphin", QtInfoMsg)

OwncloudDolphinPluginHelper* OwncloudDolphinPluginHelper::instance()
{
    static OwncloudDolphinPluginHelper self;
    return &self;
}

OwncloudDolphinPluginHelper::OwncloudDolphinPluginHelper()
{
    connect(&_socket, &QLocalSocket::connected, this, &OwncloudDolphinPluginHelper::slotConnected);
    connect(&_socket, &QLocalSocket::readyRead, this, &OwncloudDolphinPluginHelper::slotReadyRead);
    _connectTimer.start(45 * 1000, Qt::VeryCoarseTimer, this);
    tryConnect();
}

void OwncloudDolphinPluginHelper::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == _connectTimer.timerId()) {
        tryConnect();
        return;
    }
    QObject::timerEvent(e);
}

bool OwncloudDolphinPluginHelper::isConnected() const
{
    return _socket.state() == QLocalSocket::ConnectedState;
}

void OwncloudDolphinPluginHelper::sendCommand(const char* data)
{
    _socket.write(data);
    _socket.flush();
}

void OwncloudDolphinPluginHelper::sendGetClientIconCommand(int size)
{
    const QByteArray cmd = QByteArrayLiteral("V2/GET_CLIENT_ICON:");
    const QByteArray newLine = QByteArrayLiteral("\n");
    const QJsonObject args { { QStringLiteral("size"), size } };
    const QJsonObject obj { { QStringLiteral("id"), QString::number(_msgId++) }, { QStringLiteral("arguments"), args } };
    const auto json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    sendCommand(QByteArray(cmd + json + newLine));
}

void OwncloudDolphinPluginHelper::slotConnected()
{
    sendCommand("VERSION:\n");
    sendCommand("GET_STRINGS:\n");
}

void OwncloudDolphinPluginHelper::tryConnect()
{
    if (_socket.state() != QLocalSocket::UnconnectedState) {
        return;
    }

    QString socketPath = QStandardPaths::locate(QStandardPaths::RuntimeLocation,
                                                APPLICATION_SHORTNAME,
                                                QStandardPaths::LocateDirectory);
    if(socketPath.isEmpty())
        return;

    _socket.connectToServer(socketPath + QLatin1String("/socket"));
}

void OwncloudDolphinPluginHelper::slotReadyRead()
{
    while (_socket.bytesAvailable()) {
        _line += _socket.readLine();
        if (!_line.endsWith("\n"))
            continue;
        QByteArray line;
        qSwap(line, _line);
        line.chop(1);
        if (line.isEmpty())
            continue;

        if (line.startsWith("REGISTER_PATH:")) {
            auto col = line.indexOf(':');
            QString file = QString::fromUtf8(line.constData() + col + 1, line.size() - col - 1);
            _paths.append(file);
            continue;
        } else if (line.startsWith("STRING:")) {
            auto args = QString::fromUtf8(line).split(QLatin1Char(':'));
            if (args.size() >= 3) {
                _strings[args[1]] = args.mid(2).join(QLatin1Char(':'));
            }
            continue;
        } else if (line.startsWith("VERSION:")) {
            auto args = line.split(':');
            auto version = args.value(2);
            _version = version;
            if (!version.startsWith("1.")) {
                // Incompatible version, disconnect forever
                _connectTimer.stop();
                _socket.disconnectFromServer();
                return;
            }
        } else if (line.startsWith("V2/GET_CLIENT_ICON_RESULT:")) {
            line.remove(0, QStringLiteral("V2/GET_CLIENT_ICON_RESULT:").size());
            QJsonParseError error;
            auto json = QJsonDocument::fromJson(line, &error).object();
            if (error.error != QJsonParseError::NoError) {
                qCWarning(lcPluginHelper) << "Error while parsing result: " << error.error;
                continue;
            }

            auto jsonArgs = json.value("arguments").toObject();
            if (jsonArgs.isEmpty()) {
                qCWarning(lcPluginHelper) << "Error getting client icon: " << json.value("error").toObject();
                continue;
            }

            const QByteArray pngBase64 = jsonArgs.value("png").toString().toUtf8();
            QByteArray png = QByteArray::fromBase64(pngBase64);

            QPixmap pixmap;
            bool isLoaded = pixmap.loadFromData(png, "PNG");
            if (isLoaded) {
                _clientIcon = pixmap;
            }
        }

        emit commandRecieved(line);
    }
}
