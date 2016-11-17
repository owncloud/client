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

#include "propagateupload.h"
#include "propagatebundle.h"
#include "owncloudpropagator_p.h"
#include "networkjobs.h"
#include "account.h"
#include "syncjournaldb.h"
#include "syncjournalfilerecord.h"
#include "utility.h"
#include "filesystem.h"
#include "propagatorjobs.h"
#include "checksums.h"
#include "syncengine.h"

#include <QNetworkAccessManager>
#include <QFileInfo>
#include <QXmlStreamReader>

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
namespace {
const char owncloudShouldSoftCancelPropertyName[] = "owncloud-should-soft-cancel";
}
#endif

namespace OCC {

/**
 * We do not want to upload files that are currently being modified.
 * To avoid that, we don't upload files that have a modification time
 * that is too close to the current time.
 *
 * This interacts with the msBetweenRequestAndSync delay in the folder
 * manager. If that delay between file-change notification and sync
 * has passed, we should accept the file for upload here.
 */
static bool fileIsStillChanging(const SyncFileItem & item)
{
    const QDateTime modtime = Utility::qDateTimeFromTime_t(item._modtime);
    const qint64 msSinceMod = modtime.msecsTo(QDateTime::currentDateTime());

    return msSinceMod < SyncEngine::minimumFileAgeForUpload
            // if the mtime is too much in the future we *do* upload the file
            && msSinceMod > -10000;
}

void PropagateBundle::start()
{
    if (_propagator->_abortRequested.fetchAndAddRelaxed(0)) {
        return;
    }

    if (!_itemsToChecksum.isEmpty()) {
        //this will block other threads from processing files and bundling while this will do that
        //after sufficient number of files being checksumed, flag will be unblocked
        _preparingBundle = true;

        //this will add checksums and remove itself from activeJobList after is completed
        _propagator->_activeJobList.append(this);
        return slotComputeTransmissionChecksum();
    }

    //this can generate itemDone(), dont add to activeJobList
    startBundle();
}

void PropagateBundle::slotComputeTransmissionChecksum()
{
    if (_propagator->_abortRequested.fetchAndAddRelaxed(0)) {
        return;
    }

    const SyncFileItemPtr &item = _itemsToChecksum.first();
    const QString filePath = _propagator->getFilePath(item->_file);

    // remember the modtime before checksumming to be able to detect a file
    // change during the checksum calculation
    item->_modtime = FileSystem::getModTime(filePath);

    QByteArray checksumType = contentChecksumType();

    // Maybe the discovery already computed the checksum?
    if (item->_contentChecksumType == checksumType
            && !item->_contentChecksum.isEmpty()) {
        // Reuse the content checksum as the transmission checksum if possible
        const auto supportedTransmissionChecksums =
                _propagator->account()->capabilities().supportedChecksumTypes();
        if (supportedTransmissionChecksums.contains(checksumType)) {
            slotStartUpload(checksumType, item->_contentChecksum);
            return;
        }
    }

    // Compute the transmission checksum.
    auto computeChecksum = new ComputeChecksum(this);
    if (uploadChecksumEnabled()) {
        computeChecksum->setChecksumType(_propagator->account()->capabilities().uploadChecksumType());
    } else {
        computeChecksum->setChecksumType(QByteArray());
    }

    connect(computeChecksum, SIGNAL(done(QByteArray,QByteArray)),
            SLOT(slotStartUpload(QByteArray,QByteArray)));
    computeChecksum->start(filePath);
}

void PropagateBundle::slotStartUpload(const QByteArray& transmissionChecksumType, const QByteArray& transmissionChecksum)
{
    const SyncFileItemPtr &item = _itemsToChecksum.takeFirst();

    item->_contentChecksum = transmissionChecksum;
    item->_contentChecksumType = transmissionChecksumType;

    // add this item to sync list, as it now has checksum computed
    _itemsToSync.append(item);

    _currentBundleSize += item->_size;
    _currentRequestsNumber++;

    // Remove ourselfs from the list of active job, before any posible call to itemDone()
    _propagator->_activeJobList.removeOne(this);

    // check if we have anything to add to our bundle
    if (!_itemsToChecksum.empty()){
        SyncFileItemPtr nextItem = _itemsToChecksum.first();

        // if next item will exceed bundle size, send the bundle now
        // otherwise, compute next checksum
        if (((_currentBundleSize + nextItem->_size) >= chunkSize())
                || (_currentRequestsNumber >= checkBundledRequestsLimits())){
            _currentBundleSize = 0;
            _currentRequestsNumber = 0;
            startBundle();
        } else {
            start();
        }
    } else {
        //_itemsToChecksum is already empty, send what is left in _itemsToSync.
        startBundle();
    }
}

void PropagateBundle::startBundle()
{
    if (_propagator->_abortRequested.fetchAndAddRelaxed(0)) {
        return;
    }

    // job takes ownership of device via a QScopedPointer. Job deletes itself when finishing
    MultipartJob* job = new MultipartJob(_propagator->account(), _propagator->account()->davFilesPath(), new QHttpMultiPart(QHttpMultiPart::RelatedType));

    // start constructing the bundle metadata xml file
    QByteArray xml = "<?xml version='1.0' encoding='UTF-8'?>\n"
            "<d:multipart xmlns:d=\"DAV:\">\n";
    int contentID = 0;

    while (!_itemsToSync.isEmpty()){
        const SyncFileItemPtr &item = _itemsToSync.takeFirst();
        const QString fullFilePath = _propagator->getFilePath(item->_file);

        if (!FileSystem::fileExists(fullFilePath)) {
           itemDone(item, SyncFileItem::SoftError, tr("File Removed"));
           continue;
        }

        time_t prevModtime = item->_modtime; // the _item value was set in PropagateUploadFileQNAM::start()
        // but a potential checksum calculation could have taken some time during which the file could
        // have been changed again, so better check again here.

        item->_modtime = FileSystem::getModTime(fullFilePath);
        if( prevModtime != item->_modtime ) {
            _propagator->_anotherSyncNeeded = true;
            itemDone(item, SyncFileItem::SoftError, tr("Local file changed during syncing. It will be resumed."));
            continue;
        }

        item->_size = FileSystem::getSize(fullFilePath);

        // But skip the file if the mtime is too close to 'now'!
        // That usually indicates a file that is still being changed
        // or not yet fully copied to the destination.
        if (fileIsStillChanging(*item)) {
            _propagator->_anotherSyncNeeded = true;
            itemDone(item, SyncFileItem::SoftError, tr("Local file changed during sync."));
            continue;
        }

        //TODO use Upload Device to support bandwith limitation on the client
        QFile *file = new QFile(fullFilePath);
        if (! file->open(QIODevice::ReadOnly)) {
            qDebug() << "ERR: Could not prepare upload device: " << file->errorString();

            // If the file is currently locked, we want to retry the sync
            // when it becomes available again.
            if (FileSystem::isFileLocked(fullFilePath)) {
                emit _propagator->seenLockedFile(fullFilePath);
            }

            // Soft error because this is likely caused by the user modifying his files while syncing
            itemDone(item, SyncFileItem::SoftError, tr("ERR: Could not prepare upload device"));
        } else {
            // add xml part for this specific file
            xml += "  <d:part>\n"
                    "    <d:prop>\n";
            if (!item->_contentChecksumType.isEmpty()) {
                xml += "      <d:oc-checksum>" + makeChecksumHeader(item->_contentChecksumType, item->_contentChecksum) + "</d:oc-checksum>\n";
            }

            if(item->_file.contains(".sys.admin#recall#")) {
                xml += "      <d:oc-tag>" + QByteArray(".sys.admin#recall#") + "</d:oc-tag>\n";
            }

            xml += "      <d:oc-path>" + getRemotePath(item->_file) + "</d:oc-path>\n"
                    "      <d:oc-mtime>" + QByteArray::number(qint64(item->_modtime)) + "</d:oc-mtime>\n"
                    "      <d:oc-id>" + QByteArray::number(contentID) + "</d:oc-id>\n"
                    "      <d:oc-total-length>" + QByteArray::number(qint64(item->_size)) + "</d:oc-total-length>\n"
                    "    </d:prop>\n"
                    "  </d:part>\n";

            QHttpPart bundleContent;
            bundleContent.setRawHeader("Content-ID", QByteArray::number(qint64(contentID)));
            bundleContent.setBody(file->readAll());
            file->close();
            job->addItemPart(bundleContent, item);
            contentID++;
        }
        delete file;
    }

    // finish xml building
    xml += "</d:multipart>";

    if (!job->isEmpty())
    {
        QHttpPart bundleMetadata;
        bundleMetadata.setBody(xml);
        bundleMetadata.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("text/xml; charset=utf-8"));
        bundleMetadata.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(xml.size()));
        job->addRootPart(bundleMetadata);

