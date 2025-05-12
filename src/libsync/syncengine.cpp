/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "syncengine.h"
#include "account.h"
#include "common/asserts.h"
#include "common/syncfilestatus.h"
#include "common/syncjournaldb.h"
#include "common/syncjournalfilerecord.h"
#include "common/vfs.h"
#include "configfile.h"
#include "csync_exclude.h"
#include "discovery.h"
#include "discoveryphase.h"
#include "owncloudpropagator.h"
#include "propagateremotedelete.h"

#include <chrono>

#include <QDir>
#include <QLoggingCategory>
#include <QSslSocket>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QUrl>

using namespace std::chrono_literals;

namespace OCC {

Q_LOGGING_CATEGORY(lcEngine, "sync.engine", QtInfoMsg)

// doc in header
std::chrono::seconds SyncEngine::minimumFileAgeForUpload(2s);

SyncEngine::SyncEngine(AccountPtr account, const QUrl &baseUrl, const QString &localPath, const QString &remotePath, OCC::SyncJournalDb *journal)
    : _account(account)
    , _baseUrl(baseUrl)
    , _needsUpdate(false)
    , _syncRunning(false)
    , _localPath(localPath)
    , _remotePath(remotePath)
    , _journal(journal)
    , _progressInfo(new ProgressInfo)
    , _uploadLimit(0)
    , _downloadLimit(0)
{
    // Refactoring todo: reality check that we actually need to use these types in queued connections. if so,
    // we should move to a one shot registration method a) to make it really easy to see which types may
    // be passed between threads and b) to just call it once.
    // suggest calling registration method in FolderMan or one of the other single instance managers on startup
    // one day it might belong in an app builder routine.
    qRegisterMetaType<SyncFileItem>("SyncFileItem");
    qRegisterMetaType<SyncFileItemPtr>("SyncFileItemPtr");
    qRegisterMetaType<SyncFileItem::Status>("SyncFileItem::Status");
    qRegisterMetaType<SyncFileStatus>("SyncFileStatus");
    qRegisterMetaType<SyncFileItemSet>("SyncFileItemSet");
    qRegisterMetaType<SyncFileItem::Direction>("SyncFileItem::Direction");

    // Everything in the SyncEngine expects a trailing slash for the localPath.
    OC_ASSERT(localPath.endsWith(QLatin1Char('/')));

    _excludedFiles.reset(new ExcludedFiles);

    _syncFileStatusTracker.reset(new SyncFileStatusTracker(this));
}

SyncEngine::~SyncEngine()
{
    _goingDown = true;
    abort(tr("application exit", "abort reason"));
    _excludedFiles.reset();
}

/**
 * Check if the item is in the blacklist.
 * If it should not be sync'ed because of the blacklist, update the item with the error instruction
 * and proper error message, and return true.
 * If the item is not in the blacklist, or the blacklist is stale, return false.
 */
bool SyncEngine::checkErrorBlacklisting(SyncFileItem &item)
{
    if (!_journal) {
        qCCritical(lcEngine) << "Journal is undefined!";
        return false;
    }

    SyncJournalErrorBlacklistRecord entry = _journal->errorBlacklistEntry(item._file);
    item._hasBlacklistEntry = false;

    if (!entry.isValid()) {
        return false;
    }

    item._hasBlacklistEntry = true;

    // If duration has expired, it's not blacklisted anymore
    time_t now = Utility::qDateTimeToTime_t(QDateTime::currentDateTimeUtc());
    if (now >= entry._lastTryTime + entry._ignoreDuration) {
        qCInfo(lcEngine) << "blacklist entry for " << item._file << " has expired!";
        return false;
    }

    // If the file has changed locally or on the server, the blacklist
    // entry no longer applies
    if (item._direction == SyncFileItem::Up) { // check the modtime
        if (item._modtime == 0 || entry._lastTryModtime == 0) {
            return false;
        } else if (item._modtime != entry._lastTryModtime) {
            qCInfo(lcEngine) << item._file << " is blacklisted, but has changed mtime!";
            return false;
        } else if (item._renameTarget != entry._renameTarget) {
            qCInfo(lcEngine) << item._file << " is blacklisted, but rename target changed from" << entry._renameTarget;
            return false;
        }
    } else if (item._direction == SyncFileItem::Down) {
        // download, check the etag.
        if (item._etag.isEmpty() || entry._lastTryEtag.isEmpty()) {
            qCInfo(lcEngine) << item._file << "one ETag is empty, no blacklisting";
            return false;
        } else if (item._etag.toUtf8() != entry._lastTryEtag) {
            qCInfo(lcEngine) << item._file << " is blacklisted, but has changed etag!";
            return false;
        }
    }

    int waitSeconds = entry._lastTryTime + entry._ignoreDuration - now;
    qCInfo(lcEngine) << "Item is on blacklist: " << entry._file
                     << "retries:" << entry._retryCount
                     << "for another" << waitSeconds << "s";
    // We need to indicate that we skip this file due to blacklisting
    // for reporting and for making sure we don't update the blacklist
    // entry yet.
    // Classification is this _instruction and _status
    item.setInstruction(CSYNC_INSTRUCTION_IGNORE);
    if (entry._errorCategory == SyncJournalErrorBlacklistRecord::Category::LocalSoftError) {
        item._status = SyncFileItem::SoftError;
        item._errorString = entry._errorString;
    } else {
        item._status = SyncFileItem::BlacklistedError;

        auto waitSecondsStr = Utility::durationToDescriptiveString1(std::chrono::seconds(waitSeconds));
        item._errorString = tr("%1 (skipped due to earlier error, trying again in %2)").arg(entry._errorString, waitSecondsStr);

        if (entry._errorCategory == SyncJournalErrorBlacklistRecord::Category::InsufficientRemoteStorage) {
            slotInsufficientRemoteStorage();
        }
    }
    return true;
}

static bool isFileTransferInstruction(SyncInstructions instruction)
{
    return instruction == CSYNC_INSTRUCTION_CONFLICT
        || instruction == CSYNC_INSTRUCTION_NEW
        || instruction == CSYNC_INSTRUCTION_SYNC
        || instruction == CSYNC_INSTRUCTION_TYPE_CHANGE;
}

void SyncEngine::deleteStaleDownloadInfos(const SyncFileItemSet &syncItems)
{
    // Find all downloadinfo paths that we want to preserve.
    QSet<QString> download_file_paths;
    for (const auto &it : syncItems) {
        if (it->_direction == SyncFileItem::Down && it->_type == ItemTypeFile && isFileTransferInstruction(it->instruction())) {
            download_file_paths.insert(it->_file);
        }
    }

    // Delete from journal and from filesystem.
    const QVector<SyncJournalDb::DownloadInfo> deleted_infos =
        _journal->getAndDeleteStaleDownloadInfos(download_file_paths);
    for (const auto &deleted_info : deleted_infos) {
        const QString tmppath = _propagator->fullLocalPath(deleted_info._tmpfile);
        qCInfo(lcEngine) << "Deleting stale temporary file: " << tmppath;
        FileSystem::remove(tmppath);
    }
}

void SyncEngine::deleteStaleUploadInfos(const SyncFileItemSet &syncItems)
{
    // Find all blacklisted paths that we want to preserve.
    QSet<QString> upload_file_paths;
    for (const auto &it : syncItems) {
        if (it->_direction == SyncFileItem::Up && it->_type == ItemTypeFile && isFileTransferInstruction(it->instruction())) {
            upload_file_paths.insert(it->_file);
        }
    }

    // Delete from journal.
    const auto &ids = _journal->deleteStaleUploadInfos(upload_file_paths);

    // Delete the stales chunk on the server.
    if (account()->capabilities().chunkingNg()) {
        for (auto transferId : ids) {
            if (!transferId)
                continue; // Was not a chunked upload
            (new DeleteJob(account(), account()->url(), QLatin1String("remote.php/dav/uploads/") + account()->davUser() + QLatin1Char('/') + QString::number(transferId), this))->start();
        }
    }
}

void SyncEngine::deleteStaleErrorBlacklistEntries(const SyncFileItemSet &syncItems)
{
    // Find all blacklisted paths that we want to preserve.
    QSet<QString> blacklist_file_paths;
    for (const auto &it : syncItems) {
        if (it->_hasBlacklistEntry)
            blacklist_file_paths.insert(it->_file);
    }

    // Delete from journal.
    _journal->deleteStaleErrorBlacklistEntries(blacklist_file_paths);
}

void SyncEngine::conflictRecordMaintenance()
{
    // Remove stale conflict entries from the database
    // by checking which files still exist and removing the
    // missing ones.
    const auto &conflictRecordPaths = _journal->conflictRecordPaths();
    for (const auto &path : conflictRecordPaths) {
        auto fsPath = _propagator->fullLocalPath(QString::fromUtf8(path));
        if (!QFileInfo::exists(fsPath)) {
            _journal->deleteConflictRecord(path);
        }
    }

    // Did the sync see any conflict files that don't yet have records?
    // If so, add them now.
    //
    // This happens when the conflicts table is new or when conflict files
    // are downlaoded but the server doesn't send conflict headers.
    for (const auto &path : std::as_const(_seenConflictFiles)) {
        OC_ASSERT(Utility::isConflictFile(path));

        auto bapath = path.toUtf8();
        if (!conflictRecordPaths.contains(bapath)) {
            ConflictRecord record;
            record.path = bapath;
            auto basePath = Utility::conflictFileBaseNameFromPattern(bapath);
            record.initialBasePath = basePath;

            // Determine fileid of target file
            SyncJournalFileRecord baseRecord;
            if (_journal->getFileRecord(basePath, &baseRecord) && baseRecord.isValid()) {
                record.baseFileId = baseRecord._fileId;
            }

            _journal->setConflictRecord(record);
        }
    }
}


void OCC::SyncEngine::slotItemDiscovered(const OCC::SyncFileItemPtr &item)
{
    if (Utility::isConflictFile(item->_file))
        _seenConflictFiles.insert(item->_file);
    if (item->instruction() == CSYNC_INSTRUCTION_NONE) {
        if (_account->capabilities().uploadConflictFiles() && Utility::isConflictFile(item->_file)) {
            // For uploaded conflict files, files with no action performed on them should
            // be displayed: but we mustn't overwrite the instruction if something happens
            // to the file!
            item->_errorString = tr("Unresolved conflict.");
            item->setInstruction(CSYNC_INSTRUCTION_IGNORE);
            item->_status = SyncFileItem::Conflict;
        }
        return;
    }

    // check for blacklisting of this item.
    // if the item is on blacklist, the instruction was set to ERROR
    checkErrorBlacklisting(*item);
    _needsUpdate = true;

    Q_ASSERT([&] {
        const auto it = _syncItems.find(item);
        if (it != _syncItems.cend()) {
            const auto &item2 = it->get();
            qCWarning(lcEngine) << "We already have an item for " << item2->_file << ":" << item2->instruction() << item2->_direction << "|"
                                << item->instruction() << item->_direction;
            return false;
        }
        return true;
    }());
    _syncItems.insert(item);

    slotNewItem(item);

    if (item->isDirectory()) {
        slotFolderDiscovered(item->_etag.isEmpty(), item->_file);
    }
}

void SyncEngine::startSync()
{
    if (_syncRunning) {
        OC_ASSERT(false);
        return;
    }
    _duration.reset();

    _syncRunning = true;
    _anotherSyncNeeded = false;

    _seenConflictFiles.clear();

    _progressInfo->reset();

    if (!QFileInfo::exists(_localPath)) {
        // No _tr, it should only occur in non-mirall
        Q_EMIT syncError(QStringLiteral("Unable to find local sync folder."));
        finalize(false);
        return;
    }

    // Check free size on disk first.
    const qint64 minFree = criticalFreeSpaceLimit();
    const qint64 freeBytes = Utility::freeDiskSpace(_localPath);
    if (freeBytes >= 0) {
        if (freeBytes < minFree) {
            qCWarning(lcEngine()) << "Too little space available at" << _localPath << ". Have" << freeBytes << "bytes and require at least" << minFree
                                  << "bytes";
            Q_EMIT syncError(tr("Only %1 are available, need at least %2 to start",
                "Placeholders are postfixed with file sizes using Utility::octetsToString()")
                                 .arg(
                                     Utility::octetsToString(freeBytes),
                                     Utility::octetsToString(minFree)));
            finalize(false);
            return;
        } else {
            qCInfo(lcEngine) << "There are" << Utility::octetsToString(freeBytes) << "available at" << _localPath;
        }
    } else {
        qCWarning(lcEngine) << "Could not determine free space available at" << _localPath;
    }

    _syncItems.clear();
    _needsUpdate = false;

    if (!_journal->exists()) {
        qCInfo(lcEngine) << "New sync (no sync journal exists)";
    } else {
        qCInfo(lcEngine) << "Sync with existing sync journal";
    }

    qCInfo(lcEngine) << "Using Qt " << qVersion() << " SSL library " << QSslSocket::sslLibraryVersionString() << " on " << Utility::platformName();

    // This creates the DB if it does not exist yet.
    if (!_journal->open()) {
        qCWarning(lcEngine) << "No way to create a sync journal!";
        Q_EMIT syncError(tr("Unable to open or create the local sync database. Make sure you have write access in the sync folder."));
        finalize(false);
        return;
        // database creation error!
    }

    // Functionality like selective sync might have set up etag storage
    // filtering via schedulePathForRemoteDiscovery(). This *is* the next sync, so
    // undo the filter to allow this sync to retrieve and store the correct etags.
    _journal->clearEtagStorageFilter();

    _excludedFiles->setExcludeConflictFiles(!_account->capabilities().uploadConflictFiles());

    _lastLocalDiscoveryStyle = _localDiscoveryStyle;

    if (syncOptions()._vfs->mode() == Vfs::WithSuffix && syncOptions()._vfs->fileSuffix().isEmpty()) {
        Q_EMIT syncError(tr("Using virtual files with suffix, but suffix is not set"));
        finalize(false);
        return;
    }

    bool ok;
    auto selectiveSyncBlackList = _journal->getSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, ok);
    if (ok) {
        bool usingSelectiveSync = (!selectiveSyncBlackList.isEmpty());
        qCInfo(lcEngine) << (usingSelectiveSync ? "Using Selective Sync" : "NOT Using Selective Sync");
    } else {
        qCWarning(lcEngine) << "Could not retrieve selective sync list from DB";
        Q_EMIT syncError(tr("Unable to read the blacklist from the local database"));
        finalize(false);
        return;
    }

