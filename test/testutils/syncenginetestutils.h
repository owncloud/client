/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */
#pragma once

#include "account.h"
#include "common/filesystembase.h"
#include "common/syncjournaldb.h"
#include "common/syncjournalfilerecord.h"
#include "common/vfs.h"
#include "creds/abstractcredentials.h"
#include "csync_exclude.h"
#include "filesystem.h"
#include "folder.h"
#include "logger.h"
#include "syncengine.h"
#include "testutils.h"
#include <cstring>

#include <QDir>
#include <QMap>
#include <QNetworkReply>
#include <QTimer>
#include <QtTest>
#include <cookiejar.h>

#include <chrono>

using namespace OCC::FileSystem::SizeLiterals;
/*
 * TODO: In theory we should use QVERIFY instead of Q_ASSERT for testing, but this
 * only works when directly called from a QTest :-(
 */

QString getFilePathFromUrl(const QUrl &url);


inline QByteArray generateEtag()
{
    return QByteArray::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(), 16) + QByteArray::number(QRandomGenerator::global()->generate(), 16);
}
inline QByteArray generateFileId()
{
    return QByteArray::number(QRandomGenerator::global()->generate(), 16);
}

class PathComponents : public QStringList
{
public:
    PathComponents(const QString &path);
    PathComponents(const QStringList &pathComponents);

    PathComponents parentDirComponents() const;
    PathComponents subComponents() const &;
    PathComponents subComponents() &&
    {
        removeFirst();
        return std::move(*this);
    }
    QString pathRoot() const { return first(); }
    QString fileName() const { return last(); }
};

/**
 * @brief The FileModifier class defines the interface for both the local on-disk modifier and the
 * remote in-memory modifier.
 */
class FileModifier
{
public:
    static constexpr auto DefaultFileSize = 64_B;
    static constexpr char DefaultContentChar = 'X';

    virtual ~FileModifier() { }
    virtual void remove(const QString &relativePath) = 0;
    virtual void insert(const QString &relativePath, quint64 size = DefaultFileSize, char contentChar = DefaultContentChar) = 0;

    virtual void setContents(const QString &relativePath, quint64 newSize, char contentChar = DefaultContentChar) = 0;

    // prevent implicit cast to quint64
    template <typename T>
    void setContents(const QString &relativePath, T newSize, char contentChar = DefaultContentChar) = delete;

    // prevent implicit cast to quint64
    template <typename T>
    void insert(const QString &relativePath, T size, char contentChar = DefaultContentChar) = delete;

    virtual void appendByte(const QString &relativePath, char contentChar = DefaultContentChar) = 0;
    virtual void mkdir(const QString &relativePath) = 0;
    virtual void rename(const QString &relativePath, const QString &relativeDestinationDirectory) = 0;
    virtual void setModTime(const QString &relativePath, const QDateTime &modTime) = 0;
};

class FakeFolder;

/**
 * @brief The DiskFileModifier class is a FileModifier for files on the local disk.
 *
 * It uses a helper program (`test_helper`) to do the actual modifications. When running without a
 * VFS, or when running with suffix-VFS, the changes are done before doing a sync. When running
 * with a VFS, the OS will do callbacks to the test program. So in this case the modifications are
 * done while the event loop is running, in order to process those OS callbacks.
 *
 * Note: these actions do NOT read content from files on disk: if the file would be dehydrated,
 * reading will cause a re-hydration, which means network requests, and some tests explicitly check
 * for the amount of requests.
 */
class DiskFileModifier : public FileModifier
{
    QDir _rootDir;
    QStringList _processArguments;

public:
    DiskFileModifier(const QString &rootDirPath)
        : _rootDir(rootDirPath)
    {
    }
    void remove(const QString &relativePath) override;
    void insert(const QString &relativePath, quint64 size = DefaultFileSize, char contentChar = DefaultContentChar) override;
    void setContents(const QString &relativePath, quint64 newSize, char contentChar = DefaultContentChar) override;
    void appendByte(const QString &relativePath, char contentChar = DefaultContentChar) override;

