/*
 * Copyright (C) by François Revol <revol@free.fr>
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

#include "folder.h"
#include "folderwatcher.h"
#include "folderwatcher_haiku.h"


#include <cerrno>
#include <QObject>
#include <QStringList>

#include <Application.h>
#include <Path.h>
#include <String.h>
#include <private/storage/PathMonitor.h>



namespace OCC {


void WatcherHandler::MessageReceived(BMessage* message)
{
    qCDebug(lcFolderWatcher) << "WatcherHandler::MessageReceived()" << message->what;
    if (message->what != B_PATH_MONITOR) {
        BHandler::MessageReceived(message);
        return;
    }

    int32 opcode;
    if (message->FindInt32("opcode", &opcode) != B_OK)
        return;

    const char *path = NULL;
    if (message->FindString("path", &path) != B_OK)
        return;

    // get the filename
    // sometimes we have it in the message, but not always
    BPath p(path);
    BString fileName(p.Leaf());
    if (fileName.StartsWith("._sync_")
        || fileName.StartsWith(".csync_journal.db")
        || fileName.StartsWith(".owncloudsync.log")
        || fileName.StartsWith(".sync_")) {
        return;
    }

    int32 fields;
    QStringList paths;

    switch (opcode) {
        case B_ENTRY_MOVED:
        {
            const char *from = NULL;
            if (message->FindString("from path", &from) != B_OK)
                return;
            QString qFrom = QString::fromUtf8(from);
            paths.append(qFrom);
            break;
        }
        case B_ENTRY_CREATED:
        case B_ENTRY_REMOVED:
            break;
        case B_STAT_CHANGED:
            if (message->FindInt32("fields", &fields) != B_OK)
                return;
            // we're only interested in mtime
            // and content change... which should always change mtime though
            // just to be sure, check size changes as well
            if (fields & (B_STAT_MODIFICATION_TIME | B_STAT_SIZE))
                break;
            return;
        case B_ATTR_CHANGED:
            // TODO: it is just unacceptable to not handle xattrs on Haiku!
            // but it must be done properly,
            // cf. http://dcevents.dublincore.org/IntConf/dc-2011/paper/view/53
        default:
            // discard
            return;
    }

    //message->PrintToStream();

    QString qPath = QString::fromUtf8(path);
    paths.append(qPath);
    emit changed(paths);
}


FolderWatcherPrivate::FolderWatcherPrivate(FolderWatcher *p, const QString &path)
    : _parent(p)
    , _folder(path)
    , _handler(new WatcherHandler)
{
    this->startWatching();
}

FolderWatcherPrivate::~FolderWatcherPrivate()
{
    BPrivate::BPathMonitor::StopWatching(BMessenger(_handler));
    be_app->Lock();
    be_app->RemoveHandler(_handler);
    be_app->Unlock();
    delete _handler;
}

void FolderWatcherPrivate::startWatching()
{
    qCDebug(lcFolderWatcher) << "FolderWatcherPrivate::startWatching()" << _folder;

    // it needs to be attached to a BLooper, which be_app happens to be.
    be_app->Lock();
    be_app->AddHandler(_handler);
    be_app->Unlock();

    connect(_handler, SIGNAL(changed(const QStringList &)),
        _parent, SLOT(changeDetected(const QStringList &)));
    connect(_handler, SIGNAL(lostChanges()),
        _parent, SIGNAL(lostChanges()));

    BMessenger messenger(_handler);
    uint32 flags = B_WATCH_RECURSIVELY | B_WATCH_NAME | B_WATCH_STAT;
    status_t err;
    err = BPrivate::BPathMonitor::StartWatching(_folder.toUtf8().constData(), flags, messenger);
    if (err != B_OK)
        qCWarning(lcFolderWatcher) << "BPathMonitor::StartWatching failed: " << strerror(err);
}


} // ns mirall
