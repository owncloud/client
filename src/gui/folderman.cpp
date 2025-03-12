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

#include "folderman.h"

#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "common/asserts.h"
#include "common/depreaction.h"
#include "configfile.h"
#include "folder.h"
#include "gui/networkinformation.h"
#include "guiutility.h"
#include "libsync/syncengine.h"
#include "lockwatcher.h"
#include "scheduling/syncscheduler.h"
#include "socketapi/socketapi.h"
#include "spacesmanager.h"
#include "syncresult.h"
#include "theme.h"

#ifdef Q_OS_WIN
#include "common/utility_win.h"
#endif

#include <QMessageBox>
#include <QMutableSetIterator>
#include <QNetworkProxy>
#include <QStringLiteral>
#include <QtCore>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {
qsizetype numberOfSyncJournals(const QString &path)
{
    return QDir(path).entryList({ QStringLiteral(".sync_*.db"), QStringLiteral("._sync_*.db") }, QDir::Hidden | QDir::Files).size();
}

QString makeLegacyDbName(const OCC::FolderDefinition &def, const OCC::AccountPtr &account)
{
    // ensure https://demo.owncloud.org/ matches https://demo.owncloud.org
    // the empty path was the legacy formating before 2.9
    auto legacyUrl = account->url();
    if (legacyUrl.path() == QLatin1String("/")) {
        legacyUrl.setPath(QString());
    }
    const QString key = QStringLiteral("%1@%2:%3").arg(account->credentials()->user(), legacyUrl.toString(), def.targetPath());
    return OCC::SyncJournalDb::makeDbName(def.localPath(), QString::fromUtf8(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).left(6).toHex()));
}
}

