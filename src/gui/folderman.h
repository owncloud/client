/*
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

#include "folder.h"
#include "scheduling/syncscheduler.h"

#include <QList>
#include <QObject>
#include <QQueue>

class TestFolderMigration;

namespace OCC {

class FolderMan;
namespace TestUtils {
    // prototype for test friend
    FolderMan *folderMan();
}

class Application;
class SyncResult;
class SocketApi;
class LockWatcher;

/**
 * @brief Return object for Folder::trayOverallStatus.
 * @ingroup gui
 */
class TrayOverallStatusResult
{
public:
    QDateTime lastSyncDone;

    void addResult(Folder *f);
    const SyncResult &overallStatus() const;

private:
    SyncResult _overallStatus;
};

/**
 * @brief The FolderMan class
 * @ingroup gui
 *
 * The FolderMan knows about all loaded folders and is responsible for
 * scheduling them when necessary.
 *
 * A folder is scheduled if:
 * - The configured force-sync-interval has expired
 *   (_timeScheduler and slotScheduleFolderByTime())
 *
 * - A folder watcher receives a notification about a file change
 *   (_folderWatchers and Folder::slotWatchedPathsChanged())
 *
 * - The folder etag on the server has changed
 *   (_etagPollTimer)
 *
 * - The locks of a monitored file are released
 *   (_lockWatcher and slotWatchedFileUnlocked())
 *
 * - There was a sync error or a follow-up sync is requested
 *   (_timeScheduler and slotScheduleFolderByTime()
 *    and Folder::slotSyncFinished())
 */
class OWNCLOUDGUI_EXPORT FolderMan : public QObject
{
    Q_OBJECT
public:
    /**
     * For a new folder, the type guides what kind of checks are done to ensure the new folder is not embedded in an existing one.
     * Or in case of a space folder, that if the new folder is in a Space sync root, it is the sync root of the same account.
     */
    enum class NewFolderType {
        OC10SyncRoot,
        SpacesSyncRoot,
        SpacesFolder,
    };

    // Refactoring todo: why does this duplicate most of the folder definition?
    struct SyncConnectionDescription
    {
        /***
-         * The WebDAV URL for the sync connection.
         */
        QUrl davUrl;

        /***
         * The id of the space or empty in case of ownCloud 10.
         */
        QString spaceId;

        /***
         * The local folder used for the sync.
         */
        QString localPath;

        /***
         * The relative remote path
         */
        QString remotePath;

        /***
         * The Space name to display in the list of folders or an empty string.
         */
        QString displayName;

        /***
         * Wether to use virtual files.
         */
        bool useVirtualFiles;

        uint32_t priority;

        QSet<QString> selectiveSyncBlackList;
    };

    static QString suggestSyncFolder(NewFolderType folderType, const QUuid &accountUuid);
    [[nodiscard]] static bool prepareFolder(const QString &folder);

    static QString checkPathValidityRecursive(const QString &path, FolderMan::NewFolderType folderType, const QUuid &accountUuid);

    static std::unique_ptr<FolderMan> createInstance();
    ~FolderMan() override;

    /**
     * Helper to access the FolderMan instance
     * Warning: may be null in unit tests
     */
    // TODO: eliminate FolderMan as globally available resource.
    static FolderMan *instance();

    /**
     *  loads all folders from the config into the manager.
     *
     *  returns empty if a downgrade of a folder was detected
     *  otherwise it will return the number of folders that were set up (this can be zero when no folders were previously configured).
     */
    std::optional<qsizetype> setupFoldersFromConfig();

    /**
     *  core step in any add folder routine. it validates the definition, instantiates vfs, instantiates the folder and validates whether
     *  it had setup errors.
     *
     *  it is up to the caller to connect the folder, save it to settings, etc.
     *
     *  Refactoring todo: this should not be public! it is currently "required" for some tests which is not really cool, as it does not represent
     *  a complete/standalone impl.
     */
    Folder *addFolder(const AccountStatePtr &accountState, const FolderDefinition &folderDefinition);


