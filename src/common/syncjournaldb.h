/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SYNCJOURNALDB_H
#define SYNCJOURNALDB_H

#include <QObject>
#include <qmutex.h>
#include <QDateTime>
#include <QHash>
#include <functional>

#include "common/checksumalgorithms.h"
#include "common/ownsql.h"
#include "common/pinstate.h"
#include "common/preparedsqlquerymanager.h"
#include "common/result.h"
#include "common/syncjournalfilerecord.h"
#include "common/utility.h"

namespace OCC {
class SyncJournalFileRecord;

/**
 * @brief Class that handles the sync database
 *
 * This class is thread safe. All public functions lock the mutex.
 * @ingroup libsync
 */
class OCSYNC_EXPORT SyncJournalDb : public QObject
{
    Q_OBJECT
public:
    explicit SyncJournalDb(const QString &dbFilePath, QObject *parent = nullptr);
    ~SyncJournalDb() override;

    /// Create a journal path for a specific configuration
    static QString makeDbName(const QString &localPath,
        const QString &infix = QStringLiteral("journal"));

    static bool dbIsTooNewForClient(const QString &dbFilePath);

    // To verify that the record could be found check with SyncJournalFileRecord::isValid()
    bool getFileRecord(const QString &filename, SyncJournalFileRecord *rec) { return getFileRecord(filename.toUtf8(), rec); }
    bool getFileRecord(const QByteArray &filename, SyncJournalFileRecord *rec);
    bool getFileRecordByInode(quint64 inode, SyncJournalFileRecord *rec);
    bool getFileRecordsByFileId(const QByteArray &fileId, const std::function<void(const SyncJournalFileRecord &)> &rowCallback);
    bool getFilesBelowPath(const QByteArray &path, const std::function<void(const SyncJournalFileRecord&)> &rowCallback);
    bool listFilesInPath(const QByteArray &path, const std::function<void(const SyncJournalFileRecord&)> &rowCallback);
    const QVector<SyncJournalFileRecord> getFileRecordsWithDirtyPlaceholders() const;
    Result<void, QString> setFileRecord(const SyncJournalFileRecord &record);

    bool deleteFileRecord(const QString &filename, bool recursively = false);
    bool updateFileRecordChecksum(const QString &filename,
        const QByteArray &contentChecksum,
        CheckSums::Algorithm contentChecksumType);

    /// Return value for hasHydratedOrDehydratedFiles()
    struct HasHydratedDehydrated
    {
        bool hasHydrated = false;
        bool hasDehydrated = false;
    };

    /** Returns whether the item or any subitems are dehydrated */
    Optional<HasHydratedDehydrated> hasHydratedOrDehydratedFiles(const QByteArray &filename);

    bool exists();
    void walCheckpoint();

    QString databaseFilePath() const;

    static qint64 getPHash(const QByteArray &);

    void setErrorBlacklistEntry(const SyncJournalErrorBlacklistRecord &item);
    void wipeErrorBlacklistEntry(const QString &relativeFile);
    void wipeErrorBlacklistEntry(const QString &relativeFile, SyncJournalErrorBlacklistRecord::Category category);
    void wipeErrorBlacklistCategory(SyncJournalErrorBlacklistRecord::Category category);
    int wipeErrorBlacklist();
    int errorBlackListEntryCount();

    struct DownloadInfo
    {
        DownloadInfo()
            : _errorCount(0)
            , _valid(false)
        {
        }
        QString _tmpfile;
        QByteArray _etag;
        int _errorCount;
        bool _valid;
    };
    struct UploadInfo
    {
        int _chunk = 0;
        uint _transferid = 0;
        qint64 _size = 0;
        qint64 _modtime = 0;
        int _errorCount = 0;
        bool _valid = false;
        QByteArray _contentChecksum;
        QString _path; // stored as utf16
        QUrl _url; // upload url (tus)
        /**
         * Returns true if this entry refers to a chunked upload that can be continued.
         * (As opposed to a small file transfer which is stored in the db so we can detect the case
         * when the upload succeeded, but the connection was dropped before we got the answer)
         */
        bool isChunked() const { return _transferid != 0; }

        bool validate(qint64 size, qint64 modtime, const QByteArray &checksum) const
        {
            Q_ASSERT(!checksum.isEmpty());
            Q_ASSERT(!_valid || !_contentChecksum.isEmpty());
            return _valid && _size == size && _modtime == modtime && _contentChecksum == checksum;
        }
    };

    DownloadInfo getDownloadInfo(const QString &file);
    void setDownloadInfo(const QString &file, const DownloadInfo &i);
    QVector<DownloadInfo> getAndDeleteStaleDownloadInfos(const QSet<QString> &keep);
    int downloadInfoCount();

    UploadInfo getUploadInfo(const QString &file);
    std::vector<UploadInfo> getUploadInfos();

    void setUploadInfo(const QString &file, const UploadInfo &i);
    // Return the list of transfer ids that were removed.
    QVector<uint> deleteStaleUploadInfos(const QSet<QString> &keep);

