/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
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

#include "gui/owncloudguilib.h"

#include "accountstate.h"
#include "common/syncjournaldb.h"
#include "libsync/graphapi/space.h"
#include "networkjobs.h"
#include "progressdispatcher.h"
#include "syncoptions.h"
#include "syncresult.h"

#include <QDateTime>
#include <QObject>
#include <QUuid>
#include <QtQml/QtQml>

#include <chrono>
#include <memory>

class QThread;
class QSettings;

namespace OCC {

class Vfs;
class SyncEngine;
class SyncRunFileLog;
class FolderWatcher;
class LocalDiscoveryTracker;

/**
 * @brief The FolderDefinition class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT FolderDefinition
{
public:
    FolderDefinition(const QByteArray &id, const QUrl &davUrl, const QString &spaceId, const QString &displayName);

    // Lisa todo: just make this a normal public ctr. there is no reason to have this static create method, but it's used in several places so keeping it for
    // now
    static auto createNewFolderDefinition(const QUrl &davUrl, const QString &spaceId, const QString &displayName = {})
    {
        return FolderDefinition(QUuid::createUuid().toByteArray(QUuid::WithoutBraces), davUrl, spaceId, displayName);
    }


    /// Saves the folder definition into the given settings (group should be preconfigured)
    static void save(QSettings &settings, const FolderDefinition &folder);

    /// Reads a folder definition from the current settings group.
    static FolderDefinition load(QSettings &settings, const QByteArray &id);

    /// path to the journal, usually relative to localPath
    QString journalPath() const { return _journalPath; }

    void setJournalPath(const QString &newPath)
    {
        if (_journalPath != newPath)
            _journalPath = newPath;
    }

    /// whether the folder is paused
    bool paused() const { return _paused; }
    void setPaused(bool pause) { _paused = pause; }

    /// whether the folder syncs hidden files
    bool ignoreHiddenFiles() const { return _ignoreHiddenFiles; }
    void setIgnoreHiddenFiles(bool ignore) { _ignoreHiddenFiles = ignore; }

    /// Which virtual files setting the folder uses
    Vfs::Mode virtualFilesMode() const { return _virtualFilesMode; }
    void setVirtualFilesMode(Vfs::Mode mode) { _virtualFilesMode = mode; }

    /// Whether the vfs mode shall silently be updated if possible
    bool upgradeVfsMode() const { return _upgradeVfsMode; }
    void setUpgradeVfsMode(bool upgradeVfs) { _upgradeVfsMode = upgradeVfs; }


    /// Ensure / as separator and trailing /.
    void setLocalPath(const QString &path);
    QString localPath() const { return _localPath; }


    /// Remove ending /, then ensure starting '/': so "/foo/bar" and "/".
    void setTargetPath(const QString &path);
    QString targetPath() const { return _targetPath; }

    /// journalPath relative to localPath.
    QString absoluteJournalPath() const { return QDir(localPath()).filePath(_journalPath); }


    QUrl webDavUrl() const { return _webDavUrl; }
    // could change in the case of spaces
    void setWebDavUrl(const QUrl &url) { _webDavUrl = url; }


    // when using spaces we don't store the dav URL but the space id
    // this id is then used to look up the dav URL
    QString spaceId() const { return _spaceId; }
    void setSpaceId(const QString &spaceId) { _spaceId = spaceId; }

    const QByteArray &id() const { return _id; }

    QString displayName() const;

    /**
     * The folder is deployed by an admin
     * We will hide the remove option and the disable/enable vfs option.
     */
    bool isDeployed() const { return _deployed; }

    /**
     * Higher values mean more imortant
     * Used for sorting
     */
    uint32_t priority() const;
    void setPriority(uint32_t newPriority);

private:
    /// path to the journal, usually relative to localPath
    QString _journalPath;

    /// whether the folder is paused
    bool _paused = false;
    /// whether the folder syncs hidden files
    bool _ignoreHiddenFiles = true;
    /// Which virtual files setting the folder uses
    Vfs::Mode _virtualFilesMode = Vfs::Off;

    /// Whether the vfs mode shall silently be updated if possible
    bool _upgradeVfsMode = false;

    // oc10 and as cache for ocis
    QUrl _webDavUrl;

