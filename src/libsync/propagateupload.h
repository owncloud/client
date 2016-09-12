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

#include <QBuffer>
#include <QFile>
#include <QDebug>

namespace OCC {
class BandwidthManager;

/**
 * @brief The UploadDevice class
 * @ingroup libsync
 */
class UploadDevice : public QIODevice {
    Q_OBJECT
public:
    UploadDevice(BandwidthManager *bwm);
    ~UploadDevice();

    /** Reads the data from the file and opens the device */
    bool prepareAndOpen(const QString& fileName, qint64 start, qint64 size);

    qint64 writeData(const char* , qint64 ) Q_DECL_OVERRIDE;
    qint64 readData(char* data, qint64 maxlen) Q_DECL_OVERRIDE;
    bool atEnd() const Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;
    qint64 bytesAvailable() const Q_DECL_OVERRIDE;
    bool isSequential() const Q_DECL_OVERRIDE;
    bool seek ( qint64 pos ) Q_DECL_OVERRIDE;

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
    bool reset() Q_DECL_OVERRIDE { emit wasReset(); return QIODevice::reset(); }
#endif

    void setBandwidthLimited(bool);
    bool isBandwidthLimited() { return _bandwidthLimited; }
    void setChoked(bool);
    bool isChoked() { return _choked; }
    void giveBandwidthQuota(qint64 bwq);

signals:
#if QT_VERSION < 0x050402
    void wasReset();
#endif

private:

    // The file data
    QByteArray _data;
    // Position in the data
    qint64 _read;

    // Bandwidth manager related
    QPointer<BandwidthManager> _bandwidthManager;
    qint64 _bandwidthQuota;
    qint64 _readWithProgress;
    bool _bandwidthLimited; // if _bandwidthQuota will be used
    bool _choked; // if upload is paused (readData() will return 0)
    friend class BandwidthManager;
protected slots:
    void slotJobUploadProgress(qint64 sent, qint64 t);
};

/**
 * @brief The PUTFileJob class
 * @ingroup libsync
 */
class PUTFileJob : public AbstractNetworkJob {
    Q_OBJECT

private:
    QScopedPointer<QIODevice> _device;
    QMap<QByteArray, QByteArray> _headers;
    QString _errorString;

public:
    // Takes ownership of the device
    explicit PUTFileJob(AccountPtr account, const QString& path, QIODevice *device,
                        const QMap<QByteArray, QByteArray> &headers, int chunk, QObject* parent = 0)
        : AbstractNetworkJob(account, path, parent), _device(device), _headers(headers), _chunk(chunk) {}
    ~PUTFileJob();

    int _chunk;

    virtual void start() Q_DECL_OVERRIDE;

    virtual bool finished() Q_DECL_OVERRIDE {
        emit finishedSignal();
        return true;
    }

    QString errorString() {
        return _errorString.isEmpty() ? reply()->errorString() : _errorString;
    }

    virtual void slotTimeout() Q_DECL_OVERRIDE;


signals:
    void finishedSignal();
    void uploadProgress(qint64,qint64);

private slots:
#if QT_VERSION < 0x050402
    void slotSoftAbort();
#endif
};

/**
 * @brief The MultipartJob class
 * @ingroup libsync
 */
class MultipartJob : public AbstractNetworkJob {
    Q_OBJECT

private:
    QVector<SyncFileItemPtr> _syncItems;
    QHttpMultiPart* _multipart;
    QString _errorString;
public:
    QElapsedTimer _duration;
    // Takes ownership of the device
    explicit MultipartJob(AccountPtr account, const QString& path, QHttpMultiPart* multiPart, const QVector<SyncFileItemPtr> &syncItems,QObject* parent = 0)
        : AbstractNetworkJob(account, path, parent), _syncItems(syncItems), _multipart(multiPart), _errorString(QString()) {}
    ~MultipartJob();

    virtual void start() Q_DECL_OVERRIDE;

    virtual bool finished() Q_DECL_OVERRIDE {
        emit finishedSignal();
        return true;
    }

    QString errorString() {
        return _errorString.isEmpty() ? reply()->errorString() : _errorString;
    }

