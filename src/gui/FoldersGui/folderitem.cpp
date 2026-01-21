/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

#include "folderitem.h"

#include "configfile.h"
#include "folderitemupdater.h"
#include "progressdispatcher.h"
#include "theme.h"

namespace OCC {


FolderItem::FolderItem(Folder *folder)
    : QStandardItem()
    , _folder(folder)
{
    if (_folder == nullptr)
        return;

    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    _updater = new FolderItemUpdater(this, nullptr);
    updateStatusString();
    // this is really messed up -> Folder emits this signal using the progress dispatcher. imo this should be reworked so we can just listen to the
    // folder for this info directly. todo: soon.
    // QObject::connect(ProgressDispatcher::instance(), &ProgressDispatcher::progressInfo, this, &FolderItem::updateProgress);
}

FolderItem::~FolderItem()
{
    if (_updater) {
        delete _updater;
        _updater = nullptr;
    }
}

void FolderItem::refresh()
{
    updateStatusString();
    emitDataChanged();
}

void FolderItem::setProgress(const ProgressInfo &progress)
{
    if (progress.status() == ProgressInfo::Done) {
        // this might be premature - if so use one of the completion states from folder sync result
        _totalSize = 0;
        _completedSize = 0;
        return;
    }

    if (progress.totalSize() == 0)
        // nothing is going to happen so ditch - with vfs this will always be the case?
        return;
    // completed size is literal - meaning if there was nothing to actually sync,
    // total size = 0
    // this can happen if there are no files/folders in a space
    // or of the space is already 100% up to date, so no diffs, or if there is nothing to "move" between server/client (move or delete)
    _totalSize = progress.totalSize();
    _completedSize = progress.completedSize();
    refresh();
}

Folder *FolderItem::folder()
{
    return _folder;
}

QString FolderItem::statusIconName() const
{
    if (!_folder || !_folder->accountState())
        return {};
    //  QString foldername = _folder->displayName();
    SyncResult status = _folder->syncResult();
    if (!_folder->accountState()->isConnected()) {
        status.setStatus(SyncResult::Status::Offline);
    } else if (_folder->syncPaused() || NetworkInformation::instance()->isBehindCaptivePortal()
        || (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered())) {
        status.setStatus(SyncResult::Status::Paused);
    }
    return QStringLiteral("states/%1").arg(Theme::instance()->syncStateIconName(status));
}

QString FolderItem::statusAsString() const
{
    if (!_folder || !_folder->accountState())
        return {};

    // if (!_folder->accountState()->isConnected())
    //     return "Offline";

    SyncResult status = _folder->syncResult();

    switch (status.status()) {
    case SyncResult::NotYetStarted:
    case SyncResult::SyncPrepare:
    case SyncResult::Undefined:
    case SyncResult::SyncAbortRequested:
        return {};
    case SyncResult::Success:
        return QString("Synced");
        // this is only working for existing folders at runtime. if an unavailable space is loaded at startup there is no text
    case SyncResult::Unavailable:
        return "Unavailable: the corresponding space has been removed from the server";
    case SyncResult::Problem:
    case SyncResult::Error:
    case SyncResult::SetupError:
        return "Sync failed"; // todo: I presume this collection of states needs refinement, will cover it when I get to the error handling
    case SyncResult::Paused:
        return "Sync paused";
    case SyncResult::Offline:
        return "Offline";
    case SyncResult::SyncRunning: {
        QString completedFormatted = Utility::octetsToString(_completedSize);
        QString totalFormatted = Utility::octetsToString(_totalSize);

        return QString("Syncing %1 of %2").arg(completedFormatted, totalFormatted);
    }
    };
}

void FolderItem::updateStatusString()
{
    // the idea here is that we only want to update the string if the current status is something we care to show. If
    // a new status string is empty, just use the LAST status string until we get something new that we care about
    QString newStatusString = statusAsString();
    if (!newStatusString.isEmpty())
        _statusString = newStatusString;
}


QVariant FolderItem::data(int role) const
{
    if (!_folder)
        return QVariant();

    /* auto getErrors = [f] {
         auto errors = f->syncResult().errorStrings();
         const Result<void, QString> notLegacyError = FolderMan::instance()->unsupportedConfiguration(f->path());
         if (!notLegacyError) {
             errors.append(notLegacyError.error());
         }
         if (f->syncResult().hasUnresolvedConflicts()) {
             errors.append(tr("There are unresolved conflicts."));
         }
         return errors;
     };*/

    switch (role) {
        //  case Roles::Subtitle:
        //      return getDescription();
        //  case Roles::FolderErrorMsg:
        //      return getErrors();
    case Qt::DisplayRole:
        return _folder->displayName();
    case Qt::DecorationRole:
        /* if (_folder && _folder->space() && _folder->space()->image()) {
             // this is not working but I have no idea why!
             // the image is not null but won't paint, even in the default impl
             // so just using the app icon to get the delegate working for now
             QIcon spaceIcon = _folder->space()->image()->image();
             if (!spaceIcon.isNull())
                 return spaceIcon;
         }*/
        return Theme::instance()->applicationIcon();
    case FolderItemRoles::StatusIconRole:
        return Resources::getCoreIcon(statusIconName());
    case FolderItemRoles::StatusStringRole:
        return _statusString;
        // case ItemRoles::StatusInfoRole:
        //   return _progress._progressString;
        /*case Roles::SyncProgressOverallPercent:
            return folderInfo->_progress._overallPercent / 100.0;
        case Roles::SyncProgressOverallString:
            return folderInfo->_progress._overallSyncString;
        case Roles::Priority:
            // everything will be sorted in descending order, multiply the priority by 100 and prefer A over Z by applying a negative factor
            return QVariant::fromValue(
                _folder->priority() * 100 - (f->displayName().isEmpty() ? 0 : static_cast<int64_t>(f->displayName().at(0).toLower().unicode())));
        case Roles::Quota: {
            qint64 used{};
            qint64 total{};

            if (auto *space = f->space()) {
                const auto quota = space->drive().getQuota();
                if (quota.isValid()) {
                    used = quota.getUsed();
                    total = quota.getTotal();
                }
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
        }*/
    }
    return QStandardItem::data(role);
}
}