        _propagator->_activeJobList.append(this);
        job->_duration.start();
        _jobs.append(job);
        connect(job, SIGNAL(finishedSignal()), this, SLOT(slotMultipartFinished()));
        connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(slotJobDestroyed(QObject*)));
        job->start();

        // we have finished preparing bundles to send, allow other bundles to be send
        _preparingBundle = false;

        // check if there are any items to be bundled and if you can run another parallel job
        if (!_itemsToChecksum.empty() && (_propagator->_activeJobList.count() < _propagator->maximumActiveJob())) {
            start();
        }
    } else {
        delete job;
    }
}

QByteArray PropagateBundle::getRemotePath(QString filePath){
    QString remotePath(_propagator->_remoteFolder+filePath);
    return remotePath.toStdString().c_str();
}

void PropagateBundle::append(const SyncFileItemPtr &bundledFile){
    _size += bundledFile->_size;
    _itemsToChecksum.append(bundledFile);
}

bool PropagateBundle::empty(){
    return _itemsToChecksum.empty();
}

void PropagateBundle::slotMultipartFinished()
{
    MultipartJob *job = qobject_cast<MultipartJob *>(sender());
    Q_ASSERT(job);
    slotJobDestroyed(job); // remove it from the _jobs list

    qDebug() << Q_FUNC_INFO << job->reply()->request().url() << "FINISHED WITH STATUS"
             << job->reply()->error()
             << (job->reply()->error() == QNetworkReply::NoError ? QLatin1String("") : job->reply()->errorString())
             << job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute)
             << job->reply()->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

    _propagator->_activeJobList.removeOne(this);

    QNetworkReply::NetworkError err = job->reply()->error();

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
    if (job->reply()->error() == QNetworkReply::OperationCanceledError && job->reply()->property(owncloudShouldSoftCancelPropertyName).isValid()) {
        // Abort the job and try again later.
        // This works around a bug in QNAM wich might reuse a non-empty buffer for the next request.
        qDebug() << "Forcing job abort on HTTP connection reset with Qt < 5.4.2.";
        _propagator->_anotherSyncNeeded = true;
        abortWithError(SyncFileItem::SoftError, tr("Forcing job abort on HTTP connection reset with Qt < 5.4.2."));
        return;
    }
