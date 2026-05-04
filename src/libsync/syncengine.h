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

#pragma once

#include "account.h"
#include "common/checksums.h"
#include "common/chronoelapsedtimer.h"
#include "discoveryphase.h"
#include "progressdispatcher.h"
#include "syncfileitem.h"
#include "syncfilestatustracker.h"

#include <QMutex>
#include <QThread>
#include <QString>
#include <QSet>
#include <QMap>
#include <QStringList>
#include <QPointer>

#include <optional>
#include <set>

class QProcess;

namespace OCC {

class SyncJournalFileRecord;
class SyncJournalDb;
class OwncloudPropagator;
class ProcessDirectoryJob;

/**
 * @brief The SyncEngine class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT SyncEngine : public QObject
{
    Q_OBJECT

public:
    SyncEngine(Account *account, const QUrl &baseUrl, const QString &localPath, const QString &remotePath, SyncJournalDb *journal, QObject *parent);
    ~SyncEngine() override;

    Q_INVOKABLE void startSync();

    /* Abort the sync. Called from the main thread */
    void abort(const QString &reason);

    bool isSyncRunning() const { return _syncRunning; }

    const SyncOptions &syncOptions() const { return _syncOptions; }
    void setSyncOptions(const SyncOptions &options)
    {
        Q_ASSERT(options.isValid());
        _syncOptions = options;
    }
    bool ignoreHiddenFiles() const { return _ignore_hidden_files; }
    void setIgnoreHiddenFiles(bool ignore) { _ignore_hidden_files = ignore; }

    void setMoveToTrash(bool trashIt) { _moveToTrash = trashIt; }

    // file path must be absolute!
    bool isExcluded(QStringView filePath) const;
    void addManualExclude(const QString &filePath);
    void addExcludeList(const QString &filePath);
    bool loadDefaultExcludes();
    bool reloadExcludes();
    void clearManualExcludes();

    SyncFileStatus fileStatus(const QString &relativePath);
    void pathTouched(const QString &fileName);


    /* Returns whether another sync is needed to complete the sync */
    bool isAnotherSyncNeeded() { return _anotherSyncNeeded; }

    // SyncJournalDb *journal() const { return _journal; }
    QString localPath() const { return _localPath; }

    /** Duration in ms that uploads should be delayed after a file change
     *
     * In certain situations a file can be written to very regularly over a large
     * amount of time. Copying a large file could take a while. A logfile could be
     * updated every second.
     *
     * In these cases it isn't desirable to attempt to upload the "unfinished" file.
     * To avoid that, uploads of files where the distance between the mtime and the
     * current time is less than this duration are skipped.
     */
    static std::chrono::seconds minimumFileAgeForUpload;

    /**
     * Control whether local discovery should read from filesystem or db.
     *
     * If style is DatabaseAndFilesystem, paths a set of file paths relative to
     * the synced folder. All the parent directories of these paths will not
     * be read from the db and scanned on the filesystem.
     *
     * Note, the style and paths are only retained for the next sync and
     * revert afterwards. Use _lastLocalDiscoveryStyle to discover the last
     * sync's style.
     */
    void setLocalDiscoveryOptions(LocalDiscoveryStyle style, std::set<QString> paths = {});

    /**
     * Returns whether the given folder-relative path should be locally discovered
     * given the local discovery options.
     *
     * Example: If path is 'foo/bar' and style is DatabaseAndFilesystem and dirs contains
     *     'foo/bar/touched_file', then the result will be true.
     */
    bool shouldDiscoverLocally(const QString &path) const;

    /** Access the last sync run's local discovery style */
    LocalDiscoveryStyle lastLocalDiscoveryStyle() const { return _lastLocalDiscoveryStyle; }


Q_SIGNALS:

    void fileStatusChanged(const QString &systemFileName, SyncFileStatus fileStatus);

    // During update, before reconcile
    void rootEtagDiscovered(const QString &, const QDateTime &);

    // after the above signals. with the items that actually need propagating
    void aboutToPropagate(const SyncFileItemSet &items);

    // after each item completed by a job (successful or not)
    void itemCompleted(const SyncFileItemPtr &);

    void transmissionProgress(const ProgressInfo &progress);

    /// We've produced a new sync error of a type.
    void syncError(const QString &message, ErrorCategory category = ErrorCategory::Normal);
    void excluded(const QString &path);

    void finished(bool success);
    void started();

    /** Emitted when propagation has problems with a locked file.
     *
     * Forwarded from OwncloudPropagator::seenLockedFile.
     */
    void seenLockedFile(const QString &fileName, FileSystem::LockMode mode);

private Q_SLOTS:
    void slotFolderDiscovered(bool local, const QString &folder);
    void slotRootEtagReceived(const QString &, const QDateTime &time);