    qCInfo(lcEngine) << "#### Discovery start ####################################################" << _duration.duration();
    qCInfo(lcEngine) << "Server" << account()->capabilities().status().versionString()
                     << (account()->isHttp2Supported() ? "Using HTTP/2" : "");
    _progressInfo->_status = ProgressInfo::Discovery;
    Q_EMIT transmissionProgress(*_progressInfo);

    // TODO: add a constructor to DiscoveryPhase
    // pass a syncEngine object rather than copying everyhting to another object
    _discoveryPhase.reset(new DiscoveryPhase(_account, syncOptions(), _baseUrl));
    _discoveryPhase->_excludes = _excludedFiles.get();
    _discoveryPhase->_statedb = _journal;
    _discoveryPhase->_localDir = _localPath;
    if (!_discoveryPhase->_localDir.endsWith(QLatin1Char('/')))
        _discoveryPhase->_localDir+=QLatin1Char('/');
    _discoveryPhase->_remoteFolder = _remotePath;
    if (!_discoveryPhase->_remoteFolder.endsWith(QLatin1Char('/')))
        _discoveryPhase->_remoteFolder+=QLatin1Char('/');
    _discoveryPhase->_shouldDiscoverLocaly = [this](const QString &s) { return shouldDiscoverLocally(s); };
    _discoveryPhase->setSelectiveSyncBlackList(selectiveSyncBlackList);
    _discoveryPhase->setSelectiveSyncWhiteList(_journal->getSelectiveSyncList(SyncJournalDb::SelectiveSyncWhiteList, ok));
    if (!ok) {
        qCWarning(lcEngine) << "Unable to read selective sync list, aborting.";
        Q_EMIT syncError(tr("Unable to read from the sync journal."));
        finalize(false);
        return;
    }