    SyncJournalErrorBlacklistRecord errorBlacklistEntry(const QString &);
    bool deleteStaleErrorBlacklistEntries(const QSet<QString> &keep);

    /// Delete flags table entries that have no metadata correspondent
    void deleteStaleFlagsEntries();

    void avoidRenamesOnNextSync(const QString &path) { avoidRenamesOnNextSync(path.toUtf8()); }
    void avoidRenamesOnNextSync(const QByteArray &path);

    enum SelectiveSyncListType {
        /** The black list is the list of folders that are unselected in the selective sync dialog.
         * For the sync engine, those folders are considered as if they were not there, so the local
         * folders will be deleted */
        SelectiveSyncBlackList = 1,
        /** When a shared folder has a size bigger than a configured size, it is by default not sync'ed
         * Unless it is in the white list, in which case the folder is sync'ed and all its children.
         * If a folder is both on the black and the white list, the black list wins */
        SelectiveSyncWhiteList = 2,
        /** List of big sync folders that have not been confirmed by the user yet and that the UI
         * should notify about */
        SelectiveSyncUndecidedList = 3
    };
    Q_ENUM(SelectiveSyncListType)

    /* return the specified list from the database
     * ok param holds value indicating "success" of the operation. If the sync list cannot be retrieved ok will be false
     */
    QSet<QString> getSelectiveSyncList(SelectiveSyncListType type, bool &ok);

    /* Write the selective sync list (remove all other entries of that list */
    void setSelectiveSyncList(SelectiveSyncListType type, const QSet<QString> &list);

    /**
     * Make sure that on the next sync fileName and its parents are discovered from the server.
     *
     * That means its metadata and, if it's a directory, its direct contents.
     *
     * Specifically, etag (md5 field) of fileName and all its parents are set to _invalid_.
     * That causes a metadata difference and a resulting discovery from the remote for the
     * affected folders.
     *
     * Since folders in the selective sync list will not be rediscovered (csync_ftw,
     * _csync_detect_update skip them), the _invalid_ marker will stay. And any
     * child items in the db will be ignored when reading a remote tree from the database.
     *
     * Any setFileRecord() call to affected directories before the next sync run will be
     * adjusted to retain the invalid etag via _etagStorageFilter.
     */
    void schedulePathForRemoteDiscovery(const QString &fileName) { schedulePathForRemoteDiscovery(fileName.toUtf8()); }
    void schedulePathForRemoteDiscovery(const QByteArray &fileName);

    /**
     * Wipe _etagStorageFilter. Also done implicitly on close().
     */
    void clearEtagStorageFilter();

    /**
     * Ensures full remote discovery happens on the next sync.
     *
     * Equivalent to calling schedulePathForRemoteDiscovery() for all files.
     */
    void forceRemoteDiscoveryNextSync();

    /* Because sqlite transactions are really slow, we encapsulate everything in big transactions
     * Commit will actually commit the transaction and create a new one.
     */
    void commit(const QString &context, bool startTrans = true);
    void commitIfNeededAndStartNewTransaction(const QString &context);

    /** Open the db if it isn't already.
     *
     * This usually creates some temporary files next to the db file, like
     * $dbfile-shm or $dbfile-wal.
     *
     * returns true if it could be openend or is currently opened.
     */
    bool open();

    /** Returns whether the db is currently openend. */
    bool isOpen() const;

    /** Close the database */
    void close();

    /**
     * allow to reopen/recreate the db after it was closed (unit tests)
     * This is usually not allowed to prevent accidential recreation of db.
     */
    void allowReopen();

    /**
     * Returns the checksum type for an id.
     */
    QByteArray getChecksumType(int checksumTypeId);

    /**
     * The data-fingerprint used to detect backup
     */
    void setDataFingerprint(const QByteArray &dataFingerprint);
    QByteArray dataFingerprint();


    // Conflict record functions

    /// Store a new or updated record in the database
    void setConflictRecord(const ConflictRecord &record);

    /// Retrieve a conflict record by path of the file with the conflict tag
    ConflictRecord conflictRecord(const QByteArray &path);

    /// Delete a conflict record by path of the file with the conflict tag
    void deleteConflictRecord(const QByteArray &path);

    /// Return all paths of files with a conflict tag in the name and records in the db
    QByteArrayList conflictRecordPaths();

    /** Find the base name for a conflict file name, using journal or name pattern
     *
     * The path must by sync-folder relative.
     *
     * Will return an empty string if it's not even a conflict file by pattern.
     */
    QByteArray conflictFileBaseName(const QByteArray &conflictName);

    /**
     * Delete any file entry. This will force the next sync to re-sync everything as if it was new,
     * restoring everyfile on every remote. If a file is there both on the client and server side,
     * it will be a conflict that will be automatically resolved if the file is the same.
     */
    void clearFileTable();

    /**
     * Set the 'ItemTypeVirtualFileDownload' to all the files that have the ItemTypeVirtualFile flag
     * within the directory specified path path
     *
     * The path "" marks everything.
     */
    void markVirtualFileForDownloadRecursively(const QByteArray &path);

