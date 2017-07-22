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
#include "config.h"

#include "account.h"
#include "accountstate.h"
#include "folderdefinition.h"
#include "folder.h"
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

Q_LOGGING_CATEGORY(lcFolder, "gui.folder", QtInfoMsg)

Folder::Folder(const FolderDefinition &definition,
    AccountState *accountState,
    QObject *parent)
    : AbstractFolder(definition, accountState, parent)
    , _csyncUnavail(false)
{
    // FIXME: Engine is initialized in the AbstractFolder constructor.
    // Change that to have a method initializeSyncEngine(SyncEngine) in the AbstractFolder
    // that needs to be called by the Folder class, to let the Folder instanciate its own
    // SyncEngine. For that, we need an Abstract Sync Engine as well.
    _engine->setIgnoreHiddenFiles(definition.ignoreHiddenFiles);

    if (!setIgnoredFiles()) {
        qCWarning(lcFolder, "Could not read system exclude file");
    }

    connect(accountState, SIGNAL(isConnectedChanged()), this, SIGNAL(canSyncChanged()));
    connect(_engine.data(), SIGNAL(rootEtag(QString)), this, SLOT(etagRetreivedFromSyncEngine(QString)));

    connect(_engine.data(), SIGNAL(csyncUnavailable()), SLOT(slotCsyncUnavailable()), Qt::QueuedConnection);

    //direct connection so the message box is blocking the sync.
    connect(_engine.data(), SIGNAL(aboutToRemoveAllFiles(SyncFileItem::Direction, bool *)),
            SLOT(slotAboutToRemoveAllFiles(SyncFileItem::Direction, bool *)));
    connect(_engine.data(), SIGNAL(aboutToRestoreBackup(bool *)),
            SLOT(slotAboutToRestoreBackup(bool *)));
    connect(_engine.data(), SIGNAL(newBigFolder(QString, bool)),
            this, SLOT(slotNewBigFolderDiscovered(QString, bool)));
}

Folder::~Folder()
{

}

bool Folder::ignoreHiddenFiles()
{
    bool re(_definition.ignoreHiddenFiles);
    return re;
}

void Folder::setIgnoreHiddenFiles(bool ignore)
{
    _definition.ignoreHiddenFiles = ignore;
}

void Folder::slotRunEtagJob()
{
    qCInfo(lcFolder) << "Trying to check" << remoteUrl().toString() << "for changes via ETag check.";

    AccountPtr account = _accountState->account();

    if (_requestEtagJob) {
        qCInfo(lcFolder) << remoteUrl().toString() << "has ETag job queued, not trying to sync";
        return;
    }

    if (!canSync()) {
        qCInfo(lcFolder) << "Not syncing.  :" << remoteUrl().toString() << _definition.paused << AccountState::stateString(_accountState->state());
        return;
    }

    // Do the ordinary etag check for the root folder and schedule a
    // sync if it's different.

    _requestEtagJob = new RequestEtagJob(account, remotePath(), this);
    _requestEtagJob->setTimeout(60 * 1000);
    // check if the etag is different when retrieved
    QObject::connect(_requestEtagJob, SIGNAL(etagRetreived(QString)), this, SLOT(etagRetreived(QString)));
    FolderMan::instance()->slotScheduleETagJob(alias(), _requestEtagJob);
    // The _requestEtagJob is auto deleting itself on finish. Our guard pointer _requestEtagJob will then be null.
}

void Folder::etagRetreived(const QString &etag)
{
    // re-enable sync if it was disabled because network was down
    FolderMan::instance()->setSyncEnabled(true);

    if (_lastEtag != etag) {
        qCInfo(lcFolder) << "Compare etag with previous etag: last:" << _lastEtag << ", received:" << etag << "-> CHANGED";
        _lastEtag = etag;
        slotScheduleThisFolder();
    }

    _accountState->tagLastSuccessfullETagRequest();
}

void Folder::etagRetreivedFromSyncEngine(const QString &etag)
{
    qCInfo(lcFolder) << "Root etag from during sync:" << etag;
    accountState()->tagLastSuccessfullETagRequest();
    _lastEtag = etag;
}

bool Folder::isFileExcludedRelative(const QString &relativePath) const
{
    return _engine->excludedFiles().isExcluded(path() + relativePath, path(), _definition.ignoreHiddenFiles);
}

bool Folder::isFileExcludedAbsolute(const QString& fullPath) const
{
    return _engine->excludedFiles().isExcluded(fullPath, path(), _definition.ignoreHiddenFiles);
}