    QVector<SyncFileItemPtr> syncItems() const {
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
 * @brief This job implements the asynchronous PUT
 *
 * If the server replies to a PUT with a OC-Finish-Poll url, we will query this url until the server
 * replies with an etag. https://github.com/owncloud/core/issues/12097
 * @ingroup libsync
 */
class PollJob : public AbstractNetworkJob {
    Q_OBJECT
    SyncJournalDb *_journal;
    QString _localPath;
public:
    SyncFileItemPtr _item;
    // Takes ownership of the device
    explicit PollJob(AccountPtr account, const QString &path, const SyncFileItemPtr &item,
                     SyncJournalDb *journal, const QString &localPath, QObject *parent)
        : AbstractNetworkJob(account, path, parent), _journal(journal), _localPath(localPath), _item(item) {}

    void start() Q_DECL_OVERRIDE;
    bool finished() Q_DECL_OVERRIDE;
    void slotTimeout() Q_DECL_OVERRIDE {
//      emit finishedSignal(false);
//      deleteLater();
        qDebug() << Q_FUNC_INFO;
        reply()->abort();
    }

signals:
    void finishedSignal();
};

/**
 * @brief The PropagateUploadFile class
 * @ingroup libsync
 */
class PropagateUploadFile : public PropagateItemJob {
    Q_OBJECT

private:
    /**
     * That's the start chunk that was stored in the database for resuming.
     * In the non-resuming case it is 0.
     * If we are resuming, this is the first chunk we need to send
     */
    int _startChunk;
    /**
     * This is the next chunk that we need to send. Starting from 0 even if _startChunk != 0
     * (In other words,  _startChunk + _currentChunk is really the number of the chunk we need to send next)
     * (In other words, _currentChunk is the number of the chunk that we already sent or started sending)
     */
    int _currentChunk;
    int _chunkCount; /// Total number of chunks for this file
    int _transferId; /// transfer id (part of the url)
    QElapsedTimer _duration;
    QVector<AbstractNetworkJob*> _jobs; /// network jobs that are currently in transit
    bool _finished; // Tells that all the jobs have been finished

    // measure the performance of checksum calc and upload
    Utility::StopWatch _stopWatch;

    QByteArray _transmissionChecksum;
    QByteArray _transmissionChecksumType;

    bool _deleteExisting;

    quint64 chunkSize() const { return _propagator->chunkSize(); }

public:
    PropagateUploadFile(OwncloudPropagator* propagator,const SyncFileItemPtr& item)
        : PropagateItemJob(propagator, item), _startChunk(0), _currentChunk(0), _chunkCount(0), _transferId(0), _finished(false), _deleteExisting(false) {}
    void start() Q_DECL_OVERRIDE;

    bool isLikelyFinishedQuickly() Q_DECL_OVERRIDE { return _item->_size < 100*1024; }

    /**
     * Whether an existing entity with the same name may be deleted before
     * the upload.
     *
     * Default: false.
     */
    void setDeleteExisting(bool enabled);

private slots:
    void slotPutFinished();
    void slotPollFinished();
    void slotUploadProgress(qint64,qint64);
    void abort() Q_DECL_OVERRIDE;
    void startNextChunk();
    void finalize(const SyncFileItem&);
    void slotJobDestroyed(QObject *job);
    void slotStartUpload(const QByteArray& transmissionChecksumType, const QByteArray& transmissionChecksum);
    void slotComputeTransmissionChecksum(const QByteArray& contentChecksumType, const QByteArray& contentChecksum);
    void slotComputeContentChecksum();

private:
    void startPollJob(const QString& path);
    void abortWithError(SyncFileItem::Status status, const QString &error);
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
    bool _running; // Tells that all the jobs have been finished
    bool _preparingBundle; // Tells that all the jobs have been finished
    quint64 _size; // Tells what is the total size of _itemsToSync inside the bundle
    quint64 _currentBundleSize;
    quint64 _currentRequestsNumber;
    quint64 _totalFiles;
    // measure the performance of checksum calc and upload
    Utility::StopWatch _stopWatch;

public:
    PropagateBundle(OwncloudPropagator* propagator)
        : PropagateItemJob(propagator, SyncFileItemPtr(new SyncFileItem)), _running(false), _preparingBundle(true), _size(0), _currentBundleSize(0), _currentRequestsNumber(0) {}
    void start() Q_DECL_OVERRIDE;
    void startBundle();
    void append(const SyncFileItemPtr &bundledFile);
    QByteArray getRemotePath(QString filePath);
    bool empty();
    bool scheduleNextJob() Q_DECL_OVERRIDE {
        if (_running != true && _propagator->runningNowAtRootJob() == 1){
            _running = true;
            QMetaObject::invokeMethod(this, "start"); // We could be in a different thread (neon jobs)
        }
        if (_state == NotYetStarted){
            _state = Running;
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
    void itemDone(SyncFileItemPtr item, SyncFileItem::Status status, const QString &errorString = QString());
};

}

