/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
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

#include "folderstatusmodel.h"
#include "account.h"
#include "accountstate.h"
#include "folderman.h"
#include "gui/networkinformation.h"
#include "gui/quotainfo.h"
#include "theme.h"

#include "libsync/graphapi/space.h"
#include "libsync/graphapi/spacesmanager.h"

#include <QApplication>
#include <QFileIconProvider>
#include <QRandomGenerator>

#include <set>

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(QPersistentModelIndex)

namespace OCC {

Q_LOGGING_CATEGORY(lcFolderStatus, "gui.folder.model", QtInfoMsg)

namespace {
    // minimum delay between progress updates
    constexpr auto progressUpdateTimeOutC = 1s;

    QString statusIconName(Folder *f)
    {
        auto status = f->syncResult();
        if (!f->accountState()->isConnected()) {
            status.setStatus(SyncResult::Status::Offline);
        } else if (f->syncPaused() || NetworkInformation::instance()->isBehindCaptivePortal() || NetworkInformation::instance()->isMetered()) {
            status.setStatus(SyncResult::Status::Paused);
        }
        return QStringLiteral("states/%1").arg(Theme::instance()->syncStateIconName(status));
    }

    class SubFolderInfo
    {
    public:
        class Progress
        {
        public:
            Progress() { }
            bool isNull() const { return _progressString.isEmpty() && _overallSyncString.isEmpty(); }
            QString _progressString;
            QString _overallSyncString;
            float _overallPercent = 0;
        };
        SubFolderInfo(Folder *folder)
            : _folder(folder)
        {
        }
        Folder *const _folder;

        Progress _progress;

        std::chrono::steady_clock::time_point _lastProgressUpdated = std::chrono::steady_clock::now();
        ProgressInfo::Status _lastProgressUpdateStatus = ProgressInfo::None;
    };