    /** Grouping for all functions relating to pin states,
     *
     * Use internalPinStates() to get at them.
     */
    struct OCSYNC_EXPORT PinStateInterface
    {
        PinStateInterface(const PinStateInterface &) = delete;
        PinStateInterface(PinStateInterface &&) = delete;

        /**
         * Gets the PinState for the path without considering parents.
         *
         * If a path has no explicit PinState "Inherited" is returned.
         *
         * The path should not have a trailing slash.
         * It's valid to use the root path "".
         *
         * Returns none on db error.
         */
        Optional<PinState> rawForPath(const QByteArray &path);

        /**
         * Gets the PinState for the path after inheriting from parents.
         *
         * If the exact path has no entry or has an Inherited state,
         * the state of the closest parent path is returned.
         *
         * The path should not have a trailing slash.
         * It's valid to use the root path "".
         *
         * Never returns PinState::Inherited. If the root is "Inherited"
         * or there's an error, "AlwaysLocal" is returned.
         *
         * Returns none on db error.
         */
        Optional<PinState> effectiveForPath(const QByteArray &path);

        /**
         * Like effectiveForPath() but also considers subitem pin states.
         *
         * If the path's pin state and all subitem's pin states are identical
         * then that pin state will be returned.
         *
         * If some subitem's pin state is different from the path's state,
         * PinState::Inherited will be returned. Inherited isn't returned in
         * any other cases.
         *
         * It's valid to use the root path "".
         * Returns none on db error.
         */
        Optional<PinState> effectiveForPathRecursive(const QByteArray &path);

        /**
         * Sets a path's pin state.
         *
         * The path should not have a trailing slash.
         * It's valid to use the root path "".
         */
        void setForPath(const QByteArray &path, PinState state);

        /**
         * Wipes pin states for a path and below.
         *
         * Used when the user asks a subtree to have a particular pin state.
         * The path should not have a trailing slash.
         * The path "" wipes every entry.
         */
        void wipeForPathAndBelow(const QByteArray &path);

        /**
         * Returns list of all paths with their pin state as in the db.
         *
         * Returns nothing on db error.
         * Note that this will have an entry for "".
         */
        Optional<QVector<QPair<QByteArray, PinState>>> rawList();

        SyncJournalDb *_db;
    };
    friend struct PinStateInterface;

    /** Access to PinStates stored in the database.
     *
     * Important: Not all vfs plugins store the pin states in the database,
     * prefer to use Vfs::pinState() etc.
     */
    PinStateInterface internalPinStates();

    /**
     * Only used for auto-test:
     * when positive, will decrease the counter for every database operation.
     * reaching 0 makes the operation fails
     */
    int autotestFailCounter = -1;

private:
    int getFileRecordCount();
    bool updateDatabaseStructure();
    bool updateMetadataTableStructure();
    bool updateErrorBlacklistTableStructure();
    bool sqlFail(const QString &log, const SqlQuery &query);
    void commitInternal(const QString &context, bool startTrans = true);
    void startTransaction();
    void commitTransaction();
    QVector<QByteArray> tableColumns(const QByteArray &table);
    bool checkConnect();

    bool createUploadInfo();

    // Same as forceRemoteDiscoveryNextSync but without acquiring the lock
    void forceRemoteDiscoveryNextSyncLocked();

    // Returns the integer id of the checksum type
    //
    // Returns 0 on failure and for empty checksum types.
    int mapChecksumType(CheckSums::Algorithm checksumType);

    SqlDatabase _db;
    QString _dbFile;
    mutable QRecursiveMutex _mutex; // Public functions are protected with the mutex.
    QMap<CheckSums::Algorithm, int> _checksymTypeCache;
    int _transaction;
    bool _metadataTableIsEmpty;

    /* Storing etags to these folders, or their parent folders, is filtered out.
     *
     * When schedulePathForRemoteDiscovery() is called some etags to _invalid_ in the
     * database. If this is done during a sync run, a later propagation job might
     * undo that by writing the correct etag to the database instead. This filter
     * will prevent this write and instead guarantee the _invalid_ etag stays in
     * place.
     *
     * The list is cleared on close() (end of sync run) and explicitly with
     * clearEtagStorageFilter() (start of sync run).
     *
     * The contained paths have a trailing /.
     */
    QList<QByteArray> _etagStorageFilter;

    /** The journal mode to use for the db.
     *
     * Typically WAL initially, but may be set to other modes via environment
     * variable, for specific filesystems, or when WAL fails in a particular way.
     */
    QByteArray _journalMode;

    mutable PreparedSqlQueryManager _queryManager;

    /**
     * Whether the db was already closed, prevent recreation
     */
    bool _closed = false;
};

bool OCSYNC_EXPORT
operator==(const SyncJournalDb::DownloadInfo &lhs,
    const SyncJournalDb::DownloadInfo &rhs);
bool OCSYNC_EXPORT
operator==(const SyncJournalDb::UploadInfo &lhs,
    const SyncJournalDb::UploadInfo &rhs);

} // namespace OCC
#endif // SYNCJOURNALDB_H