    const QString invalidFilenamePattern = _account->capabilities().invalidFilenameRegex();
    if (!invalidFilenamePattern.isEmpty()) {
        _discoveryPhase->_invalidFilenameRx = QRegularExpression(invalidFilenamePattern);
    }
    _discoveryPhase->_serverBlacklistedFiles = _account->capabilities().blacklistedFiles();
    _discoveryPhase->_ignoreHiddenFiles = ignoreHiddenFiles();

    connect(_discoveryPhase.get(), &DiscoveryPhase::itemDiscovered, this, &SyncEngine::slotItemDiscovered);
    connect(_discoveryPhase.get(), &DiscoveryPhase::fatalError, this, [this](const QString &errorString) {
        Q_EMIT syncError(errorString);
        finalize(false);
    });
    connect(_discoveryPhase.get(), &DiscoveryPhase::finished, this, &SyncEngine::slotDiscoveryFinished);
    connect(_discoveryPhase.get(), &DiscoveryPhase::silentlyExcluded, _syncFileStatusTracker.data(), &SyncFileStatusTracker::slotAddSilentlyExcluded);
    connect(_discoveryPhase.get(), &DiscoveryPhase::excluded, _syncFileStatusTracker.data(), &SyncFileStatusTracker::slotAddSilentlyExcluded);
    connect(_discoveryPhase.get(), &DiscoveryPhase::excluded, this, &SyncEngine::excluded);

