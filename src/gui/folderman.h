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
        OC10SyncRoot, // todo: #43
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

        uint32_t sortPriority;

        QSet<QString> selectiveSyncBlackList;
    };

    static QString suggestSyncFolder(NewFolderType folderType, const QUuid &accountUuid);


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
     *
     *  emits folderListChanged
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
    Folder *addFolder(AccountState *accountState, const FolderDefinition &folderDefinition);


    /**
     *  sets up sync folders/spaces after adding a new account via the gui
     */
    void setUpInitialSyncFolders(AccountState *accountState, bool useVfs);

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
    QList<Folder *> folders() const;

    QList<Folder *> foldersForAccount(const QUuid &accountId);


    /**
     * Returns the folder which the file or directory stored in path is in
     *
     * Optionally, the path relative to the found folder is returned in
     * relativePath.
     */
    Folder *folderForPath(const QString &path, QString *relativePath = nullptr);

    /**
     * Ensures that a given directory does not contain a sync journal file.
     *
     * @returns false if the journal could not be removed, true otherwise.
     */
    static bool ensureJournalGone(const QString &journalDbFile);
    static bool ensureFilesystemSupported(const FolderDefinition &folderDefinition);

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

    bool ignoreHiddenFiles() const;
    void setIgnoreHiddenFiles(bool ignore);

    /** Simple save and remove all folders on shut down
     *
     *  emits folderListChanged
     *
     */
    void unloadAndDeleteAllFolders();

    /**
     * If enabled is set to false, no new folders will start to sync.
     * The current one will finish.
     */
    void setSyncEnabled(bool);

    SyncScheduler *scheduler() { return _scheduler; }

    void setDirtyProxy();

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
     *
     * emits folderAdded(newFolder)
     */
    // todo: #1
    // todo: #2
    void addFolderFromGui(AccountState *accountState, const SyncConnectionDescription &config);

    // todo: #3
    void removeFolderSettings(Folder *folder);

    /**
     * @brief folder retrieves the folder given the spaceId, or null if no folder exists for the account and space combo
     */
    Folder *folder(const QUuid &accountId, const QString &spaceId) const;

Q_SIGNALS:
    /**
      * signal to indicate a folder has changed its sync state.
      *
      * Attention: The folder may be zero. Do a general update of the state then.
      */
    void folderSyncStateChange(Folder *);

    /**
     * Emitted whenever the list of folders changes substantially.
     * this includes after building the list for a new account or restore from config
     * also when removing folders in bulk, on account removed or shutdown.
     */
    void folderListChanged(const QUuid &accountId, const QList<Folder *> folders);
    // emitted on incremental folder additions (eg when the user uses the folder wizard to create a new sync)
    void folderAdded(const QUuid &accountId, Folder *folder);
    // emitted on incremental folder removal (eg when the user deletes a sync connection via gui)
    void folderRemoved(const QUuid &accountId, Folder *folder);

    // still working these out but generally this belongs in folderman more than anywhere else as it can cross ref
    // existing folders and track unsynced spaces
    // still need to figure out how to "track" synced folder count when a space had a folder but was deleted
    // server side. we show these as "unavailable" in the folder list but really not sure how the effectively dead folder
    // is "counted"
    // void syncedSpaceCountChanged(QUuid accountId, int syncedCount);
    // void unsyncedSpacesChanged(QUuid accountId, QSet<GraphApi::Space *> unsyncedSpaces, int totalSpaceCount);
    void unsyncedSpaceCountChanged(QUuid accountId, int unsyncedCount, int totalSpaces);

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

    // emits folderRemoved
    void removeFolderFromGui(Folder *f);
    void forceFolderSync(Folder *f);

private Q_SLOTS:

    void slotFolderSyncPauseChanged(Folder *, bool paused);
    void slotFolderCanSyncChanged();
    void slotFolderSyncStarted();
    void slotFolderSyncFinished(const SyncResult &);

    void slotRemoveFoldersForAccount(AccountState *accountState);

    void slotServerVersionChanged(Account *account);

    // saves folder using an internally created QSettings instance. This is used for "one off" persistence operations
    // that should be synced immediately - eg when the user adds a new folder sync from the gui or as slot to eg persist a property
    // change
    // see second version of saveFolder, below, for use when persisting many folders in a row
    void saveFolder(Folder *folder);