#endif

    if (err != QNetworkReply::NoError) {
        _item->_httpErrorCode = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        QByteArray replyContent = job->reply()->readAll();
        qDebug() << replyContent; // display the XML error in the debug
        QString errorString = errorMessage(job->errorString(), replyContent);

        if (job->reply()->hasRawHeader("OC-ErrorString")) {
            errorString = job->reply()->rawHeader("OC-ErrorString");
        }

        //TODO: soft error till other mechanism will be added - this will retry bundle later.
        abortWithError(SyncFileItem::SoftError, errorString);
        return;
    }

    // Parse DAV response
    QXmlStreamReader reader(job->reply());
    reader.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("d", "DAV:"));

    QString currentHref;
    QString currentOcPath;
    QString expectedPath(_propagator->account()->davFilesPath());
    QMap<QString, QString> itemProperties;
    QMap<QString, QMap<QString, QString> > responseObjectsProperties;
    bool insidePropstat = false;
    bool insideProp = false;
    bool insideError = false;

    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType type = reader.readNext();
        QString name = reader.name().toString();
        // Start elements with DAV:
        if (type == QXmlStreamReader::StartElement && reader.namespaceUri() == QLatin1String("DAV:")) {
            if (name == QLatin1String("href")) {
                // We don't use URL encoding in our request URL (which is the expected path) (QNAM will do it for us)
                // but the result will have URL encoding..
                QString hrefString = QString::fromUtf8(QByteArray::fromPercentEncoding(reader.readElementText().toUtf8()));
                if (!hrefString.startsWith(expectedPath)) {
                    qDebug() << "Invalid href" << hrefString << "expected ending with" << expectedPath;
                }
                currentHref = hrefString;
            } else if (name == QLatin1String("response")) {
                continue;
            } else if (name == QLatin1String("propstat")) {
                insidePropstat = true;
            } else if (name == QLatin1String("status") && insidePropstat) {
                QString httpStatus = reader.readElementText();
                itemProperties.insert(name, httpStatus);
            } else if (name == QLatin1String("prop")) {
                insideProp = true;
                continue;
            } else if (name == QLatin1String("multistatus")) {
                continue;
            }
        }

        if (type == QXmlStreamReader::StartElement && insidePropstat && insideProp) {
            if (name == QLatin1String("oc-path")){
                currentOcPath = reader.readElementText(QXmlStreamReader::SkipChildElements);
            } else if (name == QLatin1String("error")){
                insideError = true;
            } else if (name == QLatin1String("exception") && insideError){
                itemProperties.insert(name, reader.readElementText(QXmlStreamReader::SkipChildElements));
            } else if (name == QLatin1String("message") && insideError){
                itemProperties.insert(name, reader.readElementText(QXmlStreamReader::SkipChildElements));
            } else{
                itemProperties.insert(name, reader.readElementText(QXmlStreamReader::SkipChildElements));
            }
        }

        // End elements with DAV:
        if (type == QXmlStreamReader::EndElement) {
            if (reader.namespaceUri() == QLatin1String("DAV:")) {
                if (reader.name() == "response") {
                    currentHref.clear();
                } else if (reader.name() == "propstat") {
                    insidePropstat = false;
                    if (!currentOcPath.isEmpty()){
                        responseObjectsProperties.insert(currentOcPath, QMap<QString,QString>(itemProperties));
                    }
                    currentOcPath.clear();
                    itemProperties.clear();
                } else if (reader.name() == "prop") {
                    insideProp = false;
                } else if (reader.name() == "error") {
                    insideError = false;
                }
            }
        }
    }

    if (reader.hasError()) {
        // XML Parser error? Whatever had been emitted before will come as directoryListingIterated
        qDebug() << "ERROR" << reader.errorString();
        abortWithError(SyncFileItem::SoftError, tr("Cannot parse multistatus response!"));
        return;
    }

    QList<SyncFileItemPtr> syncItems = job->syncItems();

    //get the total duration, and try to estimate sync time for single item in the bundle
    quint64 avgItemDuration = job->_duration.elapsed()/syncItems.count();

    foreach(const SyncFileItemPtr &item, syncItems) {
        item->_requestDuration = avgItemDuration;
        item->_responseTimeStamp = job->responseTimestamp();
        slotItemFinished(item, responseObjectsProperties);
    }

    // performance logging
    qDebug() << "*==* duration BUNDLE UPLOAD" << syncItems.count() << "items synced"
             << "average duration" << avgItemDuration;
    // The job might stay alive for the whole sync, release this tiny bit of memory.

    if (_jobs.empty() && _itemsToChecksum.isEmpty()){
        done(SyncFileItem::Success);
    } else if (!_preparingBundle){
        // This will be called only if some other thread is not currently preparing bundles (checksums)
        start();
    }
}