    void computeProgress(const ProgressInfo &progress, SubFolderInfo::Progress *pi)
    {
        // find the single item to display:  This is going to be the bigger item, or the last completed
        // item if no items are in progress.
        SyncFileItem curItem = progress._lastCompletedItem;
        qint64 curItemProgress = -1; // -1 means finished
        qint64 biggerItemSize = 0;
        quint64 estimatedUpBw = 0;
        quint64 estimatedDownBw = 0;
        QStringList allFilenames;
        for (const auto &citm : progress._currentItems) {
            if (curItemProgress == -1 || (ProgressInfo::isSizeDependent(citm._item) && biggerItemSize < citm._item._size)) {
                curItemProgress = citm._progress.completed();
                curItem = citm._item;
                biggerItemSize = citm._item._size;
            }
            if (citm._item._direction != SyncFileItem::Up) {
                estimatedDownBw += progress.fileProgress(citm._item).estimatedBandwidth;
            } else {
                estimatedUpBw += progress.fileProgress(citm._item).estimatedBandwidth;
            }
            allFilenames.append(QApplication::translate("FolderStatus", "'%1'").arg(citm._item._file));
        }
        if (curItemProgress == -1) {
            curItemProgress = curItem._size;
        }

        const QString itemFileName = curItem._file;
        const QString kindString = Progress::asActionString(curItem);

        QString fileProgressString;
        if (ProgressInfo::isSizeDependent(curItem)) {
            // quint64 estimatedBw = progress.fileProgress(curItem).estimatedBandwidth;
            if (estimatedUpBw || estimatedDownBw) {
                /*
                //: Example text: "uploading foobar.png (1MB of 2MB) time left 2 minutes at a rate of 24Kb/s"
                fileProgressString = tr("%1 %2 (%3 of %4) %5 left at a rate of %6/s")
                    .arg(kindString, itemFileName, s1, s2,
                        Utility::durationToDescriptiveString(progress.fileProgress(curItem).estimatedEta),
                        Utility::octetsToString(estimatedBw) );
                */
                //: Example text: "Syncing 'foo.txt', 'bar.txt'"
                fileProgressString = QApplication::translate("FolderStatus", "Syncing %1").arg(allFilenames.join(QStringLiteral(", ")));
                if (estimatedDownBw > 0) {
                    fileProgressString.append(QApplication::translate("FolderStatus", ", ⬇️ %1/s").arg(Utility::octetsToString(estimatedDownBw)));
                }
                if (estimatedUpBw > 0) {
                    fileProgressString.append(QApplication::translate("FolderStatus", ", ⬆️ %1/s").arg(Utility::octetsToString(estimatedUpBw)));
                }
            } else {
                //: Example text: "uploading foobar.png (2MB of 2MB)"
                fileProgressString = QApplication::translate("FolderStatus", "%1 %2 (%3 of %4)")
                                         .arg(kindString, itemFileName, Utility::octetsToString(curItemProgress), Utility::octetsToString(curItem._size));
            }
        } else if (!kindString.isEmpty()) {
            //: Example text: "uploading foobar.png"
            fileProgressString = QApplication::translate("FolderStatus", "%1 %2").arg(kindString, itemFileName);
        }
        pi->_progressString = fileProgressString;

        // overall progress
        qint64 completedSize = progress.completedSize();
        qint64 completedFile = progress.completedFiles();
        qint64 currentFile = progress.currentFile();
        qint64 totalSize = qMax(completedSize, progress.totalSize());
        qint64 totalFileCount = qMax(currentFile, progress.totalFiles());
        QString overallSyncString;
        if (totalSize > 0) {
            const QString s1 = Utility::octetsToString(completedSize);
            const QString s2 = Utility::octetsToString(totalSize);

            if (progress.trustEta()) {
                //: Example text: "5 minutes left, 12 MB of 345 MB, file 6 of 7"
                overallSyncString = QApplication::translate("FolderStatus", "%5 left, %1 of %2, file %3 of %4")
                                        .arg(s1, s2)
                                        .arg(currentFile)
                                        .arg(totalFileCount)
                                        .arg(Utility::durationToDescriptiveString1(std::chrono::milliseconds(progress.totalProgress().estimatedEta)));

            } else {
                //: Example text: "12 MB of 345 MB, file 6 of 7"
                overallSyncString = QApplication::translate("FolderStatus", "%1 of %2, file %3 of %4").arg(s1, s2).arg(currentFile).arg(totalFileCount);
            }
        } else if (totalFileCount > 0) {
            // Don't attempt to estimate the time left if there is no kb to transfer.
            overallSyncString = QApplication::translate("FolderStatus", "file %1 of %2").arg(currentFile).arg(totalFileCount);
        }

        pi->_overallSyncString = overallSyncString;

        int overallPercent = 0;
        if (totalFileCount > 0) {
            // Add one 'byte' for each file so the percentage is moving when deleting or renaming files
            overallPercent = qRound(double(completedSize + completedFile) / double(totalSize + totalFileCount) * 100.0);
        }
        pi->_overallPercent = qBound(0, overallPercent, 100);
    }
}


FolderStatusModel::FolderStatusModel(QObject *parent)
    : QAbstractListModel(parent)
    , _accountState(nullptr)
{
}

FolderStatusModel::~FolderStatusModel() { }

void FolderStatusModel::setAccountState(const AccountStatePtr &accountState)
{
    // Refactoring todo: what is the logic here? I especially don't understand why we are expecting current _accountState to
    // be nullptr (via assert) when this is called.
    // if this ptr should only be set once:
    //      public setter must be removed
    //      the ptr should be passed to the FolderStatusModel ctr as a one shot setting.
    //      if useful, split the setup routine(s) into a "configure" method that can be called after the ctr.
    //      I am also in favor of independent "connect" and "disconnect" functions to keep all that logic in one place
    //      instead of spread out all over the place. call it after construction.
    beginResetModel();
    _folders.clear();
    // at least test to see if the "new" account state is legit before we go through all the setup
    if (accountState && _accountState != accountState) {
        Q_ASSERT(!_accountState);
        _accountState = accountState;

        connect(FolderMan::instance(), &FolderMan::folderSyncStateChange, this, &FolderStatusModel::slotFolderSyncStateChange);

        if (accountState->supportsSpaces()) {
            connect(accountState->account()->spacesManager(), &GraphApi::SpacesManager::updated, this,
                [this] { Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0)); });
            connect(accountState->account()->spacesManager(), &GraphApi::SpacesManager::spaceChanged, this, [this](auto *space) {
                for (int i = 0; i < rowCount(); ++i) {
                    if (_folders[i]->_folder->space() == space) {
                        Q_EMIT dataChanged(index(i, 0), index(i, 0));
                        break;
                    }
                }
            });
        }
    }
    for (const auto &f : FolderMan::instance()->folders()) {
        if (!accountState)
            break;
        if (f->accountState() != accountState)
            continue;

        _folders.push_back(std::make_unique<SubFolderInfo>(f));

        connect(ProgressDispatcher::instance(), &ProgressDispatcher::progressInfo, this, [f, this](Folder *folder, const ProgressInfo &progress) {
            if (folder == f) {
                slotSetProgress(progress, f);
            }
        });
    }

    endResetModel();
}