    auto discoveryJob = new ProcessDirectoryJob(_discoveryPhase.get(), PinState::AlwaysLocal, _discoveryPhase.get());
    _discoveryPhase->startJob(discoveryJob);
    connect(discoveryJob, &ProcessDirectoryJob::etag, this, &SyncEngine::slotRootEtagReceived);
}

void SyncEngine::slotFolderDiscovered(bool local, const QString &folder)
{
    // Don't wanna overload the UI
    if (!_lastUpdateProgressCallbackCall.isValid()) {
        _lastUpdateProgressCallbackCall.start(); // first call
    } else if (_lastUpdateProgressCallbackCall.elapsed() < 200) {
        return;
    } else {
        _lastUpdateProgressCallbackCall.start();
    }

    if (local) {
        _progressInfo->_currentDiscoveredLocalFolder = folder;
        _progressInfo->_currentDiscoveredRemoteFolder.clear();
    } else {
        _progressInfo->_currentDiscoveredRemoteFolder = folder;
        _progressInfo->_currentDiscoveredLocalFolder.clear();
    }
    Q_EMIT transmissionProgress(*_progressInfo);
}

void SyncEngine::slotRootEtagReceived(const QString &e, const QDateTime &time)
{
    if (_remoteRootEtag.isEmpty()) {
        qCDebug(lcEngine) << "Root etag:" << e;
        _remoteRootEtag = e;
        Q_EMIT rootEtag(_remoteRootEtag, time);
    }
}