void PropagateBundle::slotItemFinished(const SyncFileItemPtr &item, QMap<QString, QMap<QString, QString> > &responseObjectsProperties)
{
    QString itemFilePath(getRemotePath(item->_file));
    QMap<QString, QString> fileProperties = responseObjectsProperties.value(itemFilePath);

    item->_httpErrorCode = getHttpStatusCode(fileProperties.value("status"));

    if (200 == item->_httpErrorCode){
        // Check if the file still exists
        const QString fullFilePath(_propagator->getFilePath(item->_file));
        if( !FileSystem::fileExists(fullFilePath) ) {
            _propagator->_anotherSyncNeeded = true;
        }

        if (! FileSystem::verifyFileUnchanged(fullFilePath, item->_size, item->_modtime)) {
            _propagator->_anotherSyncNeeded = true;
        }

        //OC-FileID section
        if (fileProperties.contains("oc-fileid")){
            QString fid = fileProperties.value("oc-fileid");
            if( !item->_fileId.isEmpty() && item->_fileId != fid ) {
                qDebug() << "WARN: File ID changed!" << item->_fileId << fid;
            }
            item->_fileId =fid.toStdString().c_str();
        }

        //OC-ETag section
        QByteArray ocEtag = parseEtag(fileProperties.value("oc-etag").toStdString().c_str());
        QByteArray etag = parseEtag(fileProperties.value("etag").toStdString().c_str());
        item->_etag = ocEtag.isEmpty() ? etag : ocEtag;
        if (ocEtag.length() > 0 && ocEtag != etag) {
            qDebug() << "Quite peculiar, we have an etag != OC-Etag [no problem!]" << etag << ocEtag;
        }

        if (fileProperties.value("oc-mtime") != "accepted"){
            // OC-MTime is supported since owncloud 5.0.   But not when chunking.
            // Normally Owncloud 6 always puts OC-MTime
            qWarning() << "Server does not support OC-MTime" << fileProperties.value("oc-mtime");
            // Well, the mtime was not set
            itemDone(item, SyncFileItem::SoftError, "Server does not support OC-MTime");
            return;
        }
    } else{
        //We do not check for problems with shared folder since it is only CREATE
        //REMARK: if the file will support update, ensure to override checkForProblemsWithShared()

        qDebug() << Q_FUNC_INFO << item->_file << "ERROR WITH STATUS"
                 << fileProperties.value("status")
                 << fileProperties.value("exception")
                 << fileProperties.value("message");

        if (412 == _item->_httpErrorCode) {
            // Precondition Failed: Maybe the bad etag is in the database, we need to clear the
           // parent folder etag so we won't read from DB next sync.
            _propagator->_journal->avoidReadFromDbOnNextSync(item->_file);
            _propagator->_anotherSyncNeeded = true;
        }

        item->_errorString = fileProperties.value("message");
        item->_status = classifyError(QNetworkReply::ContentOperationNotPermittedError, item->_httpErrorCode,
                                      &_propagator->_anotherSyncNeeded);
        itemDone(item, item->_status, item->_errorString);
        return;
    }

    if (!_propagator->_journal->setFileRecord(SyncJournalFileRecord(*item, _propagator->getFilePath(item->_file)))) {
        itemDone(item, SyncFileItem::FatalError, tr("Error writing metadata to the database"));
        return;
    }

    // Remove from the progress database:
    _propagator->_journal->setUploadInfo(item->_file, SyncJournalDb::UploadInfo());
    _propagator->_journal->commit("upload file start");
    itemDone(item, SyncFileItem::Success);
}