    void mkdir(const QString &relativePath) override;
    void rename(const QString &from, const QString &to) override;
    void setModTime(const QString &relativePath, const QDateTime &modTime) override;

    Q_REQUIRED_RESULT bool applyModifications();

    // prevent implicit cast to quint64
    template <typename T>
    void setContents(const QString &relativePath, T newSize, char contentChar = DefaultContentChar) = delete;

    // prevent implicit cast to quint64
    template <typename T>
    void insert(const QString &relativePath, T size, char contentChar = DefaultContentChar) = delete;
};

static inline qint64 defaultLastModified()
{
    auto precise = QDateTime::currentDateTimeUtc().addDays(-7);
    time_t timeInSeconds = OCC::Utility::qDateTimeToTime_t(precise);
    return timeInSeconds;
}

/**
 * @brief The FileInfo class represents the remotely stored (on-server) content. To be able to
 * modify the content, this class is also the remote \c FileModifier.
 */
class FileInfo : public FileModifier
{
    /// FIXME: we should make it explicit in the construtor if we're talking about a hydrated or a dehydrated file!
    /// FIXME: this class is both a remote folder, as the root folder which in turn is the remote FileModifier.
    ///        This is a mess: unifying remote files/folders is ok, but because it's also the remote root (which
    ///        should implement the FileModifier), this means that each single remote file/folder also implements
    ///        this interface.
public:
    static FileInfo A12_B12_C12_S12();

    FileInfo() = default;
    FileInfo(const QString &name)
        : name { name }
    {
    }
    FileInfo(const QString &name, quint64 size)
        : name { name }
        , isDir { false }
        , fileSize(size)
        , contentSize { size }
    {
    }
    FileInfo(const QString &name, quint64 size, char contentChar)
        : name { name }
        , isDir { false }
        , fileSize(size)
        , contentSize { size }
        , contentChar { contentChar }
    {
    }
    FileInfo(const QString &name, const std::initializer_list<FileInfo> &children);

    FileInfo &addChild(const FileInfo &info);

    void remove(const QString &relativePath) override;

    void insert(const QString &relativePath, quint64 size = DefaultFileSize, char contentChar = DefaultContentChar) override;

    void setContents(const QString &relativePath, quint64 newSize, char contentChar = DefaultContentChar) override;

    void appendByte(const QString &relativePath, char contentChar = DefaultContentChar) override;

    void mkdir(const QString &relativePath) override;

    void rename(const QString &oldPath, const QString &newPath) override;

    void setModTime(const QString &relativePath, const QDateTime &modTime) override;

    /// Return a pointer to the FileInfo, or a nullptr if it doesn't exist
    FileInfo *find(PathComponents pathComponents, const bool invalidateEtags = false);
    FileInfo *find(const QString &pathComponents, const bool invalidateEtags = false) { return find(PathComponents{pathComponents}, invalidateEtags); }

    FileInfo *createDir(const QString &relativePath);

    FileInfo *create(const QString &relativePath, quint64 size, char contentChar);

    bool operator<(const FileInfo &other) const
    {
        return name < other.name;
    }

    enum CompareWhat {
        CompareLastModified,
        IgnoreLastModified,
    };

    bool operator==(const FileInfo &other) const { return equals(other, CompareLastModified); }

    bool equals(const FileInfo &other, CompareWhat compareWhat) const;

    bool operator!=(const FileInfo &other) const
    {
        return !operator==(other);
    }

    QDateTime lastModified() const
    {
        return QDateTime::fromSecsSinceEpoch(_lastModifiedInSecondsUTC, Qt::LocalTime);
    }

    QDateTime lastModifiedInUtc() const
    {
        return QDateTime::fromSecsSinceEpoch(_lastModifiedInSecondsUTC, Qt::UTC);
    }

    void setLastModified(const QDateTime &t)
    {
        _lastModifiedInSecondsUTC = t.toSecsSinceEpoch();
    }