void SyncEngine::slotNewItem(const SyncFileItemPtr &item)
{
    _progressInfo->adjustTotalsForFile(*item);
}

void SyncEngine::slotDiscoveryFinished()
{
    if (!_discoveryPhase) {
        // There was an error that was already taken care of
        return;
    }

    qCInfo(lcEngine) << "#### Discovery end ####################################################" << _duration.duration();

    // Sanity check
    if (!_journal->open()) {
        qCWarning(lcEngine) << "Bailing out, DB failure";
        Q_EMIT syncError(tr("Cannot open the sync journal"));
        finalize(false);
        return;
    } else {
        // Commits a possibly existing (should not though) transaction and starts a new one for the propagate phase
        _journal->commitIfNeededAndStartNewTransaction(QStringLiteral("Post discovery"));
    }

    _progressInfo->_currentDiscoveredRemoteFolder.clear();
    _progressInfo->_currentDiscoveredLocalFolder.clear();
    _progressInfo->_status = ProgressInfo::Reconcile;
    Q_EMIT transmissionProgress(*_progressInfo);

    //    qCInfo(lcEngine) << "Permissions of the root folder: " << _csync_ctx->remote.root_perms.toString();
    auto finish = [this]{


        auto databaseFingerprint = _journal->dataFingerprint();
        // If databaseFingerprint is empty, this means that there was no information in the database
        // (for example, upgrading from a previous version, or first sync, or server not supporting fingerprint)
        if (!databaseFingerprint.isEmpty() && _discoveryPhase
            && _discoveryPhase->_dataFingerprint != databaseFingerprint) {
            qCInfo(lcEngine) << "data fingerprint changed, assume restore from backup" << databaseFingerprint << _discoveryPhase->_dataFingerprint;
            restoreOldFiles(_syncItems);
        }

        if (_discoveryPhase->_anotherSyncNeeded) {
            _anotherSyncNeeded = true;
        }

        Q_ASSERT(std::is_sorted(_syncItems.begin(), _syncItems.end()));

        const auto regex = syncOptions().fileRegex();
        if (regex.isValid()) {
            QSet<QStringView> names;
            for (auto &i : _syncItems) {
                if (regex.match(i->_file).hasMatch()) {
                    int index = -1;
                    QStringView ref;
                    do {
                        ref = QStringView(i->_file).mid(0, index);
                        names.insert(ref);
                        index = ref.lastIndexOf(QLatin1Char('/'));
                    } while (index > 0);
                }
            }
            //std::erase_if c++20
            // https://en.cppreference.com/w/cpp/container/set/erase_if
            const auto erase_if = [](auto &c, const auto &pred) {
                auto old_size = c.size();
                for (auto i = c.begin(), last = c.end(); i != last;) {
                    if (pred(*i)) {
                        i = c.erase(i);
                    } else {
                        ++i;
                    }
                }
                return old_size - c.size();
            };
            erase_if(_syncItems, [&names](const SyncFileItemPtr &i) { return !names.contains(QStringView{i->_file}); });
        }

        qCInfo(lcEngine) << "#### Reconcile (aboutToPropagate) ####################################################" << _duration.duration();

        _localDiscoveryPaths.clear();

        // To announce the beginning of the sync
        Q_EMIT aboutToPropagate(_syncItems);

        qCInfo(lcEngine) << "#### Reconcile (aboutToPropagate OK) ####################################################" << _duration.duration();

        // it's important to do this before ProgressInfo::start(), to announce start of new sync
        _progressInfo->_status = ProgressInfo::Propagation;
        Q_EMIT transmissionProgress(*_progressInfo);
        _progressInfo->startEstimateUpdates();

        // do a database commit
        _journal->commit(QStringLiteral("post treewalk"));

        _propagator = QSharedPointer<OwncloudPropagator>::create(_account, syncOptions(), _baseUrl, _localPath, _remotePath, _journal);
        connect(_propagator.data(), &OwncloudPropagator::itemCompleted,
            this, &SyncEngine::slotItemCompleted);
        connect(_propagator.data(), &OwncloudPropagator::progress,
            this, &SyncEngine::slotProgress);
        connect(_propagator.data(), &OwncloudPropagator::updateFileTotal,
            this, &SyncEngine::updateFileTotal);
        connect(_propagator.data(), &OwncloudPropagator::finished, this, &SyncEngine::slotPropagationFinished, Qt::QueuedConnection);
        connect(_propagator.data(), &OwncloudPropagator::seenLockedFile, this, &SyncEngine::seenLockedFile);
        connect(_propagator.data(), &OwncloudPropagator::insufficientLocalStorage, this, &SyncEngine::slotInsufficientLocalStorage);
        connect(_propagator.data(), &OwncloudPropagator::insufficientRemoteStorage, this, &SyncEngine::slotInsufficientRemoteStorage);
        connect(_propagator.data(), &OwncloudPropagator::newItem, this, &SyncEngine::slotNewItem);

        // apply the network limits to the propagator
        setNetworkLimits(_uploadLimit, _downloadLimit);

        deleteStaleDownloadInfos(_syncItems);
        deleteStaleUploadInfos(_syncItems);
        deleteStaleErrorBlacklistEntries(_syncItems);
        _journal->commit(QStringLiteral("post stale entry removal"));

        // Emit the started signal only after the propagator has been set up.
        if (_needsUpdate)
            Q_EMIT started();

        _propagator->start(std::move(_syncItems));


        qCInfo(lcEngine) << "#### Post-Reconcile end ####################################################" << _duration.duration();
    };

    finish();
}