namespace OCC {
Q_LOGGING_CATEGORY(lcFolderMan, "gui.folder.manager", QtInfoMsg)

void TrayOverallStatusResult::addResult(Folder *f)
{
    _overallStatus._numNewConflictItems += f->syncResult()._numNewConflictItems;
    _overallStatus._numErrorItems += f->syncResult()._numErrorItems;
    _overallStatus._numBlacklistErrors += f->syncResult()._numBlacklistErrors;

    auto time = f->lastSyncTime();
    if (time > lastSyncDone) {
        lastSyncDone = time;
    }

    auto status = f->syncPaused() || NetworkInformation::instance()->isBehindCaptivePortal() || NetworkInformation::instance()->isMetered()
        ? SyncResult::Paused
        : f->syncResult().status();
    if (status == SyncResult::Undefined) {
        status = SyncResult::Problem;
    }
    if (status > _overallStatus.status()) {
        _overallStatus.setStatus(status);
    }
}

const SyncResult &TrayOverallStatusResult::overallStatus() const
{
    return _overallStatus;
}

FolderMan *FolderMan::_instance = nullptr;

FolderMan::FolderMan()
    : _lockWatcher(new LockWatcher)
    , _scheduler(new SyncScheduler(this))
    , _socketApi(new SocketApi)
{
    connect(AccountManager::instance(), &AccountManager::accountRemoved,
        this, &FolderMan::slotRemoveFoldersForAccount);

    connect(_lockWatcher.data(), &LockWatcher::fileUnlocked, this, [this](const QString &path, FileSystem::LockMode) {
        if (Folder *f = folderForPath(path)) {
            // Treat this equivalently to the file being reported by the file watcher
            f->slotWatchedPathsChanged({path}, Folder::ChangeReason::UnLock);
        }
    });
}

FolderMan *FolderMan::instance()
{
    Q_ASSERT(_instance);
    return _instance;
}

FolderMan::~FolderMan()
{
    unloadAndDeleteAllFolders();
    qDeleteAll(_folders);
    _instance = nullptr;
}

const QVector<Folder *> &FolderMan::folders() const
{
    return _folders;
}

void FolderMan::unloadAndDeleteAllFolders()
{
    // save all folder definitions using a single settings instance to avoid file write/read churn
    auto settings = ConfigFile::makeQSettings();
    settings.beginGroup("Accounts");
    // clear the list of existing folders.
    const auto folders = std::move(_folders);
    for (auto *folder : folders) {
        auto strId = QString::fromUtf8(folder->definition().id());
        QString targetGroup = QStringLiteral("%1/Folders/%2").arg(folder->accountState()->account()->id(), strId);
        settings.beginGroup(targetGroup);
        FolderDefinition::save(settings, folder->definition());
        settings.endGroup();

        _socketApi->slotUnregisterPath(folder);
        folder->deleteLater();
    }
    Q_EMIT folderListChanged();
}

void FolderMan::registerFolderWithSocketApi(Folder *folder)
{
    if (!folder)
        return;
    if (!QDir(folder->path()).exists())
        return;

    // register the folder with the socket API
    if (folder->canSync())
        _socketApi->slotRegisterPath(folder);
}

std::optional<qsizetype> FolderMan::setupFoldersFromConfig()
{
    // is this really necessary? Do we actually re-set up folders at any point? when?
    unloadAndDeleteAllFolders();

    auto settings = ConfigFile::makeQSettings();
    settings.beginGroup(QStringLiteral("Accounts"));
    const auto &accountsWithSettings = settings.childGroups();

    qCInfo(lcFolderMan) << "Setup folders from settings file";

    for (const auto &account : AccountManager::instance()->accounts()) {
        // ignore the deprecation warning on account()->id() for now. It's basically a zero based index relative to
        // the account instances. It looks fragile to me but Erik said "don't touch it!" I'm guessing it would
        // require some migration step at least to switch from this id to the preferred uuid
        const auto accountId = account->account()->id();
        if (!accountsWithSettings.contains(accountId)) {
            qCWarning(lcFolderMan) << "Account id from account manager is missing from Config";
            continue;
        }

        settings.beginGroup(accountId); // Process settings for this account.

        auto processByGroup = [&](const QString &groupName) -> bool {
            settings.beginGroup(groupName);
            bool success = setupFoldersHelper(settings, account);
            settings.endGroup();
            return success;
        };

        if (!processByGroup(QStringLiteral("Folders"))) {
            return {};
        }

        // removed in 5.0
        {
            if (!processByGroup(QStringLiteral("FoldersWithPlaceholders"))) {
                return {};
            }

            // We don't save to `Multifolders` anymore, but for backwards compatibility we will just
            // read it like it is a `Folders` entry.
            if (!processByGroup(QStringLiteral("Multifolders"))) {
                return {};
            }
        }

        settings.endGroup(); // Finished processing this account.
    }

    // why not
    settings.sync();

    Q_EMIT folderListChanged();

    return _folders.size();
}

bool FolderMan::setupFoldersHelper(QSettings &settings, AccountStatePtr account)
{
    const auto &childGroups = settings.childGroups();
    for (const auto &folderAlias : childGroups) {
        settings.beginGroup(folderAlias);

        FolderDefinition folderDefinition = FolderDefinition::load(settings, folderAlias.toUtf8());
        // this should NEVER happen
        Q_ASSERT(!folderDefinition.id().isEmpty());

        // Lisa todo: there is also a spaces migration routine...investigate whether that should be done in tandem with the
        // config migration
        // also note this migration should probably be done elsewhere but for now...baby steps.
        bool migrated = migrateFolderDefinition(folderDefinition, account);

        // this can only happen when loading from config
        // does not belong in general addFolder routine
        if (SyncJournalDb::dbIsTooNewForClient(folderDefinition.absoluteJournalPath())) {
            return false;
        }

        Folder *folder = addFolder(account, folderDefinition);


        // save possible changes from the migration - a bit of ick here is that the folder may be saving changes
        // to the folder def during its internal setup, too, but this save should catch everything.
        if (migrated) {
            FolderDefinition::save(settings, folder->definition());
        }

        settings.endGroup();
    }

    return true;
}

bool FolderMan::migrateFolderDefinition(FolderDefinition &folderDefinition, AccountStatePtr account)
{
    // Lisa todo: remove ignoreHiddenFolders key if possible but remember config compatibility issues up AND down

    bool migrationPerformed = false;

    // if we would have booth the 2.9.0 file name and the lagacy file
    // with the md5 infix we prefer the 2.9.0 version
    QString defaultJournalPath;
    const QDir info(folderDefinition.localPath());
    QString defaultPath = SyncJournalDb::makeDbName(folderDefinition.localPath());
    QString legacyPath = makeLegacyDbName(folderDefinition, account->account());

    if (info.exists(defaultPath))
        defaultJournalPath = defaultPath;
    else if (info.exists(legacyPath))
        defaultJournalPath = legacyPath;
    // pre 2.6
    else {
        legacyPath.replace(QLatin1String(".sync_"), QLatin1String("._sync_"));
        if (info.exists(legacyPath))
            defaultJournalPath = legacyPath;
    }

    // Migration: Old settings don't have journalPath
    if (folderDefinition.journalPath().isEmpty()) {
        folderDefinition.setJournalPath(defaultJournalPath);
        migrationPerformed = true;
    }

    // Migration: ._ files sometimes can't be created.
    // So if the configured journalPath has a dot-underscore ("._sync_*.db")
    // but the current default doesn't have the underscore, switch to the
    // new default if no db exists yet.
    // LR - I think this should be an else if related to last if?
    if (folderDefinition.journalPath().startsWith(QLatin1String("._sync_")) && defaultJournalPath.startsWith(QLatin1String(".sync_"))
        && !QFile::exists(folderDefinition.absoluteJournalPath())) {
        folderDefinition.setJournalPath(defaultJournalPath);
        migrationPerformed = true;
    }

    // migration: 2.10 did not specify a WebDAV URL
    if (!folderDefinition.webDavUrl().isValid()) {
        folderDefinition.setWebDavUrl(account->account()->davUrl());
        migrationPerformed = true;
    }

    // Lisa tocheck: I moved this out of Folder::saveToSettings since it appears to be migration related so should be here, not there on
    // every save
    if (account->supportsSpaces() && folderDefinition.spaceId().isEmpty()) {
        OC_DISABLE_DEPRECATED_WARNING
        if (auto *space = account->account()->spacesManager()->spaceByUrl(folderDefinition.webDavUrl())) {
            OC_ENABLE_DEPRECATED_WARNING
            folderDefinition.setSpaceId(space->drive().getRoot().getId());
            migrationPerformed = true;
        }
    }

    return migrationPerformed;
}

bool FolderMan::ensureJournalGone(const QString &journalDbFile)
{
    // remove the old journal file
    while (QFile::exists(journalDbFile) && !QFile::remove(journalDbFile)) {
        qCWarning(lcFolderMan) << "Could not remove old db file at" << journalDbFile;
        int ret = QMessageBox::warning(nullptr, tr("Could not reset folder state"),
            tr("An old sync journal '%1' was found, "
               "but could not be removed. Please make sure "
               "that no application is currently using it.")
                .arg(QDir::fromNativeSeparators(QDir::cleanPath(journalDbFile))),
            QMessageBox::Retry | QMessageBox::Abort);
        if (ret == QMessageBox::Abort) {
            return false;
        }
    }
    return true;
}

bool FolderMan::ensureFilesystemSupported(const FolderDefinition &folderDefinition)
{
    if (Utility::isMac()) {
        QString filesystemType = FileSystem::fileSystemForPath(folderDefinition.localPath());
        if (filesystemType != QStringLiteral("apfs")) {
            QMessageBox::warning(nullptr, tr("Unsupported filesystem"), tr("On macOS, only the Apple File System is supported."), QMessageBox::Ok);

            return false;
        }
    }

    return true;
}

SocketApi *FolderMan::socketApi()
{
    return _socketApi.get();
}

void FolderMan::slotFolderSyncPauseChanged(Folder *f, bool paused)
{
    if (!f) {
        qCCritical(lcFolderMan) << "slotFolderSyncPaused called with empty folder";
        return;
    }

    if (!paused) {
        _disabledFolders.remove(f);
        if (f->canSync()) {
            scheduler()->enqueueFolder(f);
        }
    } else {
        _disabledFolders.insert(f);
    }

    // Lisa todo: save the folder definition here - or better, in a separate slot for clarity - not in the folder
}

void FolderMan::slotFolderCanSyncChanged()
{
    Folder *f = qobject_cast<Folder *>(sender());
    OC_ASSERT(f);
    if (f->canSync()) {
        _socketApi->slotRegisterPath(f);
    } else {
        _socketApi->slotUnregisterPath(f);
    }
}

Folder *FolderMan::folder(const QByteArray &id)
{
    if (!id.isEmpty()) {
        auto f = std::find_if(_folders.cbegin(), _folders.cend(), [id](auto f) {
            return f->id() == id;
        });
        if (f != _folders.cend()) {
            return *f;
        }
    }
    return nullptr;
}

void FolderMan::scheduleAllFolders()
{
    for (auto *f : std::as_const(_folders)) {
        if (f && f->canSync()) {
            scheduler()->enqueueFolder(f);
        }
    }
}

void FolderMan::slotSyncOnceFileUnlocks(const QString &path, FileSystem::LockMode mode)
{
    _lockWatcher->addFile(path, mode);
}

void FolderMan::slotIsConnectedChanged()
{
    AccountStatePtr accountState(qobject_cast<AccountState *>(sender()));
    if (!accountState) {
        return;
    }
    QString accountName = accountState->account()->displayNameWithHost();

    if (accountState->isConnected()) {
        qCInfo(lcFolderMan) << "Account" << accountName << "connected, scheduling its folders";

        for (auto *f : std::as_const(_folders)) {
            if (f
                && f->canSync()
                && f->accountState() == accountState) {
                scheduler()->enqueueFolder(f);
            }
        }
    } else {
        qCInfo(lcFolderMan) << "Account" << accountName << "disconnected or paused, "
                                                           "terminating or descheduling sync folders";

        for (auto *f : std::as_const(_folders)) {
            if (f
                && f->isSyncRunning()
                && f->accountState() == accountState) {
                f->slotTerminateSync(tr("Account disconnected or paused"));
            }
        }
    }
}

// only enable or disable foldermans will schedule and do syncs.
// this is not the same as Pause and Resume of folders.
void FolderMan::setSyncEnabled(bool enabled)
{
    if (enabled) {
        // We have things in our queue that were waiting for the connection to come back on.
        scheduler()->start();
    } else {
        scheduler()->stop();
    }
    qCInfo(lcFolderMan) << Q_FUNC_INFO << enabled;
    // force a redraw in case the network connect status changed
    Q_EMIT folderSyncStateChange(nullptr);
}

void FolderMan::slotRemoveFoldersForAccount(const AccountStatePtr &accountState)
{
    QList<Folder *> foldersToRemove;
    // reserve a magic number
    // Lisa todo: folder management would likely be a lot simpler and more efficient if we kept
    // the folders in a hash or similar to make folder lookups by id simpler.
    // We could solve the problem below by simply keeping a list of ids per account
    // this magic number thing is not healthy
    foldersToRemove.reserve(16);
    for (auto *folder : std::as_const(_folders)) {
        if (folder->accountState() == accountState) {
            foldersToRemove.append(folder);
        }
    }
    for (const auto &f : foldersToRemove) {
        removeFolderSync(f);
    }
}

void FolderMan::slotServerVersionChanged(Account *account)
{
    // Pause folders if the server version is unsupported
    if (account->serverSupportLevel() == Account::ServerSupportLevel::Unsupported) {
        qCWarning(lcFolderMan) << "The server version is unsupported:" << account->capabilities().status().versionString()
                               << "pausing all folders on the account";

        for (auto &f : std::as_const(_folders)) {
            if (f->accountState()->account().data() == account) {
                f->setSyncPaused(true);
            }
        }
    }
}

bool FolderMan::isAnySyncRunning() const
{
    if (_scheduler->hasCurrentRunningSyncRunning()) {
        return true;
    }

    for (auto f : _folders) {
        if (f->isSyncRunning())
            return true;
    }
    return false;
}

void FolderMan::slotFolderSyncStarted()
{
    auto f = qobject_cast<Folder *>(sender());
    OC_ASSERT(f);
    if (!f)
        return;

    qCInfo(lcFolderMan) << ">========== Sync started for folder [" << f->shortGuiLocalPath() << "] of account ["
                        << f->accountState()->account()->displayNameWithHost() << "] with remote [" << f->remoteUrl().toDisplayString() << "]";
}

/*
  * a folder indicates that its syncing is finished.
  * Start the next sync after the system had some milliseconds to breath.
  * This delay is particularly useful to avoid late file change notifications
  * (that we caused ourselves by syncing) from triggering another spurious sync.
  */
void FolderMan::slotFolderSyncFinished(const SyncResult &)
{
    auto f = qobject_cast<Folder *>(sender());
    OC_ASSERT(f);
    if (!f)
        return;

    qCInfo(lcFolderMan) << "<========== Sync finished for folder [" << f->shortGuiLocalPath() << "] of account ["
                        << f->accountState()->account()->displayNameWithHost() << "] with remote [" << f->remoteUrl().toDisplayString() << "]";
}

bool FolderMan::validateFolderDefinition(const FolderDefinition &folderDefinition)
{
    if (folderDefinition.id().isEmpty() || folderDefinition.journalPath().isEmpty() || !ensureFilesystemSupported(folderDefinition))
        return false;
    return true;
}


Folder *FolderMan::addFolder(const AccountStatePtr &accountState, const FolderDefinition &folderDefinition)
{
    if (Folder *f = folder(folderDefinition.id())) {
        Q_ASSERT_X(false, "addFolder", "Trying to addFolder but id is already found in the folder list");
        qCWarning(lcFolderMan) << "Trying to add folder" << folderDefinition.localPath() << "but it already exists in folder list";
        return f; // or return nullptr - talk to Erik
    }

    if (!validateFolderDefinition(folderDefinition)) {
        qCWarning(lcFolderMan) << "Folder Defnition validation failed for folder" << folderDefinition.localPath();
        return nullptr;
    }

    auto vfs = VfsPluginManager::instance().createVfsFromPlugin(folderDefinition.virtualFilesMode());
    if (!vfs) {
        qCWarning(lcFolderMan) << "Could not load plugin for mode" << folderDefinition.virtualFilesMode();
        return nullptr;
    }

    auto folder = new Folder(folderDefinition, accountState, std::move(vfs), this);

    qCInfo(lcFolderMan) << "Adding folder to Folder Map " << folder << folder->path();
    // always add the folder even if it had a setup error - future add special handling for incomplete folders if possible
    _folders.push_back(folder);
    if (folder->syncPaused()) {
        _disabledFolders.insert(folder);
    }

    if (!folder->hasSetupError()) {
        connectFolder(folder);
        Q_EMIT folderSyncStateChange(folder);
    }

    return folder;
}

void FolderMan::connectFolder(Folder *folder)
{
    // See matching disconnects in disconnectFolder().
    if (folder && !folder->hasSetupError()) {
        connect(folder, &Folder::syncStateChange, _socketApi.get(), [folder, this] { _socketApi->slotUpdateFolderView(folder); });
        connect(folder, &Folder::syncStarted, this, &FolderMan::slotFolderSyncStarted);
        connect(folder, &Folder::syncFinished, this, &FolderMan::slotFolderSyncFinished);
        connect(folder, &Folder::syncStateChange, this, [folder, this] { Q_EMIT folderSyncStateChange(folder); });
        connect(folder, &Folder::syncPausedChanged, this, &FolderMan::slotFolderSyncPauseChanged);
        connect(folder, &Folder::canSyncChanged, this, &FolderMan::slotFolderCanSyncChanged);
        connect(
            &folder->syncEngine().syncFileStatusTracker(), &SyncFileStatusTracker::fileStatusChanged, _socketApi.get(), &SocketApi::broadcastStatusPushMessage);
        connect(folder, &Folder::watchedFileChangedExternally, &folder->syncEngine().syncFileStatusTracker(), &SyncFileStatusTracker::slotPathTouched);

        registerFolderWithSocketApi(folder);
    }
}

void FolderMan::disconnectFolder(Folder *folder)
{
    if (folder && !folder->hasSetupError()) {
        _socketApi->slotUnregisterPath(folder);

        disconnect(folder, nullptr, _socketApi.get(), nullptr);
        disconnect(folder, nullptr, this, nullptr);
        disconnect(&folder->syncEngine(), nullptr, folder, nullptr);
        disconnect(
            &folder->syncEngine().syncFileStatusTracker(), &SyncFileStatusTracker::fileStatusChanged, _socketApi.get(), &SocketApi::broadcastStatusPushMessage);
        disconnect(folder, nullptr, &folder->syncEngine().syncFileStatusTracker(), nullptr);
    }
}

Folder *FolderMan::folderForPath(const QString &path, QString *relativePath)
{
    QString absolutePath = QDir::cleanPath(path) + QLatin1Char('/');

    for (auto *folder : std::as_const(_folders)) {
        const QString folderPath = folder->cleanPath() + QLatin1Char('/');

        if (absolutePath.startsWith(folderPath, (Utility::isWindows() || Utility::isMac()) ? Qt::CaseInsensitive : Qt::CaseSensitive)) {
            if (relativePath) {
                *relativePath = absolutePath.mid(folderPath.length());
                relativePath->chop(1); // we added a '/' above
            }
            return folder;
        }
    }

    if (relativePath)
        relativePath->clear();
    return nullptr;
}

QStringList FolderMan::findFileInLocalFolders(const QString &relPath, const AccountPtr acc)
{
    QStringList re;

    // We'll be comparing against Folder::remotePath which always starts with /
    QString serverPath = relPath;
    if (!serverPath.startsWith(QLatin1Char('/')))
        serverPath.prepend(QLatin1Char('/'));

    for (auto *folder : std::as_const(_folders)) {
        if (acc != nullptr && folder->accountState()->account() != acc) {
            continue;
        }
        if (!serverPath.startsWith(folder->remotePath()))
            continue;

        QString path = folder->cleanPath() + QLatin1Char('/');
        path += serverPath.mid(folder->remotePathTrailingSlash().length());
        if (QFile::exists(path)) {
            re.append(path);
        }
    }
    return re;
}

void FolderMan::removeFolderSync(Folder *f)
{
    if (!OC_ENSURE(f)) {
        return;
    }

    qCInfo(lcFolderMan) << "Removing " << f->path();

    const bool currentlyRunning = f->isSyncRunning();
    if (currentlyRunning) {
        // abort the sync now
        f->slotTerminateSync(tr("Folder is about to be removed"));
    }

    f->setSyncPaused(true);
    f->wipeForRemoval();

    // remove the folder configuration
    f->removeFromSettings();

    // Lisa todo: propose adding the call to remove the folder db here, not in addFolder

    // highly suspicious - how can there be more than one instance?!
    _folders.removeAll(f);

    disconnectFolder(f);

    Q_EMIT folderRemoved(f);
    Q_EMIT folderListChanged();

    f->deleteLater();
}

QString FolderMan::getBackupName(QString fullPathName) const
{
    if (fullPathName.endsWith(QLatin1String("/")))
        fullPathName.chop(1);

    if (fullPathName.isEmpty())
        return QString();

    QString newName = fullPathName + tr(" (backup)");
    QFileInfo fi(newName);
    int cnt = 2;
    do {
        if (fi.exists()) {
            newName = fullPathName + tr(" (backup %1)").arg(cnt++);
            fi.setFile(newName);
        }
    } while (fi.exists());

    return newName;
}

void FolderMan::setDirtyProxy()
{
    for (auto *f : std::as_const(_folders)) {
        if (f) {
            if (f->accountState() && f->accountState()->account()
                && f->accountState()->account()->accessManager()) {
                // Need to do this so we do not use the old determined system proxy
                f->accountState()->account()->accessManager()->setProxy(
                    QNetworkProxy(QNetworkProxy::DefaultProxy));
            }
        }
    }
}

void FolderMan::setDirtyNetworkLimits()
{
    for (auto *f : std::as_const(_folders)) {
        // set only in busy folders. Otherwise they read the config anyway.
        if (f && f->isSyncRunning()) {
            f->setDirtyNetworkLimits();
        }
    }
}

TrayOverallStatusResult FolderMan::trayOverallStatus(const QVector<Folder *> &folders)
{
    TrayOverallStatusResult result;

    // if one of them has an error -> show error
    // if one is paused, but others ok, show ok
    //
    for (auto *folder : folders) {
        result.addResult(folder);
    }
    return result;
}

QString FolderMan::trayTooltipStatusString(
    const SyncResult &result, bool paused)
{
    QString folderMessage;
    switch (result.status()) {
    case SyncResult::Success:
        [[fallthrough]];
    case SyncResult::Problem:
        if (result.hasUnresolvedConflicts()) {
            folderMessage = tr("Sync was successful, unresolved conflicts.");
            break;
        }
        [[fallthrough]];
    default:
        return Utility::enumToDisplayName(result.status());
    }
    if (paused) {
        // sync is disabled.
        folderMessage = tr("%1 (Sync is paused)").arg(folderMessage);
    }
    return folderMessage;
}

// QFileInfo::canonicalPath returns an empty string if the file does not exist.
// This function also works with files that does not exist and resolve the symlinks in the
// parent directories.
static QString canonicalPath(const QString &path)
{
    QFileInfo selFile(path);
    if (!selFile.exists()) {
        const auto parentPath = selFile.dir().path();

        // It's possible for the parentPath to match the path
        // (possibly we've arrived at a non-existant drive root on Windows)
        // and recursing would be fatal.
        if (parentPath == path) {
            return path;
        }

        return canonicalPath(parentPath) + QLatin1Char('/') + selFile.fileName();
    }
    return selFile.canonicalFilePath();
}

static QString checkPathForSyncRootMarkingRecursive(const QString &path, FolderMan::NewFolderType folderType, const QUuid &accountUuid)
{
    std::pair<QString, QUuid> existingTags = Utility::getDirectorySyncRootMarkings(path);
    if (!existingTags.first.isEmpty()) {
        if (existingTags.first != Theme::instance()->orgDomainName()) {
            // another application uses this as spaces root folder
            return FolderMan::tr("Folder '%1' is already in use by application %2!").arg(path, existingTags.first);
        }

        // Looks good, it's our app, let's check the account tag:
        switch (folderType) {
        case FolderMan::NewFolderType::SpacesFolder:
            if (existingTags.second == accountUuid) {
                // Nice, that's what we like, the sync root for our account in our app. No error.
                return {};
            }
            [[fallthrough]];
        case FolderMan::NewFolderType::OC10SyncRoot:
            [[fallthrough]];
        case FolderMan::NewFolderType::SpacesSyncRoot:
            // It's our application but we don't want to create a spaces folder, so it must be another space root
            return FolderMan::tr("Folder '%1' is already in use by another account.").arg(path);
        }
    }

    QString parent = QFileInfo(path).path();
    if (parent == path) { // root dir, stop recursing
        return {};
    }

    return checkPathForSyncRootMarkingRecursive(parent, folderType, accountUuid);
}

QString FolderMan::checkPathValidityRecursive(const QString &path, FolderMan::NewFolderType folderType, const QUuid &accountUuid)
{
    if (path.isEmpty()) {
        return FolderMan::tr("No valid folder selected!");
    }

#ifdef Q_OS_WIN
    Utility::NtfsPermissionLookupRAII ntfs_perm;
#endif

    auto pathLenghtCheck = Folder::checkPathLength(path);
    if (!pathLenghtCheck) {
        return pathLenghtCheck.error();
    }

    const QFileInfo selectedPathInfo(path);
    if (!selectedPathInfo.exists()) {
        const QString parentPath = selectedPathInfo.path();
        if (parentPath != path) {
            return checkPathValidityRecursive(parentPath, folderType, accountUuid);
        }
        return FolderMan::tr("The selected path does not exist!");
    }

    if (numberOfSyncJournals(selectedPathInfo.filePath()) != 0) {
        return FolderMan::tr("The folder %1 is used in a folder sync connection!").arg(QDir::toNativeSeparators(selectedPathInfo.filePath()));
    }

    // At this point we know there is no syncdb in the parent hyrarchy, check for spaces sync root.

    if (!selectedPathInfo.isDir()) {
        return FolderMan::tr("The selected path is not a folder!");
    }

    if (!selectedPathInfo.isWritable()) {
        return FolderMan::tr("You have no permission to write to the selected folder!");
    }

    return checkPathForSyncRootMarkingRecursive(path, folderType, accountUuid);
}

/*
 * OC10 folder:
 *  - sync root not in syncdb folder
 *  - sync root not in spaces root
 * with spaces:
 *  - spaces sync root not in syncdb folder
 *  - spaces sync root not in another spaces sync root
 *
 *  - space not in syncdb folder
 *  - space *can* be in sync root
 *  - space not in spaces sync root of other account (check with account uuid)
 */
QString FolderMan::checkPathValidityForNewFolder(const QString &path, NewFolderType folderType, const QUuid &accountUuid) const
{
    // check if the local directory isn't used yet in another ownCloud sync
    const auto cs = Utility::fsCaseSensitivity();

    const QString userDir = QDir::cleanPath(canonicalPath(path)) + QLatin1Char('/');
    for (auto f : _folders) {
        const QString folderDir = QDir::cleanPath(canonicalPath(f->path())) + QLatin1Char('/');

        if (QString::compare(folderDir, userDir, cs) == 0) {
            return tr("There is already a sync from the server to this local folder. "
                      "Please pick another local folder!");
        }
        if (FileSystem::isChildPathOf(folderDir, userDir)) {
            return tr("The local folder %1 already contains a folder used in a folder sync connection. "
                      "Please pick another local folder!")
                .arg(QDir::toNativeSeparators(path));
        }

        if (FileSystem::isChildPathOf(userDir, folderDir)) {
            return tr("The local folder %1 is already contained in a folder used in a folder sync connection. "
                      "Please pick another local folder!")
                .arg(QDir::toNativeSeparators(path));
        }
    }

    const auto result = checkPathValidityRecursive(path, folderType, accountUuid);
    if (!result.isEmpty()) {
        return tr("%1 Please pick another local folder!").arg(result);
    }
    return {};
}

QString FolderMan::findGoodPathForNewSyncFolder(
    const QString &basePath, const QString &newFolder, FolderMan::NewFolderType folderType, const QUuid &accountUuid)
{
    OC_ASSERT(!accountUuid.isNull() || folderType == FolderMan::NewFolderType::SpacesSyncRoot);

    // reserve extra characters to allow appending of a number
    const QString normalisedPath = FileSystem::createPortableFileName(basePath, FileSystem::pathEscape(newFolder), std::string_view(" (100)").size());

    // If the parent folder is a sync folder or contained in one, we can't
    // possibly find a valid sync folder inside it.
    // Example: Someone syncs their home directory. Then ~/foobar is not
    // going to be an acceptable sync folder path for any value of foobar.
    if (FolderMan::instance()->folderForPath(QFileInfo(normalisedPath).canonicalPath())) {
        // Any path with that parent is going to be unacceptable,
        // so just keep it as-is.
        return canonicalPath(normalisedPath);
    }
    // Count attempts and give up eventually
    {
        QString folder = normalisedPath;
        for (int attempt = 2; attempt <= 100; ++attempt) {
            if (!QFileInfo::exists(folder) && FolderMan::instance()->checkPathValidityForNewFolder(folder, folderType, accountUuid).isEmpty()) {
                return canonicalPath(folder);
            }
            folder = normalisedPath + QStringLiteral(" (%1)").arg(attempt);
        }
    }
    // we failed to find a non existing path
    return canonicalPath(normalisedPath);
}

bool FolderMan::ignoreHiddenFiles() const
{
    if (_folders.empty()) {
        return true;
    }
    // Lisa todo: make this a var on FolderMan
    return _folders.first()->ignoreHiddenFiles();
}

void FolderMan::setIgnoreHiddenFiles(bool ignore)
{
    // Lisa TODO: this is crazy - it's a global setting so create a member for folder manager, and save it ONCE, not on each folder.
    // Note that the setting will revert to 'true' if all folders
    // are deleted...
    for (auto *folder : std::as_const(_folders)) {
        folder->setIgnoreHiddenFiles(ignore);
        folder->saveToSettings();
    }
}

Result<void, QString> FolderMan::unsupportedConfiguration(const QString &path) const
{
    auto it = _unsupportedConfigurationError.find(path);
    if (it == _unsupportedConfigurationError.end()) {
        it = _unsupportedConfigurationError.insert(path, [&]() -> Result<void, QString> {
            if (numberOfSyncJournals(path) > 1) {
                const QString error = tr("Multiple accounts are sharing the folder %1.\n"
                                         "This configuration is know to lead to dataloss and is no longer supported.\n"
                                         "Please consider removing this folder from the account and adding it again.")
                                          .arg(path);
                if (Theme::instance()->warnOnMultipleDb()) {
                    qCWarning(lcFolderMan) << error;
                    return error;
                } else {
                    qCWarning(lcFolderMan) << error << "this error is not displayed to the user as this is a branded"
                                           << "client and the error itself might be a false positive caused by a previous broken migration";
                }
            }
            return {};
        }());
    }
    return *it;
}

bool FolderMan::isSpaceSynced(GraphApi::Space *space) const
{
    auto it = std::find_if(_folders.cbegin(), _folders.cend(), [space](auto f) { return f->space() == space; });
    return it != _folders.cend();
}

void FolderMan::slotReloadSyncOptions()
{
    for (auto *f : std::as_const(_folders)) {
        if (f) {
            f->reloadSyncOptions();
        }
    }
}

bool FolderMan::checkVfsAvailability(const QString &path, Vfs::Mode mode) const
{
    return unsupportedConfiguration(path) && Vfs::checkAvailability(path, mode);
}

// Lisa todo: rename this - it obvs has no dependency on any wizard, it's just for creating a new folder on demand.
Folder *FolderMan::addFolderFromWizard(const AccountStatePtr &accountStatePtr, FolderDefinition &&folderDefinition, bool useVfs)
{
    if (!FolderMan::prepareFolder(folderDefinition.localPath())) {
        return nullptr;
    }

    folderDefinition.setIgnoreHiddenFiles(ignoreHiddenFiles());
    folderDefinition.setJournalPath(SyncJournalDb::makeDbName(folderDefinition.localPath()));

    // Lisa todo: why are we doing this check when adding a folder?
    // should it not happen on removing the sync? Yes it should
    // should the journal be removed even when loading from config? Not usually, but maybe if there was a migration?
    // sticky part is old clients may not remove the old journal so we at least need a check, but ELSEWHERE
    //
    if (!ensureJournalGone(folderDefinition.absoluteJournalPath())) {
        return nullptr;
    }

    if (useVfs) {
        folderDefinition.setVirtualFilesMode(VfsPluginManager::instance().bestAvailableVfsMode());
    }

    auto newFolder = addFolder(accountStatePtr, folderDefinition);

    if (newFolder) {
        // With spaces we only handle the main folder
        if (!newFolder->groupInSidebar()) {
            Utility::setupFavLink(folderDefinition.localPath());
        }
        qCDebug(lcFolderMan) << "Local sync folder" << folderDefinition.localPath() << "successfully created!";
        newFolder->saveToSettings();
    } else {
        qCWarning(lcFolderMan) << "Failed to create local sync folder!";
    }

    // find best location with updates
    newFolder->saveToSettings();

    // should be moved to true originating caller
    Q_EMIT folderListChanged();

    return newFolder;
}

Folder *FolderMan::addFolderFromFolderWizardResult(const AccountStatePtr &accountStatePtr, const SyncConnectionDescription &description)
{
    FolderDefinition definition = FolderDefinition::createNewFolderDefinition(description.davUrl, description.spaceId, description.displayName);
    definition.setLocalPath(description.localPath);
    definition.setTargetPath(description.remotePath);
    auto f = addFolderFromWizard(accountStatePtr, std::move(definition), description.useVirtualFiles);
    // this should be moved to addFolderFromWizard
    if (f) {
        f->journalDb()->setSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, description.selectiveSyncBlackList);
        f->setPriority(description.priority);
        f->saveToSettings();
    }
    return f;
}

QString FolderMan::suggestSyncFolder(NewFolderType folderType, const QUuid &accountUuid)
{
    return FolderMan::instance()->findGoodPathForNewSyncFolder(QDir::homePath(), Theme::instance()->appName(), folderType, accountUuid);
}

bool FolderMan::prepareFolder(const QString &folder)
{
    if (!QFileInfo::exists(folder)) {
        if (!OC_ENSURE(QDir().mkpath(folder))) {
            return false;
        }
        // Lisa todo: consider renaming these or maybe better, merge them into one function that handles "prepareFolder" in full.
        // it appears these are always called together so to avoid errors, just roll it into one routine
        // this is for mac
        FileSystem::setFolderMinimumPermissions(folder);
        // this is for windows - it sets up a desktop.ini file and deals with persmissions.
        Folder::prepareFolder(folder);
    }
    return true;
}

std::unique_ptr<FolderMan> FolderMan::createInstance()
{
    OC_ASSERT(!_instance);
    _instance = new FolderMan();
    return std::unique_ptr<FolderMan>(_instance);
}

} // namespace OCC
