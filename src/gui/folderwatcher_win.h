/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MIRALL_FOLDERWATCHER_WIN_H
#define MIRALL_FOLDERWATCHER_WIN_H

#include <QAtomicInt>
#include <QThread>
#include <qt_windows.h>

namespace OCC {

class FolderWatcher;
class FolderWatcherPrivate;

/**
 * @brief The WatcherThread class
 * @ingroup gui
 */
class WatcherThread : public QThread
{
    Q_OBJECT
public:
    WatcherThread(FolderWatcherPrivate *parent, const QString &path);
    ~WatcherThread() override;

    void stop();

protected:
    enum class WatchChanges {
        Done,
        NeedBiggerBuffer,
        Error,
    };

    void run() override;
    WatchChanges watchChanges(size_t fileNotifyBufferSize);
    void processEntries(FILE_NOTIFY_INFORMATION *curEntry);
    void closeHandle();

Q_SIGNALS:
    void changed(const QSet<QString> &path);
    void lostChanges();

private:
    FolderWatcherPrivate *_parent;
    const QString _path;
    const QString _longPath;
    HANDLE _directory;
    HANDLE _resultEvent;
    HANDLE _stopEvent;
};

/**
 * @brief Windows implementation of FolderWatcher
 * @ingroup gui
 */
class FolderWatcherPrivate : public QObject
{
    Q_OBJECT
public:
    FolderWatcherPrivate(FolderWatcher *p, const QString &path);
    ~FolderWatcherPrivate() override;

    /// Set to non-zero once the WatcherThread is capturing events.
    bool isReady() const
    {
        return _ready;
    }

private:
    FolderWatcher *_parent;
    QScopedPointer<WatcherThread> _thread;
    bool _ready = false;
    friend class WatcherThread;
};
}

#endif // MIRALL_FOLDERWATCHER_WIN_H
