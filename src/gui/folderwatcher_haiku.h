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

#ifndef MIRALL_FOLDERWATCHER_HAIKU_H
#define MIRALL_FOLDERWATCHER_HAIKU_H

#include <QObject>
#include <QString>

#include <Handler.h>

namespace OCC {

/**
 * @brief The WatcherHandler class
 * @ingroup gui
 */
class WatcherHandler : public QObject, public BHandler {
    Q_OBJECT
public:
    WatcherHandler() : QObject(), BHandler("FolderWatcher") {}
    virtual ~WatcherHandler() {}
    virtual void MessageReceived(BMessage* message);

signals:
    void changed(const QStringList &paths);
    void lostChanges();

private:
    QString _path;
};

/**
 * @brief Haiku API implementation of FolderWatcher
 * @ingroup gui
 */
class FolderWatcherPrivate : public QObject
{
    Q_OBJECT
public:
    FolderWatcherPrivate(FolderWatcher *p, const QString &path);
    ~FolderWatcherPrivate();

    void addPath(const QString &) {}
    void removePath(const QString &) {}

    void startWatching();

    /// On Haiku the watcher is ready when the ctor finished.
    bool _ready = 1;

private:
    FolderWatcher *_parent;

    QString _folder;

    WatcherHandler *_handler;
};
}

#endif