void SyncEngine::setNetworkLimits(int upload, int download)
{
    _uploadLimit = upload;
    _downloadLimit = download;

    if (_propagator) {
        if (upload != 0 || download != 0) {
            qCInfo(lcEngine) << "Network Limits (down/up) " << upload << download;
            if (!_propagator->_bandwidthManager) {
                _propagator->_bandwidthManager = new BandwidthManager(_propagator.data());
            }
        }
        // this might set the limit to 0 but only the next sync will have no bandwidth manager set
        if (_propagator->_bandwidthManager) {
            _propagator->_bandwidthManager->setCurrentDownloadLimit(download);
            _propagator->_bandwidthManager->setCurrentUploadLimit(upload);
        }
    }
}

void SyncEngine::slotItemCompleted(const SyncFileItemPtr &item)
{
    Q_ASSERT([&] {
        // ensure the item is not marked as finished twice
        const bool finished = item->_finished;
        item->_finished = true;
        return !finished;
    }());

    _progressInfo->setProgressComplete(*item);

    Q_EMIT transmissionProgress(*_progressInfo);
    Q_EMIT itemCompleted(item);
}

void SyncEngine::slotPropagationFinished(bool success)
{
    if (_propagator->_anotherSyncNeeded) {
        _anotherSyncNeeded = true;
    }

    if (success && _discoveryPhase) {
        _journal->setDataFingerprint(_discoveryPhase->_dataFingerprint);
    }

    conflictRecordMaintenance();

    // update placeholders for files that where marked as dirty in a previous run
    qCInfo(lcEngine) << "Updating files marked as dirty";
    for (const auto &record : _journal->getFileRecordsWithDirtyPlaceholders()) {
        _propagator->updateMetadata(*SyncFileItem::fromSyncJournalFileRecord(record));
    }

    _journal->deleteStaleFlagsEntries();
    _journal->commit(QStringLiteral("All Finished."), false);

    // Send final progress information even if no
    // files needed propagation, but clear the lastCompletedItem
    // so we don't count this twice (like Recent Files)
    _progressInfo->_lastCompletedItem = SyncFileItem();
    _progressInfo->_status = ProgressInfo::Done;
    Q_EMIT transmissionProgress(*_progressInfo);

    finalize(success);
}