private:
    explicit FolderMan();

    /**
     * @brief prepareFolder sets up the folder with mac and windows specific operations
     * @param folder path
     * @return true if the folder path exists or can be successfully created
     */
    [[nodiscard]] static bool prepareFolder(const QString &folder);

    /**
     * Adds a folder "from scratch" as oppossd to from config, which requires less setup than when you create the folder
     * from some dynamic operation (eg folders from new account or via the gui add folder sync operations).
     */
    Folder *addFolderFromScratch(AccountState *accountState, FolderDefinition &&definition, bool useVfs);

    /**
     *  private handler connected to spacesManager::ready signal
     *  this is a bit weird as you have to ask the manager if it's ready then wait for the signal before actually loading
     *  the spaces. this function loads all the spaces into the FolderMan and saves them in an efficient manner
     *
     *  emits folderListChanged
     */
    void loadSpaces(AccountState *accountState, bool useVfs);

    /**
     * @brief onSpacesAdded - handles notice from spaces manager that spaces were added
     * @param accountId - account the new spaces live in
     * @param spaces - the new spaces
     * @param totalSpaceCount - total space count after new spaces added
     *
     * important change: the spaces manager now filters out disabled spaces automatically so no one else should be trying to figure out
     * if a space is disabled or not - they will not be part of the active space set from now on
     */
    void onSpacesAdded(const QUuid &accountId, QList<GraphApi::Space *> spaces, int totalSpaceCount);

    /**
     * @brief onSpacesRemoved - handles notice from spaces manager that spaces were removed
     * @param accountId - account the spaces lived in
     * @param spaceIds - these are the spoace id's of removed spaces since the pointers are already gone
     * @param totalSpaceCount - the number of spaces available after removal
     *
     * important change: spaces that have been disabled will now appear in the "removed" collection because the spaces manager filters
     * them out from the start, now.
     */
    void onSpacesRemoved(const QUuid &accountId, QList<QString> spaceIds, int totalSpaceCount);

    /**
     *  reads the folder defs from the config for a single account.
     *  it is important to use a preconfigured settings instance here instead of using a series of one-off instances, to improve efficiecy
     *  by reducing file operations when loading many folders.
     *
     *  The settings should be configured to group "Accounts" before it's passed to this function.
     *
     *  returns false when a downgrade of the database is detected, true otherwise.
     */
    bool addFoldersFromConfigByAccount(QSettings &settings, AccountState *account);

    // tests folder def for minimum reqs
    bool validateFolderDefinition(const FolderDefinition &folderDefinition);

    /** Connects a folder instance, provided it has no setup errors
     */
    void connectFolder(Folder *folder);

    /* disconnects a folder instance, provided it has no setup errors
     */
    void disconnectFolder(Folder *folder);

    /* disconnects the signals that trigger autosave
     * this is a bit dirty imo but we need to disconnect the autosave handling when a folder is removed from the settings
     * because in the subsequent step where the folder is removed from the folder man, we will get a stray signal or two that causes it
     * to be written back to the config again.
     * we can't call removeFromSettings after removeFolderSync as the folder is deleted in that step
     * in future when we have a better encapsulation on the folder manager this small detail will be easier to manage.
     */
    void disconnectAutoSave(Folder *folder);

    /* saves folder using the given instance of config settings already set to group "Accounts"
     * The main purpose of this version is to allow using a single settings instance to persist multiple folders
     * in a row before syncing -> we save a lot of expensive file operations this way
     */
    void saveFolder(Folder *folder, QSettings &settings);

    // used to reduce file operation overhead when removing multiple folders
    // impl detail: we also disconnect the folder from autosave here!
    void removeFolderSettings(Folder *folder, QSettings &settings);

    /**
     *  Removes a folder sync permanently and deletes the folder
     *  Not for general folder cleanup
     *  it is caller's responsibility to remove folder from settings if necessary.
     */
    void deleteFolderSync(Folder *);

    /** Queues all folders for syncing. */
    void scheduleAllFolders();

    // finds all folder configuration files
    // and create the folders
    QString getBackupName(QString fullPathName) const;

    // makes the folder known to the socket api
    // pair this with _socketApi->slotUnregisterPath(folder);
    void registerFolderWithSocketApi(Folder *folder);

    QString _folderConfigPath;
    bool _ignoreHiddenFiles = true;

    /// Folder aliases from the settings that weren't read
    QSet<QString> _additionalBlockedFolderAliases;

    /// Watches files that couldn't be synced due to locks
    QScopedPointer<LockWatcher> _lockWatcher;

    /// Scheduled folders that should be synced as soon as possible
    SyncScheduler *_scheduler;

    std::unique_ptr<SocketApi> _socketApi;

    mutable QMap<QString, Result<void, QString>> _unsupportedConfigurationError;

    static FolderMan *_instance;

    // the inner hash contains the folder pointers hashed against their spaceId which makes any kind if retrieval *much*
    // faster. the folders need to be split by account id for a variety of reasons, but a very important factor is that the "shares" space
    // always has the same space id even across accounts.
    // uuid is the account id, qstring is the folder's space id, folder is obvious I hope
    QHash<QUuid, QHash<QString, Folder *>> _folders;


    // similar to the _folders container, unsynced spaces need to be split by account id primarily because the "shares" space
    // always has the same space id even across accounts.
    // inner hash contains the unsynced spaces hashed by spaceid
    // uuid is the account id, qstring is the space id, space is self explanatory
    QHash<QUuid, QHash<QString, GraphApi::Space *>> _unsyncedSpaces;

    // as far as I can tell these are paused folders, not to be confused with folders whose space is disabled. We add and remove folders to this
    // set when they are paused/resumed but aside from that we don't really do anything with it.
    QSet<Folder *> _disabledFolders;

    friend class OCC::Application;

    // the literal is needed to get the tests to build
    inline static const QString IgnoreHiddenFilesKey = QStringLiteral("ignoreHiddenFiles");
    void scheduleFoldersForAccount(const QUuid &accountId);
};

} // namespace OCC