    QString _spaceId;
    /// For legacy reasons this can be a string, new folder objects will use a uuid
    QByteArray _id;
    QString _displayName;
    /// path on local machine (always trailing /)
    QString _localPath;
    /// path on remote (usually no trailing /, exception "/")
    QString _targetPath;
    bool _deployed = false;

    uint32_t _priority = 0;

};

/**
 * @brief The Folder class
 * @ingroup gui
 */
// Refactoring todo: this class is overgrown and has too many responsibilities. Need to evaluate and refine it's core behaviors
// and split out some of the responsibilities to new dedicated classes. Eg I already see some need for a folder sync handler, and/or
// a folder scoped vfs controller. these could still be created/configured in the Folder (worst case!) but this class just does way too much
// Refactoring the impls to be more modular will also allow us to refactor the ctr sensibly to avoid the excessive setup routines which are
// invoked before the class is even fully built.
// we also have weird split responsibility around instantiating the vfs - on creation the FolderMan instantiates vfs and passes it to
// the ctr. But the folder resets the vfs with new instance sometimes as well. This is not healthy as responsibility is split/not clear
class OWNCLOUDGUI_EXPORT Folder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GraphApi::Space *space READ space NOTIFY spaceChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Folders can only be created by the FolderManager")

public:
    enum class ChangeReason {
        Other,
        UnLock
    };
    Q_ENUM(ChangeReason)

    static void prepareFolder(const QString &path);

    /** Create a new Folder
     */
    Folder(const FolderDefinition &definition, const AccountStatePtr &accountState, std::unique_ptr<Vfs> &&vfs, QObject *parent = nullptr);

    ~Folder() override;
    /**
     * The account the folder is configured on.
     */
    AccountStatePtr accountState() const { return _accountState; }

    const FolderDefinition &definition() const { return _definition; }

    QByteArray id() const { return _definition.id(); }

    /**
     * Ignore syncing of hidden files or not. This is defined in the
     * folder definition
     *
     * Lisa todo: refactor this as
     * a) it's actually a global setting and
     * b) the value only matters when it is passed to the engine
     * it doesn't really need to be in the definition at all.
     */
    bool ignoreHiddenFiles() const { return _definition.ignoreHiddenFiles(); }

    void setIgnoreHiddenFiles(bool ignore) { _definition.setIgnoreHiddenFiles(ignore); }

    /**
     * remote folder path, usually without trailing /, exception "/"
     */
    QString remotePath() const { return _definition.targetPath(); }

    // may come from the definition, may come from the space. Lisa todo: review this as it appears questionable
    QString displayName() const;

    // may come from definition, may come from space - Lisa todo: check this out
    QUrl webDavUrl() const;

    /**
     * short local path to display on the GUI  (native separators)
     */
    QString shortGuiLocalPath() const;

    /**
     * canonical local folder path, always ends with /
     */
    QString path() const { return _canonicalLocalPath; }

    /**
     * cleaned canonical folder path, like path() but never ends with a /
     *
     * Wrapper for QDir::cleanPath(path()) except for "Z:/",
     * where it returns "Z:" instead of "Z:/".
     */
    QString cleanPath() const;


    /**
     * remote folder path, always with a trailing /
     */
    QString remotePathTrailingSlash() const;

    /**
     * remote folder path with server URL
     */
    QUrl remoteUrl() const { return Utility::concatUrlPath(webDavUrl(), remotePath()); }

    /**
     * switch sync on or off
     */
    void setSyncPaused(bool);

    bool syncPaused() const { return _definition.paused(); }

    /**
     * Returns true when the folder may sync.
     */
    bool canSync() const;

    /**
     * Whether the folder is ready
     *
     * passthrough to _engine
     */
    bool isReady() const;

    bool hasSetupError() const
    {
        return _syncResult.status() == SyncResult::SetupError;
    }

    void prepareToSync() { setSyncState(SyncResult::NotYetStarted); }

    /** True if the folder is currently synchronizing */
    bool isSyncRunning() const;

    /**
     * return the last sync result with error message and status
     */
    SyncResult syncResult() const { return _syncResult; }

    /**
      * This is called when the sync folder definition is removed. Do cleanups here.
      *
      * It removes the database, among other things.
      *
      * The folder is not in a valid state afterwards!
      */
    virtual void wipeForRemoval();

    void setSyncState(SyncResult::Status state);

    void setDirtyNetworkLimits();

    void reloadSyncOptions();


    // TODO: don't expose
    SyncJournalDb *journalDb()
    {
        return &_journal;
    }
    // TODO: don't expose
    SyncEngine &syncEngine()
    {
        return *_engine;
    }

    Vfs &vfs()
    {
        OC_ENFORCE(_vfs);
        return *_vfs;
    }

    auto lastSyncTime() const { return QDateTime::currentDateTime().addMSecs(-msecSinceLastSync().count()); }
    std::chrono::milliseconds msecSinceLastSync() const { return std::chrono::milliseconds(_timeSinceLastSyncDone.elapsed()); }
    std::chrono::milliseconds msecLastSyncDuration() const { return _lastSyncDuration; }

    /**
      * Returns whether a file inside this folder should be excluded.
      */
    bool isFileExcludedAbsolute(const QString &fullPath) const;

    /**
      * Returns whether a file inside this folder should be excluded.
      */
    bool isFileExcludedRelative(const QString &relativePath) const;

    /** virtual files of some kind are enabled
     *
     * This is independent of whether new files will be virtual. It's possible to have this enabled
     * and never have an automatic virtual file. But when it's on, the shell context menu will allow
     * users to make existing files virtual.
     */
    bool virtualFilesEnabled() const;
    void setVirtualFilesEnabled(bool enabled);

    /** Whether this folder should show selective sync ui */
    bool supportsSelectiveSync() const;

    /**
     * Whether to register the parent folder of our sync root in the explorer
     * The default behaviour is to register alls spaces in a common dir in the home folder
     * in that case we only display that common dir in the Windows side bar.
     * With the legacy behaviour we only have one dir which we will register with Windows
     */
    bool groupInSidebar() const;

    /**
     * The folder is deployed by an admin
     * We will hide the remove option and the disable/enable vfs option.
     */
    bool isDeployed() const;

    uint32_t priority() const { return _definition.priority(); }


    void setPriority(uint32_t p) { _definition.setPriority(p); }

    static Result<void, QString> checkPathLength(const QString &path);

    /**
     *
     * @return The corresponding space object or null
     */
    GraphApi::Space *space() const;