    /**
     *  sets up sync folders/spaces after adding a new account via the gui
     */
    void setUpInitialSyncFolders(AccountStatePtr accountStatePtr, bool useVfs);

    // Refactoring todo: this function actually just returns the internal vector of folders. I do not see any evidence of
    // what is docced here, at least related to this specific function.
    // Propose changing the container to a hash or similar anyway to allow fast retrieval of folder by id and other
    // maintenance activities.
    /** Find folder setting keys that need to be ignored or deleted for being too new.
     *
     * The client has a maximum supported version for the folders lists (maxFoldersVersion
     * in folderman.cpp) and a second maximum version for the contained folder configuration
     * (FolderDefinition::maxSettingsVersion()). If a future client creates configurations
     * with higher versions the older client will not be able to process them.
     *
     * Skipping or deleting these keys prevents accidents when switching from a newer
     * client to an older one.
     *
     * This function scans through the settings and finds too-new entries that can be
     * ignored (ignoreKeys) and entries that have to be deleted to keep going (deleteKeys).
     *
     * This data is used in Application::configVersionMigration() to backward-migrate
     * future configurations (possibly with user confirmation for deletions) and in
     * FolderMan::setupFolders() to know which too-new folder configurations to skip.
     */
    const QVector<Folder *> &folders() const;


    /**
     *  Removes a folder sync permanently in response to user request
     *  Not for general folder cleanup
     *  it is caller's responsibility to remove folder from settings if necessary.
     */
    // Refactoring todo: this is called directly from the AccountSettings gui - instead the gui should signal a request to
    // remove the folder.
    // also, this function *is* used for general folder cleanup when an account is removed.
    // we must develop concise, SINGLE impls for eg adding or removing folders instead of spreading the handling over multiple
    // locations.
    void removeFolderSync(Folder *);

    /**
     * Returns the folder which the file or directory stored in path is in
     *
     * Optionally, the path relative to the found folder is returned in
     * relativePath.
     */
    Folder *folderForPath(const QString &path, QString *relativePath = nullptr);

    /**
      * returns a list of local files that exist on the local harddisk for an
      * incoming relative server path. The method checks with all existing sync
      * folders.
      */
    QStringList findFileInLocalFolders(const QString &relPath, const AccountPtr acc);

    /** Returns the folder by id or NULL if no folder with the id exists. */
    [[deprecated("directly reference the folder")]] Folder *folder(const QByteArray &id);

    /**
     * Ensures that a given directory does not contain a sync journal file.
     *
     * @returns false if the journal could not be removed, true otherwise.
     */
    static bool ensureJournalGone(const QString &journalDbFile);
    static bool ensureFilesystemSupported(const FolderDefinition &folderDefinition);

    /// Produce text for use in the tray tooltip
    static QString trayTooltipStatusString(const SyncResult &result, bool paused);

    /**
     * Compute status summarizing multiple folders
     * @return tuple containing folders, status, unresolvedConflicts and lastSyncDone
     */
    static TrayOverallStatusResult trayOverallStatus(const QVector<Folder *> &folders);

    SocketApi *socketApi();

    /**
     * Check if @a path is a valid path for a new folder considering the already sync'ed items.
     * Make sure that this folder, or any subfolder is not sync'ed already.
     *
     * @returns an empty string if it is allowed, or an error if it is not allowed
     */
    QString checkPathValidityForNewFolder(const QString &path, NewFolderType folderType, const QUuid &accountUuid) const;

    /**
     * Attempts to find a non-existing, acceptable path for creating a new sync folder.
     *
     * Uses \a basePath as the baseline. It'll return this path if it's acceptable.
     *
     * Note that this can fail. If someone syncs ~ and \a basePath is ~/ownCloud, no
     * subfolder of ~ would be a good candidate. When that happens \a basePath
     * is returned.
     */
    static QString findGoodPathForNewSyncFolder(const QString &basePath, const QString &newFolder, NewFolderType folderType, const QUuid &accountUuid);