    qint64 lastModifiedInSecondsUTC() const
    {
        return _lastModifiedInSecondsUTC;
    }

    void setLastModifiedFromSecondsUTC(qint64 utcSecs)
    {
        _lastModifiedInSecondsUTC = utcSecs;
    }

    QString path() const;
    QString absolutePath() const;

    void fixupParentPathRecursively();

    QString name;
    bool isDir = true;
    bool isShared = false;
    OCC::RemotePermissions permissions; // When uset, defaults to everything
    qint64 _lastModifiedInSecondsUTC = defaultLastModified();
    QByteArray etag = generateEtag();
    QByteArray fileId = generateFileId();
    QByteArray checksums;
    QByteArray extraDavProperties;
    quint64 fileSize = 0;
    quint64 contentSize = 0;
    char contentChar = 'W';
    bool isDehydratedPlaceholder = false;

    // Sorted by name to be able to compare trees
    QMap<QString, FileInfo> children;
    QString parentPath;

    FileInfo *findInvalidatingEtags(PathComponents pathComponents);

    friend inline QDebug operator<<(QDebug dbg, const FileInfo &fi)
    {
        return dbg.nospace().noquote()
            << "{ '" << fi.path() << "': "
            << ", isDir:" << fi.isDir
            << QStringLiteral(", lastModified: %1 (%2)").arg(QString::number(fi._lastModifiedInSecondsUTC), fi.lastModifiedInUtc().toString())
            << ", fileSize:" << fi.fileSize
            << ", contentSize:" << fi.contentSize
            << QStringLiteral(", contentChar: 0x%1").arg(QString::number(int(fi.contentChar), 16))
            << ", isDehydratedPlaceholder:" << fi.isDehydratedPlaceholder
            << ", children:" << fi.children
            << " }";
    }

    // prevent implicit cast to quint64
    template <typename T>
    void setContents(const QString &relativePath, T newSize, char contentChar = DefaultContentChar) = delete;

    // prevent implicit cast to quint64
    template <typename T>
    void insert(const QString &relativePath, T size, char contentChar = DefaultContentChar) = delete;
};

class FakeReply : public QNetworkReply
{
    Q_OBJECT

public:
    FakeReply(QObject *parent);
    virtual ~FakeReply() override;

    // useful to be public for testing
    using QNetworkReply::setAttribute;
    using QNetworkReply::setRawHeader;

    void checkedFinished();

    virtual void abort() override;
};

class FakePropfindReply : public FakeReply

{
    Q_OBJECT

public:
    QByteArray payload;

    FakePropfindReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    Q_INVOKABLE void respond();

    Q_INVOKABLE void respond404();

    qint64 bytesAvailable() const override;
    qint64 readData(char *data, qint64 maxlen) override;
};

class FakePutReply : public FakeReply
{
    Q_OBJECT

public:
    FakePutReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &putPayload, QObject *parent);

    static FileInfo *perform(FileInfo &remoteRootFileInfo, const QNetworkRequest &request, const QByteArray &putPayload);

    Q_INVOKABLE virtual void respond();

    qint64 readData(char *, qint64) override { return 0; }

private:
    FileInfo *fileInfo;
};

class FakeMkcolReply : public FakeReply
{
    Q_OBJECT

public:
    FakeMkcolReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    Q_INVOKABLE void respond();

    qint64 readData(char *, qint64) override { return 0; }

private:
    FileInfo *fileInfo;
};

class FakeDeleteReply : public FakeReply
{
    Q_OBJECT

public:
    FakeDeleteReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    Q_INVOKABLE void respond();

    qint64 readData(char *, qint64) override { return 0; }
};

class FakeMoveReply : public FakeReply
{
    Q_OBJECT

public:
    FakeMoveReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    Q_INVOKABLE void respond();

    qint64 readData(char *, qint64) override { return 0; }
};

