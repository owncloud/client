/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <QSocketNotifier>
#include <QHash>
#include <QDir>

#include "folderwatcher.h"

class QTimer;

namespace OCC {

/**
 * @brief Linux (inotify) API implementation of FolderWatcher
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT FolderWatcherPrivate : public QObject
{
    Q_OBJECT
public:
    FolderWatcherPrivate() {}
    FolderWatcherPrivate(FolderWatcher *p, const QString &path);

    int testWatchCount() const { return _pathToWatch.size(); }

    /// On linux the watcher is ready when the ctor finished.
    constexpr bool isReady() const { return true; }

protected Q_SLOTS:
    void slotReceivedNotification(int fd);
    void slotAddFolderRecursive(const QString &path);

protected:
    bool findFoldersBelow(const QDir &dir, QStringList &fullList);
    void inotifyRegisterPath(const QString &path);
    void removeFoldersBelow(const QString &path);

private:
    FolderWatcher *_parent;

    QString _folder;
    QHash<int, QString> _watchToPath;
    QMap<QString, int> _pathToWatch;
    QScopedPointer<QSocketNotifier> _socket;
    int _fd;
};
}
