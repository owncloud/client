/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "folderwatcher.h"
#include "folderwatcher_mac.h"

#include <cerrno>

#include <QScopeGuard>
#include <QStringList>


namespace OCC {

FolderWatcherPrivate::FolderWatcherPrivate(FolderWatcher *p, const QString &path)
    : _parent(p)
    , _folder(path)
{
    this->startWatching();
}

FolderWatcherPrivate::~FolderWatcherPrivate()
{
    FSEventStreamStop(_stream);
    FSEventStreamInvalidate(_stream);
    FSEventStreamRelease(_stream);
}

static void callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPathsVoid,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
    Q_UNUSED(streamRef)
    Q_UNUSED(eventFlags)
    Q_UNUSED(eventIds)

    const FSEventStreamEventFlags c_interestingFlags =
        kFSEventStreamEventFlagItemCreated // for new folder/file
        | kFSEventStreamEventFlagItemRemoved // for rm
        | kFSEventStreamEventFlagItemInodeMetaMod // for mtime change
        | kFSEventStreamEventFlagItemRenamed // also coming for moves to trash in finder
        | kFSEventStreamEventFlagItemModified; // for content change
    // We ignore other flags, e.g. for owner change, xattr change, Finder label change etc

    QSet<QString> paths;
    CFArrayRef eventPaths = static_cast<CFArrayRef>(eventPathsVoid);

    for (CFIndex i = 0; i < static_cast<CFIndex>(numEvents); ++i) {
        auto cfPath = reinterpret_cast<CFStringRef>(CFArrayGetValueAtIndex(eventPaths, i));
        const auto qPath = QString::fromCFString(cfPath).normalized(QString::NormalizationForm_C);

        if (!(eventFlags[i] & c_interestingFlags)) {
            qCDebug(lcFolderWatcher) << "Ignoring non-content changes for" << qPath;
            continue;
        }

        paths.insert(qPath);
    }

    if (!paths.isEmpty()) {
        reinterpret_cast<FolderWatcherPrivate *>(clientCallBackInfo)->doNotifyParent(paths);
    }
}

void FolderWatcherPrivate::startWatching()
{
    qCDebug(lcFolderWatcher) << "FolderWatcherPrivate::startWatching()" << _folder;

    CFStringRef cfFolder = _folder.toCFString();
    QScopeGuard freeFolder([cfFolder]() { CFRelease(cfFolder); });

    CFArrayRef pathsToWatch = CFStringCreateArrayBySeparatingStrings(nullptr, cfFolder, CFSTR(":"));
    QScopeGuard freePaths([pathsToWatch]() { CFRelease(pathsToWatch); });

    FSEventStreamContext ctx = { 0, this, nullptr, nullptr, nullptr };

    // TODO: Add kFSEventStreamCreateFlagFileEvents ?

    _stream = FSEventStreamCreate(nullptr,
        &callback,
        &ctx,
        pathsToWatch,
        kFSEventStreamEventIdSinceNow,
        0, // latency
        kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagIgnoreSelf);

    FSEventStreamScheduleWithRunLoop(_stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(_stream);
}

void FolderWatcherPrivate::doNotifyParent(const QSet<QString> &paths)
{
    _parent->changeDetected(paths);
}


} // ns mirall
