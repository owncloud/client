/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MIRALL_FOLDERWATCHER_MAC_H
#define MIRALL_FOLDERWATCHER_MAC_H

#include <QObject>
#include <QString>

#include <CoreServices/CoreServices.h>


namespace OCC {

class FolderWatcher;

/**
 * @brief Mac OS X API implementation of FolderWatcher
 * @ingroup gui
 */
class FolderWatcherPrivate
{
public:
    FolderWatcherPrivate(FolderWatcher *p, const QString &path);
    ~FolderWatcherPrivate();

    void startWatching();
    void doNotifyParent(const QSet<QString> &);

    /// On OSX the watcher is ready when the ctor finished.
    constexpr bool isReady() const { return true; }

private:
    FolderWatcher *_parent;

    QString _folder;

    FSEventStreamRef _stream;
};

} // namespace OCC

#endif