    /**
     * While ignoring hidden files can theoretically be switched per folder,
     * it's currently a global setting that users can only change for all folders
     * at once.
     * These helper functions can be removed once it's properly per-folder.
     */
    // Refactoring todo: when is the ability to switch ignoreHiddenFiles per folder going to happen? what does it
    // depend on? if there is no change in sight for this, we should refactor the prop to be a general setting
    // that applies to all folders, and is persisted in the General settings group of the config. The churn around
    // moving this setting to the folder defs is absurd if it's not needed (and currently it's not actually needed)
    bool ignoreHiddenFiles() const;
    void setIgnoreHiddenFiles(bool ignore);

    /**
     * Returns true if any folder is currently syncing.
     *
     * This might be a FolderMan-scheduled sync, or a externally
     * managed sync like a placeholder hydration.
     */
    bool isAnySyncRunning() const;

    /** Simple save and remove all folders on shut down */
    void unloadAndDeleteAllFolders();

    /**
     * If enabled is set to false, no new folders will start to sync.
     * The current one will finish.
     */
    void setSyncEnabled(bool);

    SyncScheduler *scheduler() { return _scheduler; }


    void setDirtyProxy();
    void setDirtyNetworkLimits();

    /** Whether or not vfs is supported in the location. */
    bool checkVfsAvailability(const QString &path, Vfs::Mode mode = VfsPluginManager::instance().bestAvailableVfsMode()) const;

    /** If the folder configuration is no longer supported this will return an error string */
    Result<void, QString> unsupportedConfiguration(const QString &path) const;

    [[nodiscard]] bool isSpaceSynced(GraphApi::Space *space) const;

    /**
     * adds a folder from a gui operation. this is a "presetup" step that converts the SycnConnectionDescription to a FolderDefinition
     * and does a few other things uniquely required by the gui workflow.
     *
     * this handler also saves the new folder def that is created by user request
     */
    // Refactoring todo: try to replace the SyncConnectionDescription with FolderDefinition if reasonable
    // also replace hard calls to this from the gui with signal to request to add the new folder and use this as a slot, roughly.
    void addFolderFromGui(const AccountStatePtr &accountStatePtr, const SyncConnectionDescription &config);

    // Refactoring todo: this is temporarily public - it should eventually be private once we have refactored accountSettings remove
    // folder sync to be a signal/request instead of a local impl which calls directly into FolderMan
    // In fact all of the folder persistence stuff should be handled by a third party persistence component that listens for relevant folder changes.
    // That is a long way off, unfortunately, but at least we can collect all of that
    // behavior here in the FolderMan to ensure it's done as efficiently as possible
    void removeFolderSettings(Folder *folder);

Q_SIGNALS:
    /**
      * signal to indicate a folder has changed its sync state.
      *
      * Attention: The folder may be zero. Do a general update of the state then.
      */
    void folderSyncStateChange(Folder *);

    /**
     * Emitted whenever the list of configured folders changes.
     */
    void folderListChanged();
    void folderRemoved(Folder *folder);
    // Refactoring todo: we need folderAdded too. The folder model should use that for normal folder updates instead of folderListChanged
    // which causes full rebuild of the model -> crazy inefficient. Ideally folderListChanged should only be emitted for large operations (eg after loading
    // folders from config or from new account)

public Q_SLOTS:

    /**
     * Schedules folders of newly connected accounts, terminates and
     * de-schedules folders of disconnected accounts.
     */
    void slotIsConnectedChanged();

    /**
     * Triggers a sync run once the lock on the given file is removed.
     *
     * Automatically detemines the folder that's responsible for the file.
     * See slotWatchedFileUnlocked().
     */
    void slotSyncOnceFileUnlocks(const QString &path, FileSystem::LockMode mode);

    /// This slot will tell all sync engines to reload the sync options.
    void slotReloadSyncOptions();

private Q_SLOTS:
    void slotFolderSyncPauseChanged(Folder *, bool paused);
    void slotFolderCanSyncChanged();
    void slotFolderSyncStarted();
    void slotFolderSyncFinished(const SyncResult &);