    /** When the discovery phase discovers an item */
    void slotItemDiscovered(const SyncFileItemPtr &item);

    /** Called when a SyncFileItem gets accepted for a sync.
     *
     * Mostly done in initial creation inside treewalkFile but
     * can also be called via the propagator for items that are
     * created during propagation.
     */
    void slotNewItem(const SyncFileItemPtr &item);

    void slotItemCompleted(const SyncFileItemPtr &item);
    void slotDiscoveryFinished();
    void slotPropagationFinished(bool success);
    void slotProgress(const SyncFileItem &item, qint64 curent);
    void updateFileTotal(const SyncFileItem &item, qint64 newSize);

    /** Emit a summary error, unless it was seen before */
    void slotSummaryError(const QString &message);

    void slotInsufficientLocalStorage();
    void slotInsufficientRemoteStorage();

private:
    bool checkErrorBlacklisting(SyncFileItem &item);

    // Cleans up unnecessary downloadinfo entries in the journal as well
    // as their temporary files.
    void deleteStaleDownloadInfos(const SyncFileItemSet &syncItems);

    // Removes stale uploadinfos from the journal.
    void deleteStaleUploadInfos(const SyncFileItemSet &syncItems);

    // Removes stale error blacklist entries from the journal.
    void deleteStaleErrorBlacklistEntries(const SyncFileItemSet &syncItems);

    // Removes stale and adds missing conflict records after sync
    void conflictRecordMaintenance();

    // cleanup and Q_EMIT the finished signal
    void finalize(bool success);

    // Must only be acessed during update and reconcile
    SyncFileItemSet _syncItems;

    // this seems to be the folder davUrl - verifying before rename
    const QUrl _baseUrl;

    bool _needsUpdate;
    bool _syncRunning;
    QString _localPath;
    QString _remotePath;
    QString _remoteRootEtag;

    // the ever present account pointer primarily used for running jobs
    QPointer<Account> _account;
    // this is owned by the folder
    QPointer<SyncJournalDb> _journal;

    // both of these are parented/owned by the engine but are "rebuilt" on every sync
    // todo: investigate to determine whether we can improve this pointer handling but simply clearing/resetting the
    // states between runs (similar to ProgressInfo::reset, which contrary to the naming, is not related to smartpointers)
    DiscoveryPhase *_discoveryPhase = nullptr;
    OwncloudPropagator *_propagator = nullptr;

    // these pointers are all parented/owned by the engine
    ProgressInfo *_progressInfo = nullptr;
    ExcludedFiles *_excludedFiles = nullptr;
    SyncFileStatusTracker *_syncFileStatusTracker = nullptr;

    // List of all files with conflicts
    QSet<QString> _seenConflictFiles;
    Utility::ChronoElapsedTimer _duration;

    /**
     * Instead of downloading files from the server, upload the files to the server
     */
    void restoreOldFiles(SyncFileItemSet &syncItems);

    // If ignored files should be ignored
    bool _ignore_hidden_files = false;
    // should deleted files be moved to trash
    bool _moveToTrash = false;

    SyncOptions _syncOptions;

    bool _anotherSyncNeeded = false;

    QElapsedTimer _lastUpdateProgressCallbackCall;

    /** List of unique errors that occurred in a sync run. */
    QSet<QString> _uniqueErrors;

    /** The kind of local discovery the last sync run used */
    LocalDiscoveryStyle _lastLocalDiscoveryStyle = LocalDiscoveryStyle::FilesystemOnly;
    LocalDiscoveryStyle _localDiscoveryStyle = LocalDiscoveryStyle::FilesystemOnly;

    // must be ordered
    std::set<QString> _localDiscoveryPaths;

    // destructor called
    bool _goingDown = false;
};
}

static const int syncFileItemTypeId = qRegisterMetaType<OCC::SyncFileItem>("SyncFileItem");
static const int syncFileItemPtrTypeId = qRegisterMetaType<OCC::SyncFileItemPtr>("SyncFileItemPtr");
static const int syncFileItemStatusTypeId = qRegisterMetaType<OCC::SyncFileItem::Status>("SyncFileItem::Status");
static const int syncFileStatusTypeId = qRegisterMetaType<OCC::SyncFileStatus>("SyncFileStatus");
static const int syncFileItemSetTypeId = qRegisterMetaType<OCC::SyncFileItemSet>("SyncFileItemSet");
static const int syncFileItemDirectionTypeId = qRegisterMetaType<OCC::SyncFileItem::Direction>("SyncFileItem::Direction");