class FakeGetReply : public FakeReply
{
    Q_OBJECT

public:
    enum class State {
        Ok,
        Aborted,
        FileNotFound,
    };
    Q_ENUM(State);

    const FileInfo *fileInfo;
    char payload;
    qint64 size;
    State state = State::Ok;
    const std::pair<qint64, qint64> _range;

    FakeGetReply(FileInfo &remoteRootFileInfo, QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    virtual void respond();

    void abort() override;
    virtual qint64 bytesAvailable() const override;

    virtual qint64 readData(char *data, qint64 maxlen) override;

    static std::pair<qint64, qint64> parseRange(const QNetworkRequest &request);
};

class FakeChunkMoveReply : public FakeReply
{
    Q_OBJECT
    FileInfo *fileInfo;

public:
    FakeChunkMoveReply(FileInfo &uploadsFileInfo, FileInfo &remoteRootFileInfo,
        QNetworkAccessManager::Operation op, const QNetworkRequest &request,
        QObject *parent);

    static FileInfo *perform(FileInfo &uploadsFileInfo, FileInfo &remoteRootFileInfo, const QNetworkRequest &request);

    Q_INVOKABLE virtual void respond();

    Q_INVOKABLE void respondPreconditionFailed();

    qint64 readData(char *, qint64) override { return 0; }
};

class FakePayloadReply : public FakeReply
{
    Q_OBJECT
public:
    FakePayloadReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
        const QByteArray &body, QObject *parent);

    virtual void respond();

    qint64 readData(char *buf, qint64 max) override;
    qint64 bytesAvailable() const override;
    QByteArray _body;
};


class FakeErrorReply : public FakeReply
{
    Q_OBJECT
public:
    FakeErrorReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
        QObject *parent, int httpErrorCode, const QByteArray &body = QByteArray());

    Q_INVOKABLE virtual void respond();

    // make public to give tests easy interface
    using QNetworkReply::setAttribute;
    using QNetworkReply::setError;

public Q_SLOTS:
    void slotSetFinished();

public:
    qint64 readData(char *buf, qint64 max) override;
    qint64 bytesAvailable() const override;

    QByteArray _body;
};

// A reply that never responds
class FakeHangingReply : public FakeReply
{
    Q_OBJECT
public:
    FakeHangingReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QObject *parent);

    qint64 readData(char *, qint64) override { return 0; }
};

// A delayed reply
template <class OriginalReply>
class DelayedReply : public OriginalReply
{
public:
    template <typename... Args>
    explicit DelayedReply(const std::chrono::milliseconds delayMS, Args &&...args)
        : OriginalReply(std::forward<Args>(args)...)
        , _delayMs(delayMS)
    {
    }
    std::chrono::milliseconds _delayMs;

    void respond() override
    {
        QTimer::singleShot(_delayMs, static_cast<OriginalReply *>(this), [this] {
            // Explicit call to bases's respond();
            this->OriginalReply::respond();
        });
    }
};

class FakeAM : public OCC::AccessManager
{
public:
    using Override = std::function<QNetworkReply *(Operation, const QNetworkRequest &, QIODevice *)>;

private:
    FileInfo _remoteRootFileInfo;
    FileInfo _uploadFileInfo;
    // maps a path to an HTTP error
    QHash<QString, int> _errorPaths;
    // monitor requests and optionally provide custom replies
    Override _override;

public:
    FakeAM(FileInfo initialRoot, QObject *parent);
    FileInfo &currentRemoteState() { return _remoteRootFileInfo; }
    FileInfo &uploadState() { return _uploadFileInfo; }

    QHash<QString, int> &errorPaths() { return _errorPaths; }

    void setOverride(const Override &override) { _override = override; }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
        QIODevice *outgoingData = nullptr) override;
};

class FakeCredentials : public OCC::AbstractCredentials
{
public:
    FakeCredentials(OCC::AccessManager *am)
        : _am { am }
    {
    }