    void slotRemoveFoldersForAccount(const AccountStatePtr &accountState);

    void slotServerVersionChanged(Account *account);

    // saves folder using an internally created QSettings instance. This is used for "one off" persistence operations
    // that should be synced immediately - eg when the user adds a new folder sync from the gui or as slot to eg persist a property
    // change
    // see second version of saveFolder, below, for use when persisting many folders in a row
    void saveFolder(Folder *folder);


private:
    explicit FolderMan();

    /**
     * Adds a folder "from scratch" as oppossd to from config, which requires less setup than when you create the folder
     * from some dynamic operation (eg folders from new account or via the gui add folder sync operations).
     * In case Wizard::SyncMode::SelectiveSync is used, nullptr is returned.
     */
    // Lisa todo with Erik: what is this ref about returning null if SelectiveSync is used? I don't see that in the impl
    // is it buried somewhere?
    // also todo: discuss naming, I honestly could not come up with anything better, as this is exactly what it does
    Folder *addFolderFromScratch(const AccountStatePtr &accountStatePtr, FolderDefinition &&definition, bool useVfs);

    /**
     *  private handler connected to spacesManager::ready signal
     *  this is a bit weird as you have to ask the manager if it's ready then wait for the signal before actually loading
     *  the spaces. this function loads all the spaces into the FolderMan and saves them in an efficient manner
     */
    void loadSpacesWhenReady(AccountStatePtr accountState, bool useVfs);

    /**
     *  reads the folder defs from the config for a single account.
     *  it is important to use a preconfigured settings instance here instead of using a series of one-off instances, to improve efficiecy
     *  by reducing file operations when loading many folders.
     *
     *  The settings should be configured to group "Accounts" before it's passed to this function.
     *
     *  returns false when a downgrade of the database is detected, true otherwise.
     */
    bool addFoldersFromConfigByAccount(QSettings &settings, AccountStatePtr account);

    // tests folder def for minimum reqs
    bool validateFolderDefinition(const FolderDefinition &folderDefinition);

    /// \returns true of the FolderDefinition was migrated, false if it was unchanged
    bool migrateFolderDefinition(FolderDefinition &folderDef, AccountStatePtr account);

    /** Connects a folder instance, provided it has no setup errors
     */
    void connectFolder(Folder *folder);

    /* disconnects a folder instance, provided it has no setup errors
     */
    void disconnectFolder(Folder *folder);

    /* saves folder using the given instance of config settings already set to group "Accounts"
     * The main purpose of this version is to allow using a single settings instance to persist multiple folders
     * in a row before syncing -> we save a lot of expensive file operations this way
     */
    void saveFolder(Folder *folder, QSettings &settings);

    // used to reduce file operation overhead when removing multiple folders
    void removeFolderSettings(Folder *folder, QSettings &settings);

    /** Queues all folders for syncing. */
    void scheduleAllFolders();

    // finds all folder configuration files
    // and create the folders
    QString getBackupName(QString fullPathName) const;

    // makes the folder known to the socket api
    // pair this with _socketApi->slotUnregisterPath(folder);
    void registerFolderWithSocketApi(Folder *folder);

    QSet<Folder *> _disabledFolders;
    QVector<Folder *> _folders;
    QString _folderConfigPath;

    /// Folder aliases from the settings that weren't read
    QSet<QString> _additionalBlockedFolderAliases;

    /// Watches files that couldn't be synced due to locks
    QScopedPointer<LockWatcher> _lockWatcher;

    /// Scheduled folders that should be synced as soon as possible
    SyncScheduler *_scheduler;

    std::unique_ptr<SocketApi> _socketApi;

    mutable QMap<QString, Result<void, QString>> _unsupportedConfigurationError;

    static FolderMan *_instance;
    friend class OCC::Application;
    friend class ::TestFolderMigration;
};

} // namespace OCC