QVariant FolderStatusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::EditRole)
        return QVariant();

    const auto &folderInfo = _folders.at(index.row());
    auto f = folderInfo->_folder;
    if (!f)
        return QVariant();

    auto getErrors = [f] {
        auto errors = f->syncResult().errorStrings();
        const auto legacyError = FolderMan::instance()->unsupportedConfiguration(f->path());
        if (!legacyError) {
            errors.append(legacyError.error());
        }
        if (f->syncResult().hasUnresolvedConflicts()) {
            errors.append(tr("There are unresolved conflicts."));
        }
        if (f->isReady() && f->virtualFilesEnabled() && f->vfs().mode() == Vfs::Mode::WithSuffix) {
            errors.append({
                tr("The Suffix-VFS plugin is deprecated and will be removed in the 7.0 release.\n"
                   "Please use the context menu and select \"Disable virtual file support\" to ensure future access to your synced files.\n"
                   "You are going to lose access to your sync folder if you do not do so!"),
            });
        }
        return errors;
    };

    auto getDescription = [f] {
        if (auto *space = f->space()) {
            if (!space->drive().getDescription().isEmpty()) {
                return space->drive().getDescription();
            }
        }
        return tr("Local folder: %1").arg(f->shortGuiLocalPath());
    };

    switch (static_cast<Roles>(role)) {
    case Roles::Subtitle:
        return getDescription();
    case Roles::FolderErrorMsg:
        return getErrors();
    case Roles::DisplayName:
        return f->displayName();
    case Roles::FolderStatusIcon:
        return statusIconName(f);
    case Roles::SyncProgressItemString:
        return folderInfo->_progress._progressString;
    case Roles::SyncProgressOverallPercent:
        return folderInfo->_progress._overallPercent / 100.0;
    case Roles::SyncProgressOverallString:
        return folderInfo->_progress._overallSyncString;
    case Roles::Priority:
        // everything will be sorted in descending order, multiply the priority by 100 and prefer A over Z by appling a negative factor
        return QVariant::fromValue(f->priority() * 100 - (f->displayName().isEmpty() ? 0 : static_cast<int64_t>(f->displayName().at(0).toLower().unicode())));
    case Roles::Quota: {
        qint64 used{};
        qint64 total{};
        if (_accountState->supportsSpaces()) {
            if (auto *space = f->space()) {
                const auto quota = space->drive().getQuota();
                if (quota.isValid()) {
                    used = quota.getUsed();
                    total = quota.getTotal();
                }
            }
        } else {
            used = f->accountState()->quotaInfo()->lastQuotaUsedBytes();
            total = f->accountState()->quotaInfo()->lastQuotaTotalBytes();
        }
        if (total <= 0) {
            return {};
        }
        return tr("%1 of %2 used").arg(Utility::octetsToString(used), Utility::octetsToString(total));
    }
    case Roles::Folder:
        return QVariant::fromValue(f);
    case Roles::AccessibleDescriptionRole: {
        QStringList desc = {f->displayName(), Utility::enumToDisplayName(f->syncResult().status())};
        desc << getErrors();
        if (f->syncResult().status() == SyncResult::SyncRunning) {
            desc << folderInfo->_progress._overallSyncString << QStringLiteral("%1%").arg(QString::number(folderInfo->_progress._overallPercent));
        }
        desc << getDescription();
        return desc.join(QLatin1Char(','));
    }
    }
    return {};
}

Folder *FolderStatusModel::folder(const QModelIndex &index) const
{
    Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    return _folders.at(index.row())->_folder;
}

int FolderStatusModel::rowCount(const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return static_cast<int>(_folders.size());
}

