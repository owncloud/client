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

#ifndef ABSTRACTFOLDER_H
#define ABSTRACTFOLDER_H

#include "syncresult.h"
#include "progressdispatcher.h"
#include "syncjournaldb.h"
#include "clientproxy.h"
#include "networkjobs.h"
#include "folderdefinition.h"


#include <QObject>
#include <QStringList>

class QThread;
class QSettings;

namespace OCC {

class SyncEngine;
class AccountState;
class SyncRunFileLog;

/**
 * @brief The Folder class
 * @ingroup gui
 */
class AbstractFolder : public QObject
{
    Q_OBJECT

public:
    AbstractFolder(const FolderDefinition &definition, AccountState *accountState, QObject *parent = 0L);

    ~AbstractFolder();

    typedef QMap<QString, AbstractFolder *> Map;
    typedef QMapIterator<QString, AbstractFolder *> MapIterator;

    /**
     * The account the folder is configured on.
     */
    AccountState *accountState() const { return _accountState.data(); }

    /**
     * alias or nickname
     */
    QString alias() const;
    QString shortGuiRemotePathOrAppName() const; // since 2.0 we don't want to show aliases anymore, show the path instead

    /**
     * short local path to display on the GUI  (native separators)
     */
    QString shortGuiLocalPath() const;

    /**
     * canonical local folder path, always ends with /
     */
    QString path() const;

    /**
     * cleaned canonical folder path, like path() but never ends with a /
     *
     * Wrapper for QDir::cleanPath(path()) except for "Z:/",
     * where it returns "Z:" instead of "Z:/".
     */
    QString cleanPath() const;

    /**
     * remote folder path
     */
    QString remotePath() const;

    /**
     * remote folder path with server url
     */
    QUrl remoteUrl() const;

    /**
     * switch sync on or off
     */
    void setSyncPaused(bool);

    bool syncPaused() const;

    /**
     * Returns true when the folder may sync.
     */
    bool canSync() const;

    void prepareToSync();

    /**
     * True if the folder is busy and can't initiate
     * a synchronization
     */
    virtual bool isBusy() const;

    /**
     * return the last sync result with error message and status
     */
    SyncResult syncResult() const;

    /**
      * set the config file name.
      */
    // FIXME: Can probably be removed!
    void setConfigFile(const QString &);
    QString configFile();

    /**
      * This is called if the sync folder definition is removed. Do cleanups here.
      */
    virtual void wipe();

    void setSyncState(SyncResult::Status state);

    void setDirtyNetworkLimits();

    // Used by the Socket API
    SyncJournalDb *journalDb() { return &_journal; }
    SyncEngine &syncEngine() { return *_engine; }

    qint64 msecSinceLastSync() const { return _timeSinceLastSyncDone.elapsed(); }
    qint64 msecLastSyncDuration() const { return _lastSyncDuration; }
    int consecutiveFollowUpSyncs() const { return _consecutiveFollowUpSyncs; }
    int consecutiveFailingSyncs() const { return _consecutiveFailingSyncs; }

    /// Saves the folder data in the account's settings.
    void saveToSettings() const;
    /// Removes the folder from the account's settings.
    void removeFromSettings() const;

    /** Calls schedules this folder on the FolderMan after a short delay.
      *
      * This should be used in situations where a sync should be triggered
      * because a local file was modified. Syncs don't upload files that were
      * modified too recently, and this delay ensures the modification is
      * far enough in the past.
      *
      * The delay doesn't reset with subsequent calls.
      */
    void scheduleThisFolderSoon();

    /**
      * Migration: When this flag is true, this folder will save to
      * the backwards-compatible 'Folders' section in the config file.
      */
    void setSaveBackwardsCompatible(bool save);

    /**
     * @brief etagJob
     * If the folder uses an ETag job to look for changes, it is returned by
     * this method. Reimplemented in the normal folder.
     */
    virtual RequestEtagJob *etagJob() { return nullptr; }