    QString authType() const override { return QStringLiteral("test"); }
    QString user() const override { return QStringLiteral("admin"); }
    OCC::AccessManager *createAM() const override { return _am; }
    bool ready() const override { return true; }
    void fetchFromKeychain() override { }
    void askFromUser() override { }
    bool stillValid(QNetworkReply *) override { return true; }
    void persist() override { }
    void invalidateToken() override { }
    void forgetSensitiveData() override { }

private:
    OCC::AccessManager *_am;
};

class FakeFolder : public QObject
{
    Q_OBJECT
    const QTemporaryDir _tempDir = OCC::TestUtils::createTempDir();
    DiskFileModifier _localModifier;
    FakeAM *_fakeAm;
    OCC::TestUtils::TestUtilsPrivate::AccountStateRaii _accountState =
        OCC::TestUtils::TestUtilsPrivate::AccountStateRaii{nullptr, &OCC::TestUtils::TestUtilsPrivate::accountStateDeleter};
    std::unique_ptr<OCC::SyncJournalDb> _journalDb;
    std::unique_ptr<OCC::SyncEngine> _syncEngine;

public:
    FakeFolder(const FileInfo &fileTemplate, OCC::Vfs::Mode vfsMode = OCC::Vfs::Off, bool filesAreDehydrated = false);
    ~FakeFolder();

    void switchToVfs(QSharedPointer<OCC::Vfs> vfs);

    OCC::AccountPtr account() const { return _accountState->account(); }
    OCC::SyncEngine &syncEngine() const { return *_syncEngine; }
    OCC::SyncJournalDb &syncJournal() const { return *_journalDb; }

    FileModifier &localModifier() { return _localModifier; }
    FileInfo &remoteModifier() { return _fakeAm->currentRemoteState(); }
    FileInfo currentLocalState();

    FileInfo &currentRemoteState() { return _fakeAm->currentRemoteState(); }
    FileInfo &uploadState() { return _fakeAm->uploadState(); }
    FileInfo dbState() const;

    struct ErrorList
    {
        FakeAM *_qnam;
        void append(const QString &path, int error = 500)
        {
            _qnam->errorPaths().insert(path, error);
        }
        void clear() { _qnam->errorPaths().clear(); }
    };
    ErrorList serverErrorPaths() { return { _fakeAm }; }
    void setServerOverride(const FakeAM::Override &override) { _fakeAm->setOverride(override); }

    QString localPath() const;

    void scheduleSync();

    void execUntilBeforePropagation();

    void execUntilItemCompleted(const QString &relativePath);

    bool execUntilFinished();

    bool syncOnce();

    Q_REQUIRED_RESULT bool applyLocalModificationsAndSync()
    {
        if (!_localModifier.applyModifications()) {
            return false;
        }
        return syncOnce();
    }

    Q_REQUIRED_RESULT bool applyLocalModificationsWithoutSync()
    {
        return _localModifier.applyModifications();
    }

    bool isDehydratedPlaceholder(const QString &filePath);
    QSharedPointer<OCC::Vfs> vfs() const;

private:
    static void toDisk(QDir &dir, const FileInfo &templateFi);

    void fromDisk(QDir &dir, FileInfo &templateFi);
};


/* Return the FileInfo for a conflict file for the specified relative filename */
inline const FileInfo *findConflict(FileInfo &dir, const QString &filename)
{
    QFileInfo info(filename);
    const FileInfo *parentDir = dir.find(info.path());
    if (!parentDir)
        return nullptr;
    QString start = info.baseName() + QStringLiteral(" (conflicted copy");
    for (const auto &item : parentDir->children) {
        if (item.name.startsWith(start)) {
            return &item;
        }
    }
    return nullptr;
}

struct ItemCompletedSpy : QSignalSpy
{
    explicit ItemCompletedSpy(FakeFolder &folder)
        : QSignalSpy(&folder.syncEngine(), &OCC::SyncEngine::itemCompleted)
    {
    }

    OCC::SyncFileItemPtr findItem(const QString &path) const;
};

