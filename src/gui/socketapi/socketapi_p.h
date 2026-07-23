/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QBitArray>
#include <QPointer>

#include <QJsonObject>

namespace OCC {

class BloomFilter
{
    // Initialize with m=1024 bits and k=2 (high and low 16 bits of a qHash).
    // For a client navigating in less than 100 directories, this gives us a probability less than
    // (1-e^(-2*100/1024))^2 = 0.03147872136 false positives.
    const static int NumBits = 1024;

public:
    BloomFilter()
        : hashBits(NumBits)
    {
    }

    void storeHash(size_t hash)
    {
        hashBits.setBit((hash & 0xFFFF) % NumBits);
        hashBits.setBit((hash >> 16) % NumBits);
    }
    bool isHashMaybeStored(size_t hash) const { return hashBits.testBit((hash & 0xFFFF) % NumBits) && hashBits.testBit((hash >> 16) % NumBits); }

private:
    QBitArray hashBits;
};

class SocketListener
{
public:
    QPointer<QIODevice> socket;

    explicit SocketListener(QIODevice *_socket)
        : socket(_socket)
    {
    }

    void sendMessage(const QString &message, bool doWait = false) const;
    void sendWarning(const QString &message, bool doWait = false) const
    {
        sendMessage(QStringLiteral("WARNING:") + message, doWait);
    }
    void sendError(const QString &message, bool doWait = false) const
    {
        sendMessage(QStringLiteral("ERROR:") + message, doWait);
    }

    void sendMessageIfDirectoryMonitored(const QString &message, size_t systemDirectoryHash) const
    {
        if (_monitoredDirectoriesBloomFilter.isHashMaybeStored(systemDirectoryHash))
            sendMessage(message, false);
    }

    void registerMonitoredDirectory(size_t systemDirectoryHash) { _monitoredDirectoriesBloomFilter.storeHash(systemDirectoryHash); }

private:
    BloomFilter _monitoredDirectoriesBloomFilter;
};

class SocketApiJobV2 : public QObject
{
    Q_OBJECT
public:
    explicit SocketApiJobV2(const QSharedPointer<SocketListener> &socketListener, const QString &command, const QJsonObject &arguments);

    void success(const QJsonObject &response) const;
    void failure(const QString &error) const;

    const QJsonObject &arguments() const { return _arguments; }
    QString command() const { return _command; }

    QString warning() const;
    void setWarning(const QString &warning);

Q_SIGNALS:
    void finished() const;

private:
    void doFinish(const QJsonObject &obj) const;

    QSharedPointer<SocketListener> _socketListener;
    const QString _command;
    QString _jobId;
    QJsonObject _arguments;
    QString _warning;
};
}

Q_DECLARE_METATYPE(OCC::SocketListener *)