void SyncEngine::finalize(bool success)
{
    qCInfo(lcEngine) << "Sync run took" << _duration.duration();
    _duration.stop();

    if (_discoveryPhase) {
        _discoveryPhase.release()->deleteLater();
    }
    _syncRunning = false;
    Q_EMIT finished(success);

    // Delete the propagator only after emitting the signal.
    _propagator.clear();
    _seenConflictFiles.clear();
    _uniqueErrors.clear();
    _localDiscoveryPaths.clear();
    _localDiscoveryStyle = LocalDiscoveryStyle::FilesystemOnly;
}

void SyncEngine::slotProgress(const SyncFileItem &item, qint64 current)
{
    _progressInfo->setProgressItem(item, current);
    Q_EMIT transmissionProgress(*_progressInfo);
}

void SyncEngine::updateFileTotal(const SyncFileItem &item, qint64 newSize)
{
    _progressInfo->updateTotalsForFile(item, newSize);
    Q_EMIT transmissionProgress(*_progressInfo);
}
void SyncEngine::restoreOldFiles(SyncFileItemSet &syncItems)
{
    /* When the server is trying to send us lots of file in the past, this means that a backup
       was restored in the server.  In that case, we should not simply overwrite the newer file
       on the file system with the older file from the backup on the server. Instead, we will
       upload the client file. But we still downloaded the old file in a conflict file just in case
    */

    for (auto it = syncItems.begin(); it != syncItems.end(); ++it) {
        if ((*it)->_direction != SyncFileItem::Down)
            continue;

        switch ((*it)->instruction()) {
        case CSYNC_INSTRUCTION_SYNC:
            qCWarning(lcEngine) << "restoreOldFiles: RESTORING" << (*it)->_file;
            (*it)->setInstruction(CSYNC_INSTRUCTION_CONFLICT);
            break;
        case CSYNC_INSTRUCTION_REMOVE:
            qCWarning(lcEngine) << "restoreOldFiles: RESTORING" << (*it)->_file;
            (*it)->setInstruction(CSYNC_INSTRUCTION_NEW);
            (*it)->_direction = SyncFileItem::Up;
            break;
        case CSYNC_INSTRUCTION_RENAME:
        case CSYNC_INSTRUCTION_NEW:
            // Ideally we should try to revert the rename or remove, but this would be dangerous
            // without re-doing the reconcile phase.  So just let it happen.
            break;
        default:
            break;
        }
    }
}

AccountPtr SyncEngine::account() const
{
    return _account;
}

void SyncEngine::setLocalDiscoveryOptions(LocalDiscoveryStyle style, std::set<QString> paths)
{
    _localDiscoveryStyle = style;
    _localDiscoveryPaths = std::move(paths);


    // add running upload to the local discovery
    const auto info = _journal->getUploadInfos();
    for (const auto &i : info) {
        _localDiscoveryPaths.insert(i._path);
    }

    // Normalize to make sure that no path is a contained in another.
    // Note: for simplicity, this code consider anything less than '/' as a path separator, so for
    // example, this will remove "foo.bar" if "foo" is in the list. This will mean we might have
    // some false positive, but that's Ok.
    // This invariant is used in SyncEngine::shouldDiscoverLocally
    QString prev;
    auto it = _localDiscoveryPaths.begin();
    while(it != _localDiscoveryPaths.end()) {
        if (!prev.isNull() && it->startsWith(prev) && (prev.endsWith(QLatin1Char('/')) || *it == prev || it->at(prev.size()) <= QLatin1Char('/'))) {
            it = _localDiscoveryPaths.erase(it);
        } else {
            prev = *it;
            ++it;
        }
    }
}

