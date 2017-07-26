/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 * Copyright (C) by Klaas Freitag <freitag@owncloud.org>
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
#include "config.h"

#include "account.h"
#include "accountstate.h"
#include "abstractfolder.h"
#include "folderman.h"
#include "logger.h"
#include "configfile.h"
#include "networkjobs.h"
#include "syncjournalfilerecord.h"
#include "syncresult.h"
#include "utility.h"
#include "clientproxy.h"
#include "syncengine.h"
#include "syncrunfilelog.h"
#include "socketapi.h"
#include "theme.h"
#include "filesystem.h"
#include "excludedfiles.h"

#include "creds/abstractcredentials.h"

#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QSettings>

#include <QMessageBox>
#include <QPushButton>

namespace OCC {

Q_LOGGING_CATEGORY(lcAbstractFolder, "gui.abstractfolder", QtInfoMsg)

AbstractFolder::AbstractFolder(const FolderDefinition &definition,
                               AccountState *accountState,
                               QObject *parent)
    : QObject(parent)
    , _accountState(accountState)
    , _definition(definition)
    , _proxyDirty(true)
    , _lastSyncDuration(0)
    , _consecutiveFailingSyncs(0)
    , _consecutiveFollowUpSyncs(0)
    , _journal(_definition.absoluteJournalPath())
    , _fileLog(new SyncRunFileLog)
    , _saveBackwardsCompatible(false)
{
    qsrand(QTime::currentTime().msec());
    _timeSinceLastSyncStart.start();
    _timeSinceLastSyncDone.start();

    SyncResult::Status status = SyncResult::NotYetStarted;
    if (definition.paused) {
        status = SyncResult::Paused;
    }
    _syncResult.setStatus(status);

    // check if the local path exists
    checkLocalPath();

    _syncResult.setFolder(_definition.alias);

    _engine.reset(new SyncEngine(_accountState->account(), path(), remotePath(), &_journal));
    // pass the setting if hidden files are to be ignored, will be read in csync_update
    _engine->setIgnoreHiddenFiles(_definition.ignoreHiddenFiles);

    connect(_accountState.data(), SIGNAL(isConnectedChanged()), this, SIGNAL(canSyncChanged()));

    connect(_engine.data(), SIGNAL(started()), SLOT(slotSyncStarted()), Qt::QueuedConnection);
    connect(_engine.data(), SIGNAL(finished(bool)), SLOT(slotSyncFinished(bool)), Qt::QueuedConnection);
    // connect(_engine.data(), SIGNAL(folderDiscovered(bool, QString)), this, SLOT(slotFolderDiscovered(bool, QString)));
    connect(_engine.data(), SIGNAL(transmissionProgress(ProgressInfo)), this, SLOT(slotTransmissionProgress(ProgressInfo)));
    connect(_engine.data(), SIGNAL(itemCompleted(const SyncFileItemPtr &)),
            this, SLOT(slotItemCompleted(const SyncFileItemPtr &)));
    connect(_engine.data(), SIGNAL(seenLockedFile(QString)), FolderMan::instance(), SLOT(slotSyncOnceFileUnlocks(QString)));
    connect(_engine.data(), SIGNAL(aboutToPropagate(SyncFileItemVector &)),
        SLOT(slotLogPropagationStart()));

    _scheduleSelfTimer.setSingleShot(true);
    _scheduleSelfTimer.setInterval(SyncEngine::minimumFileAgeForUpload);
    connect(&_scheduleSelfTimer, SIGNAL(timeout()),
        SLOT(slotScheduleThisFolder()));
}

AbstractFolder::~AbstractFolder()
{
    // Reset then engine first as it will abort and try to access members of the Folder
    _engine.reset();
}


void AbstractFolder::checkLocalPath()
{
    const QFileInfo fi(_definition.localPath);
    _canonicalLocalPath = fi.canonicalFilePath();
    if (_canonicalLocalPath.isEmpty()) {
        qCWarning(lcAbstractFolder) << "Broken symlink:" << _definition.localPath;
        _canonicalLocalPath = _definition.localPath;
    } else if (!_canonicalLocalPath.endsWith('/')) {
        _canonicalLocalPath.append('/');
    }

    if (fi.isDir() && fi.isReadable()) {
        qCDebug(lcAbstractFolder) << "Checked local path ok";
    } else {
        // Check directory again
        if (!FileSystem::fileExists(_definition.localPath, fi)) {
            _syncResult.appendErrorString(tr("Local folder %1 does not exist.").arg(_definition.localPath));
            _syncResult.setStatus(SyncResult::SetupError);
        } else if (!fi.isDir()) {
            _syncResult.appendErrorString(tr("%1 should be a folder but is not.").arg(_definition.localPath));
            _syncResult.setStatus(SyncResult::SetupError);
        } else if (!fi.isReadable()) {
            _syncResult.appendErrorString(tr("%1 is not readable.").arg(_definition.localPath));
            _syncResult.setStatus(SyncResult::SetupError);
        }
    }
}

QString AbstractFolder::shortGuiRemotePathOrAppName() const
{
    if (remotePath().length() > 0 && remotePath() != QLatin1String("/")) {
        QString a = QFile(remotePath()).fileName();
        if (a.startsWith('/')) {
            a = a.remove(0, 1);
        }
        return a;
    } else {
        return Theme::instance()->appNameGUI();
    }
}

QString AbstractFolder::alias() const
{
    return _definition.alias;
}

QString AbstractFolder::path() const
{
    return _canonicalLocalPath;
}

QString AbstractFolder::shortGuiLocalPath() const
{
    QString p = _definition.localPath;
    QString home = QDir::homePath();
    if (!home.endsWith('/')) {
        home.append('/');
    }
    if (p.startsWith(home)) {
        p = p.mid(home.length());
    }
    if (p.length() > 1 && p.endsWith('/')) {
        p.chop(1);
    }
    return QDir::toNativeSeparators(p);
}

QString AbstractFolder::cleanPath() const
{
    QString cleanedPath = QDir::cleanPath(_canonicalLocalPath);

    if (cleanedPath.length() == 3 && cleanedPath.endsWith(":/"))
        cleanedPath.remove(2, 1);

    return cleanedPath;
}

bool AbstractFolder::isBusy() const
{
    return _engine->isSyncRunning();
}

QString AbstractFolder::remotePath() const
{
    return _definition.targetPath;
}

QUrl AbstractFolder::remoteUrl() const
{
    return Utility::concatUrlPath(_accountState->account()->davUrl(), remotePath());
}

bool AbstractFolder::syncPaused() const
{
    return _definition.paused;
}

bool AbstractFolder::canSync() const
{
    return !syncPaused() && accountState()->isConnected();
}

void AbstractFolder::setSyncPaused(bool paused)
{
    if (paused == _definition.paused) {
        return;
    }

    _definition.paused = paused;
    saveToSettings();

    if (!paused) {
        setSyncState(SyncResult::NotYetStarted);
    } else {
        setSyncState(SyncResult::Paused);
    }
    emit syncPausedChanged(this, paused);
    emit syncStateChange();
    emit canSyncChanged();
}

void AbstractFolder::setSyncState(SyncResult::Status state)
{
    _syncResult.setStatus(state);
}

SyncResult AbstractFolder::syncResult() const
{
    return _syncResult;
}

void AbstractFolder::prepareToSync()
{
    _syncResult.reset();
    _syncResult.setStatus(SyncResult::NotYetStarted);
}

void AbstractFolder::showSyncResultPopup()
{
    if (_syncResult.firstItemNew()) {
        createGuiLog(_syncResult.firstItemNew()->_file, LogStatusNew, _syncResult.numNewItems());
    }
    if (_syncResult.firstItemDeleted()) {
        createGuiLog(_syncResult.firstItemDeleted()->_file, LogStatusRemove, _syncResult.numRemovedItems());
    }
    if (_syncResult.firstItemUpdated()) {
        createGuiLog(_syncResult.firstItemUpdated()->_file, LogStatusUpdated, _syncResult.numUpdatedItems());
    }

    if (_syncResult.firstItemRenamed()) {
        LogStatus status(LogStatusRename);
        // if the path changes it's rather a move
        QDir renTarget = QFileInfo(_syncResult.firstItemRenamed()->_renameTarget).dir();
        QDir renSource = QFileInfo(_syncResult.firstItemRenamed()->_file).dir();
        if (renTarget != renSource) {
            status = LogStatusMove;
        }
        createGuiLog(_syncResult.firstItemRenamed()->_originalFile, status,
            _syncResult.numRenamedItems(), _syncResult.firstItemRenamed()->_renameTarget);
    }

    if (_syncResult.firstNewConflictItem()) {
        createGuiLog(_syncResult.firstNewConflictItem()->_file, LogStatusConflict, _syncResult.numNewConflictItems());
    }
    if (int errorCount = _syncResult.numErrorItems()) {
        createGuiLog(_syncResult.firstItemError()->_file, LogStatusError, errorCount);
    }

    qCInfo(lcAbstractFolder) << "Folder sync result: " << int(_syncResult.status());
}

void AbstractFolder::createGuiLog(const QString &filename, LogStatus status, int count,
    const QString &renameTarget)
{
    if (count > 0) {
        Logger *logger = Logger::instance();

        QString file = QDir::toNativeSeparators(filename);
        QString text;

        switch (status) {
        case LogStatusRemove:
            if (count > 1) {
                text = tr("%1 and %n other file(s) have been removed.", "", count - 1).arg(file);
            } else {
                text = tr("%1 has been removed.", "%1 names a file.").arg(file);
            }
            break;
        case LogStatusNew:
            if (count > 1) {
                text = tr("%1 and %n other file(s) have been downloaded.", "", count - 1).arg(file);
            } else {
                text = tr("%1 has been downloaded.", "%1 names a file.").arg(file);
            }
            break;
        case LogStatusUpdated:
            if (count > 1) {
                text = tr("%1 and %n other file(s) have been updated.", "", count - 1).arg(file);
            } else {
                text = tr("%1 has been updated.", "%1 names a file.").arg(file);
            }
            break;
        case LogStatusRename:
            if (count > 1) {
                text = tr("%1 has been renamed to %2 and %n other file(s) have been renamed.", "", count - 1).arg(file, renameTarget);
            } else {
                text = tr("%1 has been renamed to %2.", "%1 and %2 name files.").arg(file, renameTarget);
            }
            break;
        case LogStatusMove:
            if (count > 1) {
                text = tr("%1 has been moved to %2 and %n other file(s) have been moved.", "", count - 1).arg(file, renameTarget);
            } else {
                text = tr("%1 has been moved to %2.").arg(file, renameTarget);
            }
            break;
        case LogStatusConflict:
            if (count > 1) {
                text = tr("%1 has and %n other file(s) have sync conflicts.", "", count - 1).arg(file);
            } else {
                text = tr("%1 has a sync conflict. Please check the conflict file!").arg(file);
            }
            break;
        case LogStatusError:
            if (count > 1) {
                text = tr("%1 and %n other file(s) could not be synced due to errors. See the log for details.", "", count - 1).arg(file);
            } else {
                text = tr("%1 could not be synced due to an error. See the log for details.").arg(file);
            }
            break;
        }

        if (!text.isEmpty()) {
            logger->postOptionalGuiLog(tr("Sync Activity"), text);
        }
    }
}

int AbstractFolder::slotDiscardDownloadProgress()
{
    // Delete from journal and from filesystem.
    QDir folderpath(_definition.localPath);
    QSet<QString> keep_nothing;
    const QVector<SyncJournalDb::DownloadInfo> deleted_infos =
        _journal.getAndDeleteStaleDownloadInfos(keep_nothing);
    foreach (const SyncJournalDb::DownloadInfo &deleted_info, deleted_infos) {
        const QString tmppath = folderpath.filePath(deleted_info._tmpfile);
        qCInfo(lcAbstractFolder) << "Deleting temporary file: " << tmppath;
        FileSystem::remove(tmppath);
    }
    return deleted_infos.size();
}

int AbstractFolder::downloadInfoCount()
{
    return _journal.downloadInfoCount();
}

int AbstractFolder::errorBlackListEntryCount()
{
    return _journal.errorBlackListEntryCount();
}

int AbstractFolder::slotWipeErrorBlacklist()
{
    return _journal.wipeErrorBlacklist();
}

void AbstractFolder::slotWatchedPathChanged(const QString &path)
{
// The folder watcher fires a lot of bogus notifications during
// a sync operation, both for actual user files and the database
// and log. Therefore we check notifications against operations
// the sync is doing to filter out our own changes.
#ifdef Q_OS_MAC
    Q_UNUSED(path)
// On OSX the folder watcher does not report changes done by our
// own process. Therefore nothing needs to be done here!
#else
    // Use the path to figure out whether it was our own change
    const auto maxNotificationDelay = 15 * 1000;
    qint64 time = _engine->timeSinceFileTouched(path);
    if (time != -1 && time < maxNotificationDelay) {
        return;
    }
#endif

    // Check that the mtime actually changed.
    if (path.startsWith(this->path())) {
        auto relativePath = path.mid(this->path().size());
        auto record = _journal.getFileRecord(relativePath);
        if (record.isValid() && !FileSystem::fileChanged(path, record._fileSize, Utility::qDateTimeToTime_t(record._modtime))) {
            qCInfo(lcAbstractFolder) << "Ignoring spurious notification for file" << relativePath;
            return; // probably a spurious notification
        }
    }

    emit watchedFileChangedExternally(path);

    // Also schedule this folder for a sync, but only after some delay:
    // The sync will not upload files that were changed too recently.
    scheduleThisFolderSoon();
}

void AbstractFolder::saveToSettings() const
{
    // Remove first to make sure we don't get duplicates
    removeFromSettings();

    auto settings = _accountState->settings();

    // The folder is saved to backwards-compatible "Folders"
    // section only if it has the migrate flag set (i.e. was in
    // there before) or if the folder is the only one for the
    // given target path.
    // This ensures that older clients will not read a configuration
    // where two folders for different accounts point at the same
    // local folders.
    bool oneAccountOnly = true;
    foreach (AbstractFolder *other, FolderMan::instance()->map()) {
        if (other != this && other->cleanPath() == this->cleanPath()) {
            oneAccountOnly = false;
            break;
        }
    }

    bool compatible = _saveBackwardsCompatible || oneAccountOnly;

    if (compatible) {
        settings->beginGroup(QLatin1String("Folders"));
    } else {
        settings->beginGroup(QLatin1String("Multifolders"));
    }
    FolderDefinition::save(*settings, _definition);

    settings->sync();
    qCInfo(lcAbstractFolder) << "Saved folder" << _definition.alias << "to settings, status" << settings->status();
}

void AbstractFolder::removeFromSettings() const
{
    auto settings = _accountState->settings();
    settings->beginGroup(QLatin1String("Folders"));
    settings->remove(FolderMan::escapeAlias(_definition.alias));
    settings->endGroup();
    settings->beginGroup(QLatin1String("Multifolders"));
    settings->remove(FolderMan::escapeAlias(_definition.alias));
}

void AbstractFolder::slotTerminateSync()
{
    qCInfo(lcAbstractFolder) << "folder " << alias() << " Terminating!";

    if (_engine->isSyncRunning()) {
        _engine->abort();

        setSyncState(SyncResult::SyncAbortRequested);
    }
}

// This removes the csync File database
// This is needed to provide a clean startup again in case another
// local folder is synced to the same ownCloud.
void AbstractFolder::wipe()
{
    QString stateDbFile = _engine->journal()->databaseFilePath();

    // Delete files that have been partially downloaded.
    slotDiscardDownloadProgress();

    //Unregister the socket API so it does not keep the ._sync_journal file open
    FolderMan::instance()->socketApi()->slotUnregisterPath(alias());
    _journal.close(); // close the sync journal

    QFile file(stateDbFile);
    if (file.exists()) {
        if (!file.remove()) {
            qCWarning(lcAbstractFolder) << "Failed to remove existing csync StateDB " << stateDbFile;
        } else {
            qCInfo(lcAbstractFolder) << "wipe: Removed csync StateDB " << stateDbFile;
        }
    } else {
        qCWarning(lcAbstractFolder) << "statedb is empty, can not remove.";
    }

    // Also remove other db related files
    QFile::remove(stateDbFile + ".ctmp");
    QFile::remove(stateDbFile + "-shm");
    QFile::remove(stateDbFile + "-wal");
    QFile::remove(stateDbFile + "-journal");

    if (canSync())
        FolderMan::instance()->socketApi()->slotRegisterPath(alias());
}

void AbstractFolder::setProxyDirty(bool value)
{
    _proxyDirty = value;
}

bool AbstractFolder::proxyDirty()
{
    return _proxyDirty;
}

void AbstractFolder::startSync(const QStringList &pathList)
{
    Q_UNUSED(pathList)
    if (proxyDirty()) {
        setProxyDirty(false);
    }

    if (isBusy()) {
        qCCritical(lcAbstractFolder) << "ERROR csync is still running and new sync requested.";
        return;
    }

    _timeSinceLastSyncStart.start();
    _syncResult.setStatus(SyncResult::SyncPrepare);
    emit syncStateChange();

    qCInfo(lcAbstractFolder) << "*** Start syncing " << remoteUrl().toString() << " - client version"
                     << qPrintable(Theme::instance()->version());

    _fileLog->start(path());

#if 0
    // FIXME: Rethink this: why is this here?
    if (!setIgnoredFiles()) {
        slotSyncError(tr("Could not read system exclude file"));
        QMetaObject::invokeMethod(this, "slotSyncFinished", Qt::QueuedConnection, Q_ARG(bool, false));
        return;
    }
#endif

    setDirtyNetworkLimits();
    setSyncOptions();

    _engine->setIgnoreHiddenFiles(_definition.ignoreHiddenFiles);

    QMetaObject::invokeMethod(_engine.data(), "startSync", Qt::QueuedConnection);

    emit syncStarted();
}

void AbstractFolder::setSyncOptions()
{
    SyncOptions opt;
    ConfigFile cfgFile;

    auto newFolderLimit = cfgFile.newBigFolderSizeLimit();
    opt._newBigFolderSizeLimit = newFolderLimit.first ? newFolderLimit.second * 1000LL * 1000LL : -1; // convert from MB to B
    opt._confirmExternalStorage = cfgFile.confirmExternalStorage();

    QByteArray chunkSizeEnv = qgetenv("OWNCLOUD_CHUNK_SIZE");
    if (!chunkSizeEnv.isEmpty()) {
        opt._initialChunkSize = chunkSizeEnv.toUInt();
    } else {
        opt._initialChunkSize = cfgFile.chunkSize();
    }
    QByteArray minChunkSizeEnv = qgetenv("OWNCLOUD_MIN_CHUNK_SIZE");
    if (!minChunkSizeEnv.isEmpty()) {
        opt._minChunkSize = minChunkSizeEnv.toUInt();
    } else {
        opt._minChunkSize = cfgFile.minChunkSize();
    }
    QByteArray maxChunkSizeEnv = qgetenv("OWNCLOUD_MAX_CHUNK_SIZE");
    if (!maxChunkSizeEnv.isEmpty()) {
        opt._maxChunkSize = maxChunkSizeEnv.toUInt();
    } else {
        opt._maxChunkSize = cfgFile.maxChunkSize();
    }

    // Previously min/max chunk size values didn't exist, so users might
    // have setups where the chunk size exceeds the new min/max default
    // values. To cope with this, adjust min/max to always include the
    // initial chunk size value.
    opt._minChunkSize = qMin(opt._minChunkSize, opt._initialChunkSize);
    opt._maxChunkSize = qMax(opt._maxChunkSize, opt._initialChunkSize);

    QByteArray targetChunkUploadDurationEnv = qgetenv("OWNCLOUD_TARGET_CHUNK_UPLOAD_DURATION");
    if (!targetChunkUploadDurationEnv.isEmpty()) {
        opt._targetChunkUploadDuration = targetChunkUploadDurationEnv.toUInt();
    } else {
        opt._targetChunkUploadDuration = cfgFile.targetChunkUploadDuration();
    }

    _engine->setSyncOptions(opt);
}

void AbstractFolder::setDirtyNetworkLimits()
{
    ConfigFile cfg;
    int downloadLimit = -75; // 75%
    int useDownLimit = cfg.useDownloadLimit();
    if (useDownLimit >= 1) {
        downloadLimit = cfg.downloadLimit() * 1000;
    } else if (useDownLimit == 0) {
        downloadLimit = 0;
    }

    int uploadLimit = -75; // 75%
    int useUpLimit = cfg.useUploadLimit();
    if (useUpLimit >= 1) {
        uploadLimit = cfg.uploadLimit() * 1000;
    } else if (useUpLimit == 0) {
        uploadLimit = 0;
    }

    _engine->setNetworkLimits(uploadLimit, downloadLimit);
}


void AbstractFolder::slotSyncError(const QString &err)
{
    _syncResult.appendErrorString(err);
}

void AbstractFolder::slotSyncStarted()
{
    qCInfo(lcAbstractFolder) << "#### Propagation start ####################################################";
    _syncResult.setStatus(SyncResult::SyncRunning);
    emit syncStateChange();
}

void AbstractFolder::slotSyncFinished(bool success)
{
    qCInfo(lcAbstractFolder) << "Client version" << qPrintable(Theme::instance()->version())
                     << " Qt" << qVersion()
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                     << " SSL " << QSslSocket::sslLibraryVersionString().toUtf8().data()
#endif
        ;

    bool syncError = !_syncResult.errorStrings().isEmpty();
    if (syncError) {
        qCWarning(lcAbstractFolder) << "SyncEngine finished with ERROR";
    } else {
        qCInfo(lcAbstractFolder) << "SyncEngine finished without problem.";
    }
    _fileLog->finish();
    showSyncResultPopup();

    auto anotherSyncNeeded = _engine->isAnotherSyncNeeded();

    if (syncError) {
        _syncResult.setStatus(SyncResult::Error);
#if 0
        // FIXME: There needs to be some way of error reporting from the folder
        // implementation to here, as with this csyncUnavail
    } else if (_csyncUnavail) {
        _syncResult.setStatus(SyncResult::Error);
        qCWarning(lcAbstractFolder) << "csync not available.";
#endif
    }
    else if (_syncResult.foundFilesNotSynced()) {
        _syncResult.setStatus(SyncResult::Problem);
    } else if (_definition.paused) {
        // Maybe the sync was terminated because the user paused the folder
        _syncResult.setStatus(SyncResult::Paused);
    } else {
        _syncResult.setStatus(SyncResult::Success);
    }

    // Count the number of syncs that have failed in a row.
    if (_syncResult.status() == SyncResult::Success
        || _syncResult.status() == SyncResult::Problem) {
        _consecutiveFailingSyncs = 0;
    } else {
        _consecutiveFailingSyncs++;
        qCInfo(lcAbstractFolder) << "the last" << _consecutiveFailingSyncs << "syncs failed";
    }

    if (_syncResult.status() == SyncResult::Success && success) {
        // Clear the white list as all the folders that should be on that list are sync-ed
        journalDb()->setSelectiveSyncList(SyncJournalDb::SelectiveSyncWhiteList, QStringList());
    }

    emit syncStateChange();

    // The syncFinished result that is to be triggered here makes the folderman
    // clear the current running sync folder marker.
    // Lets wait a bit to do that because, as long as this marker is not cleared,
    // file system change notifications are ignored for that folder. And it takes
    // some time under certain conditions to make the file system notifications
    // all come in.
    QTimer::singleShot(200, this, SLOT(slotEmitFinishedDelayed()));

    _lastSyncDuration = _timeSinceLastSyncStart.elapsed();
    _timeSinceLastSyncDone.start();

    // Increment the follow-up sync counter if necessary.
    if (anotherSyncNeeded == ImmediateFollowUp) {
        _consecutiveFollowUpSyncs++;
        qCInfo(lcAbstractFolder) << "another sync was requested by the finished sync, this has"
                         << "happened" << _consecutiveFollowUpSyncs << "times";
    } else {
        _consecutiveFollowUpSyncs = 0;
    }

    // Maybe force a follow-up sync to take place, but only a couple of times.
    if (anotherSyncNeeded == ImmediateFollowUp && _consecutiveFollowUpSyncs <= 3) {
        // Sometimes another sync is requested because a local file is still
        // changing, so wait at least a small amount of time before syncing
        // the folder again.
        scheduleThisFolderSoon();
    }
}

void AbstractFolder::slotEmitFinishedDelayed()
{
    emit syncFinished(_syncResult);
}


void AbstractFolder::slotFolderDiscovered(bool, QString folderName)
{
    ProgressInfo pi;
    pi._currentDiscoveredFolder = folderName;
    emit progressInfo(pi);
    ProgressDispatcher::instance()->setProgressInfo(alias(), pi);
}


// the progress comes without a folder and the valid path set. Add that here
// and hand the result over to the progress dispatcher.
void AbstractFolder::slotTransmissionProgress(const ProgressInfo &pi)
{
    emit progressInfo(pi);
    ProgressDispatcher::instance()->setProgressInfo(alias(), pi);
}

// a item is completed: count the errors and forward to the ProgressDispatcher
void AbstractFolder::slotItemCompleted(const SyncFileItemPtr &item)
{
    // add new directories or remove gone away dirs to the watcher
    if (item->_isDirectory && item->_instruction == CSYNC_INSTRUCTION_NEW) {
        FolderMan::instance()->addMonitorPath(alias(), path() + item->_file);
    }
    if (item->_isDirectory && item->_instruction == CSYNC_INSTRUCTION_REMOVE) {
        FolderMan::instance()->removeMonitorPath(alias(), path() + item->_file);
    }

    _syncResult.processCompletedItem(item);

    _fileLog->logItem(*item);
    emit ProgressDispatcher::instance()->itemCompleted(alias(), item);
}

void AbstractFolder::slotLogPropagationStart()
{
    _fileLog->logLap("Propagation starts");
}

void AbstractFolder::slotScheduleThisFolder()
{
    FolderMan::instance()->scheduleFolder(this);
}

void AbstractFolder::scheduleThisFolderSoon()
{
    if (!_scheduleSelfTimer.isActive()) {
        _scheduleSelfTimer.start();
    }
}

void AbstractFolder::setSaveBackwardsCompatible(bool save)
{
    _saveBackwardsCompatible = save;
}

} // namespace OCC
