/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
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
#pragma once

#include "owncloudpropagator.h"
#include "networkjobs.h"

#include <QHttpMultiPart>
#include <QFile>
#include <QDebug>

namespace OCC {

/**
 * @brief The MultipartJob class
 * @ingroup libsync
 */
class MultipartJob : public AbstractNetworkJob {
    Q_OBJECT

private:
    QHttpMultiPart* _multipart;

    // Vector_syncItems is used in constructing a response at the response arrival
    QList<SyncFileItemPtr> _syncItems;

    // Vector containing QHttpParts which will be later inserted into request body
    QList<QHttpPart> _syncParts;

    // error string
    QString _errorString;
public:
    QElapsedTimer _duration;
    // Takes ownership of the device
    explicit MultipartJob(AccountPtr account, const QString& path, QHttpMultiPart* multipart, QObject* parent = 0)
        : AbstractNetworkJob(account, path, parent), _multipart(multipart), _errorString(QString()) {}
    ~MultipartJob();

    virtual void start() Q_DECL_OVERRIDE;

    virtual bool finished() Q_DECL_OVERRIDE {
        emit finishedSignal();
        return true;
    }

    void addItemPart(const QHttpPart &itemPart, const SyncFileItemPtr &item);

    void addRootPart(const QHttpPart &itemPart);

    bool isEmpty() {
        return _syncItems.isEmpty();
    }

    QString errorString() {
        return _errorString.isEmpty() ? reply()->errorString() : _errorString;
    }

    QList<SyncFileItemPtr> syncItems() const {
        return _syncItems;
    }

    virtual void slotTimeout() Q_DECL_OVERRIDE;


signals:
    void finishedSignal();

private slots:
#if QT_VERSION < 0x050402
    void slotSoftAbort();
#endif
};

/**
 * @brief The PropagateBundle class
 * @ingroup libsync
 */
class PropagateBundle : public PropagateItemJob {
    Q_OBJECT

private:
    QLinkedList<SyncFileItemPtr> _itemsToSync;
    QLinkedList<SyncFileItemPtr> _itemsToChecksum;
    QVector<MultipartJob*> _jobs; /// network jobs that are currently in transit
    bool _preparingBundle; // Flag blocking processing the same files by many threads
    quint64 _size; // Tells what is the total size of _itemsToSync inside the bundle
    quint64 _currentBundleSize;
    quint64 _currentRequestsNumber;
    quint64 _totalFiles;
    // measure the performance of checksum calc and upload
    Utility::StopWatch _stopWatch;

public:
    PropagateBundle(OwncloudPropagator* propagator)
        : PropagateItemJob(propagator, SyncFileItemPtr(new SyncFileItem)), _preparingBundle(true), _size(0), _currentBundleSize(0), _currentRequestsNumber(0) {}
    void start() Q_DECL_OVERRIDE;
    void startBundle();
    void append(const SyncFileItemPtr &bundledFile);
    QByteArray getRemotePath(QString filePath);
    bool empty();
    bool scheduleNextJob() Q_DECL_OVERRIDE {
        if (_state == NotYetStarted){
            _state = Running;
            QMetaObject::invokeMethod(this, "start");
            return true;
        }
        return false;
    }
    quint64 syncItemsSize() const { return _size; }
    quint64 syncItemsNumber() const { return _itemsToChecksum.count(); }
private slots:
    void slotComputeTransmissionChecksum();
    void slotStartUpload(const QByteArray& transmissionChecksumType, const QByteArray& transmissionChecksum);
    void slotMultipartFinished();
    void slotJobDestroyed(QObject *job);
    void abortWithError(SyncFileItem::Status status, const QString &error);

private:
    quint64 checkBundledRequestsLimits()
    {
        //TODO: obtain this value from the server or by other means
        quint64 maximumBundledFiles = 500;
        return (maximumBundledFiles/_propagator->maximumActiveJob());
    }
    quint64 chunkSize() const { return _propagator->chunkSize(); }
    int getHttpStatusCode(const QString &status);
    void slotItemFinished(const SyncFileItemPtr &item, QMap<QString, QMap<QString, QString> > &responseObjectsProperties);
};

}
