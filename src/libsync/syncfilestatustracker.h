/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Jocelyn Turcotte <jturcotte@woboq.com>
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

#ifndef SYNCFILESTATUSTRACKER_H
#define SYNCFILESTATUSTRACKER_H

#include "common/syncfilestatus.h"
#include "common/syncjournaldb.h"
#include "syncfileitem.h"
#include <QPointer>
#include <QSet>
#include <map>

namespace OCC {

class SyncEngine;

/**
 * @brief Takes care of tracking the status of individual files as they
 *        go through the SyncEngine, to be reported as overlay icons in the shell.
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT SyncFileStatusTracker : public QObject
{
    Q_OBJECT
public:
    // sync engine is always parent
    explicit SyncFileStatusTracker(const QString &folderPath, SyncJournalDb *journal, SyncEngine *syncEngine);

    SyncFileStatus fileStatus(const QString &relativePath);
    void pathTouched(const QString &fileName);

    void updateAboutToPropagate(const SyncFileItemSet &items);
    void updateItemCompleted(const SyncFileItemPtr &item);
    void updateSyncFinished();
    void updateSyncRunningChanged();

public Q_SLOTS:
    // path relative to folder
    void slotAddExcluded(const QString &folderPath);

Q_SIGNALS:
    void fileStatusChanged(const QString &systemFileName, SyncFileStatus fileStatus);

private:
    struct PathComparator {
        bool operator()( const QString& lhs, const QString& rhs ) const;
    };
    typedef std::map<QString, SyncFileStatus::SyncFileStatusTag, PathComparator> ProblemsMap;
    SyncFileStatus::SyncFileStatusTag lookupProblem(const QString &pathToMatch, const ProblemsMap &problemMap);

    enum SharedFlag { UnknownShared,
        NotShared,
        Shared };
    enum PathKnownFlag { PathUnknown = 0,
        PathKnown };
    SyncFileStatus resolveSyncAndErrorStatus(const QString &relativePath, SharedFlag sharedState, PathKnownFlag isPathKnown = PathKnown);

    void invalidateParentPaths(const QString &path);
    QString getSystemDestination(const QString &relativePath);
    void incSyncCountAndEmitStatusChanged(const QString &relativePath, SharedFlag sharedState);
    void decSyncCountAndEmitStatusChanged(const QString &relativePath, SharedFlag sharedState);

    // sync engine is also the parent. We *only* have this as a member to ask whether the path is excluded or not.
    // consider replacing the sync engine as member with an instance of the exluded files list
    QPointer<SyncEngine> _syncEngine;
    QString _folderPath;
    QPointer<SyncJournalDb> _journal;

    ProblemsMap _syncProblems;
    QSet<QString> _dirtyPaths;
    // Counts the number direct children currently being synced (has unfinished propagation jobs).
    // We'll show a file/directory as SYNC as long as its sync count is > 0.
    // A directory that starts/ends propagation will in turn increase/decrease its own parent by 1.
    QHash<QString, int> _syncCount;

    // case sensitivity used for path comparisons
    Qt::CaseSensitivity _caseSensitivity;
};
}

#endif