Q_SIGNALS:
    void syncStateChange();
    void syncStarted();
    void syncFinished(const SyncResult &result);
    void syncPausedChanged(Folder *, bool paused);
    void canSyncChanged();
    void spaceChanged();
    void vfsModeChanged(Folder *f, Vfs::Mode newMode);


    /**
     * Fires for each change inside this folder that wasn't caused
     * by sync activity.
     */
    void watchedFileChangedExternally(const QString &path);

public Q_SLOTS:
    /**
       * terminate the current sync run
       */
    void slotTerminateSync(const QString &reason);

    /**
      * Starts a sync operation
      *
      * If the list of changed files is known, it is passed.
      */
    void startSync();

    void slotDiscardDownloadProgress();
    int slotWipeErrorBlacklist();

    /**
       * Triggered by the folder watcher when a file/dir in this folder
       * changes. Needs to check whether this change should trigger a new
       * sync run to be scheduled.
       */
    void slotWatchedPathsChanged(const QSet<QString> &paths, ChangeReason reason);

    /**
     * Mark a virtual file as being requested for download, and start a sync.
     *
     * "implicit" here means that this download request comes from the user wanting
     * to access the file's data. The user did not change the file's pin state.
     * If the file is currently OnlineOnly its state will change to Unspecified.
     *
     * The download request is stored by setting ItemTypeVirtualFileDownload
     * in the database. This is necessary since the hydration is not driven by
     * the pin state.
     *
     * relativepath is the folder-relative path to the file (including the extension)
     *
     * Note, passing directories is not supported. Files only.
     */
    void implicitlyHydrateFile(const QString &relativepath);

    /** Ensures that the next sync performs a full local discovery. */
    void slotNextSyncFullLocalDiscovery();

    /** Adds the path to the local discovery list
     *
     * A weaker version of slotNextSyncFullLocalDiscovery() that just
     * schedules all parent and child items of the path for local
     * discovery.
     */
    void schedulePathForLocalDiscovery(const QString &relativePath);

    /// Reloads the excludes, used when changing the user-defined excludes after saving them to disk.
    bool reloadExcludes();

