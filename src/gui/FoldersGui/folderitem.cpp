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

    _updater = new FolderItemUpdater(this);
    updateStatusString();
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

    if (progress.totalSize() == 0) {
        // nothing is going to happen so ditch - with vfs this will always be the case?
        return;
    }

    // completed size is literal - meaning if there was nothing to actually sync,
    // total size = 0
    // this can happen if there are no files/folders in a space
    // or of the space is already 100% up to date, so no diffs, or if there is nothing to "move" between server/client (move or delete)
    _totalSize = progress.totalSize();
    _completedSize = progress.completedSize();
    //_percentComplete = progress.refresh();
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
    SyncResult status = _folder->syncResult();
    if (!_folder->isConnected()) {
        status.setStatus(SyncResult::Status::Offline);
    } else if (_folder->syncPaused() || NetworkInformation::instance()->isBehindCaptivePortal()
        || (NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered())) {
        status.setStatus(SyncResult::Status::Paused);
    }
    return QString("states/%1").arg(Theme::instance()->syncStateIconName(status));
}

QString FolderItem::statusAsString() const
{
    if (!_folder || !_folder->accountState())
        return {};

    if (!_folder->isConnected())
        return tr("Offline");

    // this can get out of sorts wrt sync state if we were offline then go on - no sync is ever attempted if the folder is already
    // marked unavailable, so hopefully this catches the corner cases
    if (!_folder->isAvailable())
        return tr("Unavailable: the space is no longer available on the server");

    // when reloading folders from config, no sync on paused folders is attempted, so we can't rely on the sync status
    if (_folder->syncPaused())
        return tr("Sync paused");

    SyncResult status = _folder->syncResult();

    switch (status.status()) {
    case SyncResult::SyncPrepare:
    case SyncResult::Undefined:
    case SyncResult::SyncAbortRequested:
        return {};
    case SyncResult::Success:
        return QString("Synced");
    case SyncResult::Unavailable:
        return tr("Unavailable: the space is no longer available on the server");
    case SyncResult::Problem:
    case SyncResult::Error:
    case SyncResult::SetupError:
        return tr("Sync failed"); // todo: I presume this collection of states needs refinement, will cover it when I get to the error handling
    case SyncResult::Paused:
        return tr("Sync paused");
    case SyncResult::Offline:
        return tr("Offline");
    case SyncResult::NotYetStarted:
        return tr("Sync pending");
    case SyncResult::SyncRunning: {
        QString completedFormatted = Utility::octetsToString(_completedSize);
        QString totalFormatted = Utility::octetsToString(_totalSize);

        return tr("Syncing %1 of %2").arg(completedFormatted, totalFormatted);
    }
    };

    return {};
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
            return folderInfo->_progress._overallSyncString; */
    case FolderItemRoles::SortPriorityRole:
        // everything will be sorted in descending order, multiply the priority by 100 and prefer A over Z by applying a negative factor
        return QVariant::fromValue(
            _folder->sortPriority() * 100 - (_folder->displayName().isEmpty() ? 0 : static_cast<int64_t>(_folder->displayName().at(0).toLower().unicode())));
        /*

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