    /**
      * Are hidden files ignored in the synchronization by default?
      *
      * This should be a per-folder setting, but currently it is not in the client.
      */
    virtual bool ignoreHiddenFiles() { return false; }
    virtual void setIgnoreHiddenFiles(bool ignore) { Q_UNUSED(ignore) } // default does nothing, FIXME

    // Exclude handling, FIXME: Think again!
    virtual bool isFileExcludedRelative( const QString& ) const { return false; } // nothing excluded by default.
    virtual bool isFileExcludedAbsolute( const QString& ) const { return false; } // nothing excluded by default.

signals:
    void syncStateChange();
    void syncStarted();
    void syncFinished(const SyncResult &result);
    void progressInfo(const ProgressInfo &progress);
    void syncPausedChanged(AbstractFolder *, bool paused);
    void canSyncChanged();

    /**
     * Fires for each change inside this folder that wasn't caused
     * by sync activity.
     */
    void watchedFileChangedExternally(const QString &path);

public slots:

    /**
       * terminate the current sync run
       */
    void slotTerminateSync();

    /**
      * Starts a sync operation
      *
      * If the list of changed files is known, it is passed.
      */
    void startSync(const QStringList &pathList = QStringList());

    void setProxyDirty(bool value);
    bool proxyDirty();

    int slotDiscardDownloadProgress();
    int downloadInfoCount();
    int slotWipeErrorBlacklist();
    int errorBlackListEntryCount();

    /**
       * Triggered by the folder watcher when a file/dir in this folder
       * changes. Needs to check whether this change should trigger a new
       * sync run to be scheduled.
       */
    void slotWatchedPathChanged(const QString &path);

private slots:
    void slotSyncStarted();
    void slotSyncError(const QString &);
    void slotSyncFinished(bool);

    void slotFolderDiscovered(bool local, QString folderName);
    void slotTransmissionProgress(const ProgressInfo &pi);
    void slotItemCompleted(const SyncFileItemPtr &);

    void slotEmitFinishedDelayed();

    void slotLogPropagationStart();

    /** Adds this folder to the list of scheduled folders in the
     *  FolderMan.
     */
protected slots:
    void slotScheduleThisFolder();

protected:
    // FIXME: This should be empty
    AccountStatePtr _accountState;
    FolderDefinition _definition;
    QScopedPointer<SyncEngine> _engine;

private:
    void showSyncResultPopup();

    void checkLocalPath();

    void setSyncOptions();

    enum LogStatus {
        LogStatusRemove,
        LogStatusRename,
        LogStatusMove,
        LogStatusNew,
        LogStatusError,
        LogStatusConflict,
        LogStatusUpdated
    };

    void createGuiLog(const QString &filename, LogStatus status, int count,
        const QString &renameTarget = QString::null);


    QString _canonicalLocalPath; // As returned with QFileInfo:canonicalFilePath.  Always ends with "/"

    SyncResult _syncResult;
    bool _proxyDirty;
    QElapsedTimer _timeSinceLastSyncDone;
    QElapsedTimer _timeSinceLastSyncStart;
    qint64 _lastSyncDuration;

    /// The number of syncs that failed in a row.
    /// Reset when a sync is successful.
    int _consecutiveFailingSyncs;

    /// The number of requested follow-up syncs.
    /// Reset when no follow-up is requested.
    int _consecutiveFollowUpSyncs;

    SyncJournalDb _journal;

    ClientProxy _clientProxy;

    QScopedPointer<SyncRunFileLog> _fileLog;

    QTimer _scheduleSelfTimer;

    /**
     * When the same local path is synced to multiple accounts, only one
     * of them can be stored in the settings in a way that's compatible
     * with old clients that don't support it. This flag marks folders
     * that shall be written in a backwards-compatible way, by being set
     * on the *first* Folder instance that was configured for each local
     * path.
     */
    bool _saveBackwardsCompatible;
};
}

#endif