private Q_SLOTS:
    void slotSyncStarted();
    void slotSyncFinished(bool);

    /** Adds a error message that's not tied to a specific item.
     */
    void slotSyncError(const QString &message, ErrorCategory category = ErrorCategory::Normal);

    void slotItemCompleted(const SyncFileItemPtr &);

    void slotLogPropagationStart();

    /** Adjust sync result based on conflict data from IssuesWidget.
     *
     * This is pretty awkward, but IssuesWidget just keeps better track
     * of conflicts across partial local discovery.
     */
    void slotFolderConflicts(Folder *folder, const QStringList &conflictPaths);

    /** Warn users if they create a file or folder that is selective-sync excluded */
    void warnOnNewExcludedItem(const SyncJournalFileRecord &record, QStringView path);

    /** Warn users about an unreliable folder watcher */
    void slotWatcherUnreliable(const QString &message);

private:
    void showSyncResultPopup();

    bool checkLocalPath();

    SyncOptions loadSyncOptions();

    /**
     * Sets up this folder's folderWatcher if possible.
     *
     * May be called several times.
     */
    // Refactoring todo: I would expect a "registration" function to take an arg for the thing that is supposed to be registered
    // I also do not see evidence that this is called "several times" aside from the fact that it's called any time the
    //
    void registerFolderWatcher();

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
        const QString &renameTarget = QString());

    void startVfs();

    void changeVfsMode(Vfs::Mode newMode);

    AccountStatePtr _accountState;
    FolderDefinition _definition;
    QString _canonicalLocalPath; // As returned with QFileInfo:canonicalFilePath.  Always ends with "/"

    SyncResult _syncResult;
    QScopedPointer<SyncEngine> _engine;
    QElapsedTimer _timeSinceLastSyncDone;
    QElapsedTimer _timeSinceLastSyncStart;
    QElapsedTimer _timeSinceLastFullLocalDiscovery;
    std::chrono::milliseconds _lastSyncDuration = {};

    /// The number of syncs that failed in a row.
    /// Reset when a sync is successful.
    int _consecutiveFailingSyncs = 0;

    /// The number of requested follow-up syncs.
    /// Reset when no follow-up is requested.
    int _consecutiveFollowUpSyncs = 0;

    mutable SyncJournalDb _journal;

    QScopedPointer<SyncRunFileLog> _fileLog;

    QTimer _scheduleSelfTimer;

    /**
     * Setting up vfs is a async operation
     */
    bool _vfsIsReady = false;

    /**
     * Watches this folder's local directory for changes.
     *
     * Created by registerFolderWatcher(), triggers slotWatchedPathsChanged()
     */
    // Refactor todo: I think this should just be a normal pointer since it's not passed around and is truly a child of the
    // parent Folder instance, so will be destructed along with the folder. Why are we making this so complicated?
    // Further, I'm not even sure it's safe to use a scoped pointer here since the whole purpose is to auto delete the pointer
    // when it goes "out of scope" - eg when this parent folder is destructed. If we have some real reason to keep it,
    // I think at the very least we need to be sure to use the QScopedPointerDeleteLater custom deleter as described in the docs:
    // QScopedPointer<MyCustomClass, ScopedPointerCustomDeleter> customPointer(new MyCustomClass);
    QScopedPointer<FolderWatcher> _folderWatcher;

    /**
     * Keeps track of locally dirty files so we can skip local discovery sometimes.
     */
    // Refactoring todo: as above, except we need to add the possibility to pass a parent to this class's ctr to allow auto-cleanup
    // again: this should NOT be complicated
    QScopedPointer<LocalDiscoveryTracker> _localDiscoveryTracker;

    /**
     * The vfs mode instance (created by plugin) to use. Never null.
     */
    // Refactoring todo: this is shared with the SyncOptions that are passed to the engine. This needs reevaluation and cleanup
    // to ensure we don't keep it alive outside of useable scope. Would probably be simplest to make it a QPointer for use with the
    // SyncOptions and rely on normal qt parenting scheme to ensure correct cleanup timing if it's not explicitly deleted.
    // it is also false that it is never null - it is reset in wipeForRemoval
    // extra fun is I have no idea what happens to the instance in the SyncOptions - is it still alive relative to the engine?
    // I don't see any handling of the engine or SyncOptions whatsoever in wipeForRemoval so we'll need to go spelunking.
    QSharedPointer<Vfs> _vfs;

    friend class SpaceMigration;
};
}
