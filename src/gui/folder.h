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

#ifndef MIRALL_FOLDER_H
#define MIRALL_FOLDER_H

#include "syncresult.h"
#include "progressdispatcher.h"
#include "syncjournaldb.h"
#include "clientproxy.h"
#include "networkjobs.h"
#include "abstractfolder.h"

#include <csync.h>

#include <QObject>
#include <QStringList>

class QThread;
class QSettings;

namespace OCC {

class SyncEngine;
class AccountState;
class FolderDefinition;

/**
 * @brief The Folder class
 * @ingroup gui
 */
class Folder : public AbstractFolder
{
    Q_OBJECT

public:
    Folder(const FolderDefinition &definition, AccountState *accountState, QObject *parent = 0L);

    ~Folder();

     /**
      * Ignore syncing of hidden files or not. This is defined in the
      * folder definition
      */
    bool ignoreHiddenFiles();
    void setIgnoreHiddenFiles(bool ignore);

    RequestEtagJob *etagJob() { return _requestEtagJob; }

    /**
      * Returns whether a file inside this folder should be excluded.
      */
    bool isFileExcludedAbsolute(const QString &fullPath) const;

    /**
      * Returns whether a file inside this folder should be excluded.
      */
    bool isFileExcludedRelative(const QString &relativePath) const;

signals:
    void newBigFolderDiscovered(const QString &); // A new folder bigger than the threshold was discovered

public slots:

    // connected to the corresponding signals in the SyncEngine
    void slotAboutToRemoveAllFiles(SyncFileItem::Direction, bool *);
    void slotAboutToRestoreBackup(bool *);

    void startSync(const QStringList &pathList = QStringList());

private slots:
    void slotCsyncUnavailable();
    void slotRunEtagJob();
    void etagRetreived(const QString &);
    void etagRetreivedFromSyncEngine(const QString &);

    void slotNewBigFolderDiscovered(const QString &, bool isExternal);

private:
    bool setIgnoredFiles();

    bool _csyncUnavail;
    QPointer<RequestEtagJob> _requestEtagJob;
    QString _lastEtag;
};
}

#endif