bool SyncEngine::shouldDiscoverLocally(const QString &path) const
{
    if (_localDiscoveryStyle == LocalDiscoveryStyle::FilesystemOnly) {
        return true;
    }

    // The intention is that if "A/X" is in _localDiscoveryPaths:
    // - parent folders like "/", "A" will be discovered (to make sure the discovery reaches the
    //   point where something new happened)
    // - the folder itself "A/X" will be discovered
    // - subfolders like "A/X/Y" will be discovered (so data inside a new or renamed folder will be
    //   discovered in full)
    // Check out TestLocalDiscovery::testLocalDiscoveryDecision()

    auto it = _localDiscoveryPaths.lower_bound(path);
    if (it == _localDiscoveryPaths.end() || !it->startsWith(path)) {
        // Maybe a subfolder of something in the list?
        if (it != _localDiscoveryPaths.begin() && path.startsWith(*(--it))) {
            return it->endsWith(QLatin1Char('/')) || (path.size() > it->size() && path.at(it->size()) <= QLatin1Char('/'));
        }
        return false;
    }

    // maybe an exact match or an empty path?
    if (it->size() == path.size() || path.isEmpty())
        return true;

    // Maybe a parent folder of something in the list?
    // check for a prefix + / match
    while (true) {
        if (it->size() > path.size() && it->at(path.size()) == QLatin1Char('/'))
            return true;
        ++it;
        if (it == _localDiscoveryPaths.end() || !it->startsWith(path))
            return false;
    }
    return false;
}

void SyncEngine::abort(const QString &reason)
{
    bool aborting = false;
    if (_propagator) {
        aborting = true;
        // If we're already in the propagation phase, aborting that is sufficient
        _propagator->abort();
    } else if (_discoveryPhase) {
        aborting = true;
        // Delete the discovery and all child jobs after ensuring
        // it can't finish and start the propagator
        disconnect(_discoveryPhase.get(), nullptr, this, nullptr);
    }
    if (aborting) {
        qCInfo(lcEngine) << "Aborting sync, stated reason:" << reason;
        if (!_goingDown) {
            Q_EMIT syncError(tr("Aborted due to %1").arg(reason));
        }
        finalize(false);
    }
}

void SyncEngine::slotSummaryError(const QString &message)
{
    if (_uniqueErrors.contains(message))
        return;

    _uniqueErrors.insert(message);
    Q_EMIT syncError(message, ErrorCategory::Normal);
}

void SyncEngine::slotInsufficientLocalStorage()
{
    slotSummaryError(
        tr("Disk space is low: Downloads that would reduce free space "
           "below %1 were skipped.")
            .arg(Utility::octetsToString(freeSpaceLimit())));
}

void SyncEngine::slotInsufficientRemoteStorage()
{
    auto msg = tr("There is insufficient space available on the server for some uploads.");
    if (_uniqueErrors.contains(msg))
        return;

    _uniqueErrors.insert(msg);
    Q_EMIT syncError(msg, ErrorCategory::InsufficientRemoteStorage);
}

bool SyncEngine::isExcluded(QStringView filePath) const
{
    Q_ASSERT(QDir::isAbsolutePath(filePath.toString()));
    return _excludedFiles->isExcluded(filePath, localPath(), ignoreHiddenFiles());
}

bool SyncEngine::loadDefaultExcludes()
{
    ConfigFile::setupDefaultExcludeFilePaths(*_excludedFiles);
    return _excludedFiles->reloadExcludeFiles();
}

void SyncEngine::clearManualExcludes()
{
    _excludedFiles->clearManualExcludes();
}

bool SyncEngine::reloadExcludes()
{
    return _excludedFiles->reloadExcludeFiles();
}

void SyncEngine::addExcludeList(const QString &filePath)
{
    _excludedFiles->addExcludeFilePath(filePath);
}

void SyncEngine::addManualExclude(const QString &filePath)
{
    _excludedFiles->addManualExclude(filePath);
}

} // namespace OCC