bool Folder::setIgnoredFiles()
{
    // Note: Doing this on each sync run and on Folder construction is
    // unnecessary, because _engine->excludedFiles() persists between
    // sync runs. This is not a big problem because ExcludedFiles maintains
    // a QSet of files to load.
    ConfigFile cfg;
    QString systemList = cfg.excludeFile(ConfigFile::SystemScope);
    qCInfo(lcFolder) << "Adding system ignore list to csync:" << systemList;
    _engine->excludedFiles().addExcludeFilePath(systemList);

    QString userList = cfg.excludeFile(ConfigFile::UserScope);
    if (QFile::exists(userList)) {
        qCInfo(lcFolder) << "Adding user defined ignore list to csync:" << userList;
        _engine->excludedFiles().addExcludeFilePath(userList);
    }

    return _engine->excludedFiles().reloadExcludes();
}


void Folder::slotCsyncUnavailable()
{
    _csyncUnavail = true;
}

void Folder::slotNewBigFolderDiscovered(const QString &newF, bool isExternal)
{
    auto newFolder = newF;
    if (!newFolder.endsWith(QLatin1Char('/'))) {
        newFolder += QLatin1Char('/');
    }
    auto journal = journalDb();

    // Add the entry to the blacklist if it is neither in the blacklist or whitelist already
    bool ok1, ok2;
    auto blacklist = journal->getSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, &ok1);
    auto whitelist = journal->getSelectiveSyncList(SyncJournalDb::SelectiveSyncWhiteList, &ok2);
    if (ok1 && ok2 && !blacklist.contains(newFolder) && !whitelist.contains(newFolder)) {
        blacklist.append(newFolder);
        journal->setSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, blacklist);
    }

    // And add the entry to the undecided list and signal the UI
    auto undecidedList = journal->getSelectiveSyncList(SyncJournalDb::SelectiveSyncUndecidedList, &ok1);
    if (ok1) {
        if (!undecidedList.contains(newFolder)) {
            undecidedList.append(newFolder);
            journal->setSelectiveSyncList(SyncJournalDb::SelectiveSyncUndecidedList, undecidedList);
            emit newBigFolderDiscovered(newFolder);
        }
        QString message = !isExternal ? (tr("A new folder larger than %1 MB has been added: %2.\n")
                                                .arg(ConfigFile().newBigFolderSizeLimit().second)
                                                .arg(newF))
                                      : (tr("A folder from an external storage has been added.\n"));
        message += tr("Please go in the settings to select it if you wish to download it.");

        auto logger = Logger::instance();
        logger->postOptionalGuiLog(Theme::instance()->appNameGUI(), message);
    }
}

void Folder::slotAboutToRemoveAllFiles(SyncFileItem::Direction dir, bool *cancel)
{
    ConfigFile cfgFile;
    if (!cfgFile.promptDeleteFiles())
        return;

    QString msg = dir == SyncFileItem::Down ? tr("All files in the sync folder '%1' folder were deleted on the server.\n"
                                                 "These deletes will be synchronized to your local sync folder, making such files "
                                                 "unavailable unless you have a right to restore. \n"
                                                 "If you decide to keep the files, they will be re-synced with the server if you have rights to do so.\n"
                                                 "If you decide to delete the files, they will be unavailable to you, unless you are the owner.")
                                            : tr("All the files in your local sync folder '%1' were deleted. These deletes will be "
                                                 "synchronized with your server, making such files unavailable unless restored.\n"
                                                 "Are you sure you want to sync those actions with the server?\n"
                                                 "If this was an accident and you decide to keep your files, they will be re-synced from the server.");
    QMessageBox msgBox(QMessageBox::Warning, tr("Remove All Files?"),
        msg.arg(shortGuiLocalPath()));
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
    msgBox.addButton(tr("Remove all files"), QMessageBox::DestructiveRole);
    QPushButton *keepBtn = msgBox.addButton(tr("Keep files"), QMessageBox::AcceptRole);
    if (msgBox.exec() == -1) {
        *cancel = true;
        return;
    }
    *cancel = msgBox.clickedButton() == keepBtn;
    if (*cancel) {
        FileSystem::setFolderMinimumPermissions(path());
        journalDb()->clearFileTable();
        _lastEtag.clear();
        slotScheduleThisFolder();
    }
}

void Folder::slotAboutToRestoreBackup(bool *restore)
{
    QString msg =
        tr("This sync would reset the files to an earlier time in the sync folder '%1'.\n"
           "This might be because a backup was restored on the server.\n"
           "Continuing the sync as normal will cause all your files to be overwritten by an older "
           "file in an earlier state. "
           "Do you want to keep your local most recent files as conflict files?");
    QMessageBox msgBox(QMessageBox::Warning, tr("Backup detected"),
        msg.arg(shortGuiLocalPath()));
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
    msgBox.addButton(tr("Normal Synchronisation"), QMessageBox::DestructiveRole);
    QPushButton *keepBtn = msgBox.addButton(tr("Keep Local Files as Conflict"), QMessageBox::AcceptRole);

    if (msgBox.exec() == -1) {
        *restore = true;
        return;
    }
    *restore = msgBox.clickedButton() == keepBtn;
}

void Folder::startSync(const QStringList &pathList){

    _csyncUnavail = false;
    AbstractFolder::startSync(pathList);
}

} // namespace OCC