// QTest::toString overloads
namespace OCC {
inline char *toString(const SyncFileStatus &s)
{
    return QTest::toString(QStringLiteral("SyncFileStatus(%1)").arg(s.toSocketAPIString()));
}
}

inline void addFiles(QStringList &dest, const FileInfo &fi)
{
    if (fi.isDir) {
        dest += QStringLiteral("%1 - dir").arg(fi.path());
        for (const auto &child : fi.children)
            addFiles(dest, child);
    } else {
        dest += QStringLiteral("%1 - %2 %3-bytes (%4)").arg(fi.path(), QString::number(fi.contentSize), QChar::fromLatin1(fi.contentChar), fi.lastModifiedInUtc().toString());
    }
}

inline QString toStringNoElide(const FileInfo &fi)
{
    QStringList files;
    for (const auto &childInfo : fi.children) {
        addFiles(files, childInfo);
    }
    files.sort();
    return QStringLiteral("FileInfo with %1 files(\n\t%2\n)").arg(files.size()).arg(files.join(QStringLiteral("\n\t")));
}

inline char *toString(const FileInfo &fi)
{
    return QTest::toString(toStringNoElide(fi));
}

inline void addFilesDbData(QStringList &dest, const FileInfo &fi)
{
    // could include etag, permissions etc, but would need extra work
    if (fi.isDir) {
        dest += QStringLiteral("%1 - %2 %3 %4").arg(fi.name, fi.isDir ? QStringLiteral("dir") : QStringLiteral("file"), QString::number(fi.lastModifiedInSecondsUTC()), QString::fromUtf8(fi.fileId));
        for (const auto &child : fi.children)
            addFilesDbData(dest, child);
    } else {
        dest += QStringLiteral("%1 - %2 %3 %4 %5").arg(fi.name, fi.isDir ? QStringLiteral("dir") : QStringLiteral("file"), QString::number(fi.contentSize), QString::number(fi.lastModifiedInSecondsUTC()), QString::fromUtf8(fi.fileId));
    }
}

inline char *printDbData(const FileInfo &fi)
{
    QStringList files;
    for (const auto &child : fi.children)
        addFilesDbData(files, child);
    return QTest::toString(QStringLiteral("FileInfo with %1 files(%2)").arg(files.size()).arg(files.join(QStringLiteral(", "))));
}

/**
 * @brief Utility class that count the number of GET/PUT/MOVE/DELETE operations during a sync.
 *
 * This can be used for subsequent syncs, but the counters need to be reset.
 */
struct OperationCounter
{
    int nGET = 0;
    int nPUT = 0;
    int nMOVE = 0;
    int nDELETE = 0;

    OperationCounter() { }
    OperationCounter(const OperationCounter &) = delete;
    OperationCounter(OperationCounter &&) = delete;
    void operator=(OperationCounter const &) = delete;
    void operator=(OperationCounter &&) = delete;

    void reset()
    {
        nGET = 0;
        nPUT = 0;
        nMOVE = 0;
        nDELETE = 0;
    }

    QNetworkReply *serverOverride(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *)
    {
        if (op == QNetworkAccessManager::GetOperation) {
            ++nGET;
        } else if (op == QNetworkAccessManager::PutOperation) {
            ++nPUT;
        } else if (op == QNetworkAccessManager::DeleteOperation) {
            ++nDELETE;
        } else if (req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() == QByteArrayLiteral("MOVE")) {
            ++nMOVE;
        }
        return nullptr;
    }
    auto functor()
    {
        return [this](QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device) { return serverOverride(op, req, device); };
    }

    OperationCounter(FakeFolder &fakeFolder)
    {
        fakeFolder.setServerOverride(functor());
    }

    friend inline QDebug operator<<(QDebug dbg, const OperationCounter &oc)
    {
        return dbg << "nGET:" << oc.nGET << " nPUT:" << oc.nPUT << " nMOVE:" << oc.nMOVE << " nDELETE:" << oc.nDELETE;
    }
};