int PropagateBundle::getHttpStatusCode(const QString &status){
    //if cannot read code, it means that server is not supported and raise 500 Internal Server Error
    int code = 500;
    QStringList statusList = status.split(QRegExp("\\s+"));
    if (2 < statusList.size()){
        //we expect status list to be at least 3 elements and second element to be the http error code
        code = statusList.at(1).toInt();
    }
    return code;
}

void PropagateBundle::slotJobDestroyed(QObject* job)
{
    _jobs.erase(std::remove(_jobs.begin(), _jobs.end(), job) , _jobs.end());
}

// This function is used whenever there is an error occuring and we need to raise status for whole the bundle
void PropagateBundle::abortWithError(SyncFileItem::Status status, const QString &error)
{
    foreach(auto *job, _jobs) {
        if (job->reply()) {
            qDebug() << Q_FUNC_INFO << job;
            job->reply()->abort();
        }
    }

    done(status, error);
}

MultipartJob::~MultipartJob()
{
    // Make sure that we destroy the QNetworkReply before our _device of which it keeps an internal pointer.
    setReply(0);
}

void MultipartJob::start() {
    while(!_syncParts.empty()) {
        const QHttpPart &itemPart = _syncParts.takeFirst();
        _multipart->append(itemPart);
    }

    QNetworkRequest req;
    setReply(multipartRequest(path(), req, _multipart));
    _multipart->setParent(reply()); // delete the multiPart with the job
    setupConnections(reply());

    if( reply()->error() != QNetworkReply::NoError ) {
        qWarning() << Q_FUNC_INFO << " Network error: " << reply()->errorString();
    }

    connect(this, SIGNAL(networkActivity()), account().data(), SIGNAL(propagatorNetworkActivity()));

    //TODO this uses _device, and we have no access here to that
//    // For Qt versions not including https://codereview.qt-project.org/110150
//    // Also do the runtime check if compiled with an old Qt but running with fixed one.
//    // (workaround disabled on windows and mac because the binaries we ship have patched qt)
//#if QT_VERSION < QT_VERSION_CHECK(4, 8, 7)
//    if (QLatin1String(qVersion()) < QLatin1String("4.8.7"))
//        connect(_device.data(), SIGNAL(wasReset()), this, SLOT(slotSoftAbort()));
//#elif QT_VERSION > QT_VERSION_CHECK(5, 0, 0) && QT_VERSION < QT_VERSION_CHECK(5, 4, 2) && !defined Q_OS_WIN && !defined Q_OS_MAC
//    if (QLatin1String(qVersion()) < QLatin1String("5.4.2"))
//        connect(_device.data(), SIGNAL(wasReset()), this, SLOT(slotSoftAbort()));
//#endif

    AbstractNetworkJob::start();
}

void MultipartJob::addItemPart(const QHttpPart &itemPart, const SyncFileItemPtr &item){
    // add itemPart to queue, this queue will be used to construct the multipart message
    _syncParts.append(itemPart);

    // add item pointer to queue, it will be used to construct the response at the end.
    _syncItems.append(item);
}

void MultipartJob::addRootPart(const QHttpPart &itemPart){
    // if used, this part will be added to the begining of multipart request
    _syncParts.prepend(itemPart);
}

void MultipartJob::slotTimeout() {
    qDebug() << "Timeout" << (reply() ? reply()->request().url() : path());
    if (!reply())
        return;
    _errorString =  tr("Connection Timeout");
    reply()->abort();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
void MultipartJob::slotSoftAbort() {
    reply()->setProperty(owncloudShouldSoftCancelPropertyName, true);
    reply()->abort();
}
#endif

}