QHash<int, QByteArray> FolderStatusModel::roleNames() const
{
    return {
        {static_cast<int>(Roles::DisplayName), "displayName"},
        {static_cast<int>(Roles::Subtitle), "subtitle"},
        {static_cast<int>(Roles::FolderStatusIcon), "statusIcon"},
        {static_cast<int>(Roles::SyncProgressOverallPercent), "progress"},
        {static_cast<int>(Roles::SyncProgressOverallString), "overallText"},
        {static_cast<int>(Roles::SyncProgressItemString), "itemText"},
        {static_cast<int>(Roles::FolderErrorMsg), "errorMsg"},
        {static_cast<int>(Roles::Quota), "quota"},
        {static_cast<int>(Roles::Folder), "folder"},
        {static_cast<int>(Roles::AccessibleDescriptionRole), "accessibleDescription"},
    };
}

void FolderStatusModel::slotUpdateFolderState(Folder *folder)
{
    if (!folder)
        return;
    for (int i = 0; i < _folders.size(); ++i) {
        if (_folders.at(i)->_folder == folder) {
            Q_EMIT dataChanged(index(i, 0), index(i, 0));
        }
    }
}

void FolderStatusModel::slotSetProgress(const ProgressInfo &progress, Folder *f)
{
    if (!qobject_cast<QWidget *>(QObject::parent())->isVisible()) {
        return; // for https://github.com/owncloud/client/issues/2648#issuecomment-71377909
    }

    const int folderIndex = indexOf(f);
    if (folderIndex < 0) {
        return;
    }
    const auto &folder = _folders.at(folderIndex);

    auto *pi = &folder->_progress;
    // depending on the use of virtual files or small files this slot might be called very often.
    // throttle the model updates to prevent an needlessly high cpu usage used on ui updates.
    if (folder->_lastProgressUpdateStatus != progress.status() || (std::chrono::steady_clock::now() - folder->_lastProgressUpdated > progressUpdateTimeOutC)) {
        folder->_lastProgressUpdateStatus = progress.status();

        switch (progress.status()) {
        case ProgressInfo::None:
            Q_UNREACHABLE();
            break;
        case ProgressInfo::Discovery:
            if (!progress._currentDiscoveredRemoteFolder.isEmpty()) {
                pi->_overallSyncString = tr("Checking for changes in remote '%1'").arg(progress._currentDiscoveredRemoteFolder);
            } else if (!progress._currentDiscoveredLocalFolder.isEmpty()) {
                pi->_overallSyncString = tr("Checking for changes in local '%1'").arg(progress._currentDiscoveredLocalFolder);
            }
            break;
        case ProgressInfo::Reconcile:
            pi->_overallSyncString = tr("Reconciling changes");
            break;
        case ProgressInfo::Propagation:
            Q_FALLTHROUGH();
        case ProgressInfo::Done:
            computeProgress(progress, pi);
        }
        Q_EMIT dataChanged(index(folderIndex, 0), index(folderIndex, 0));
        folder->_lastProgressUpdated = std::chrono::steady_clock::now();
    }
}

int FolderStatusModel::indexOf(Folder *f) const
{
    const auto found = std::find_if(_folders.cbegin(), _folders.cend(), [f](const auto &it) { return it->_folder == f; });
    if (found == _folders.cend()) {
        return -1;
    }
    return std::distance(_folders.cbegin(), found);
}

void FolderStatusModel::slotFolderSyncStateChange(Folder *f)
{
    if (!f) {
        return;
    }

    const int folderIndex = indexOf(f);
    if (folderIndex < 0) {
        return;
    }

    auto &pi = _folders.at(folderIndex)->_progress;

    SyncResult::Status state = f->syncResult().status();
    if (!f->canSync()) {
        // Reset progress info.
        pi = {};
    } else if (state == SyncResult::NotYetStarted) {
        pi = {};
        pi._overallSyncString = tr("Queued");
    } else if (state == SyncResult::SyncPrepare) {
        pi = {};
        pi._overallSyncString = Utility::enumToDisplayName(SyncResult::SyncPrepare);
    } else if (state == SyncResult::Problem || state == SyncResult::Success || state == SyncResult::Error) {
        // Reset the progress info after a sync.
        pi = {};
    }

    // update the icon etc. now
    slotUpdateFolderState(f);
}

void FolderStatusModel::resetFolders()
{
    setAccountState(_accountState);
}

} // namespace OCC
