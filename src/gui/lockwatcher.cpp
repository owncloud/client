/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lockwatcher.h"
#include "filesystem.h"

#include <QLoggingCategory>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

using namespace OCC;

Q_LOGGING_CATEGORY(lcLockWatcher, "gui.lockwatcher", QtInfoMsg)

namespace {
const auto check_frequency = 20s;
}

LockWatcher::LockWatcher(QObject *parent)
    : QObject(parent)
{
    connect(&_timer, &QTimer::timeout,
        this, &LockWatcher::checkFiles);
    _timer.start(check_frequency);
}

void LockWatcher::addFile(const QString &path, FileSystem::LockMode mode)
{
    qCInfo(lcLockWatcher) << "Watching for lock of" << path << mode << "being released";
    _watchedPaths.insert({ path, mode });
}

void LockWatcher::setCheckInterval(std::chrono::milliseconds interval)
{
    _timer.start(interval.count());
}

bool LockWatcher::contains(const QString &path, OCC::FileSystem::LockMode mode) const
{
    return _watchedPaths.find({ path, mode }) != _watchedPaths.cend();
}

void LockWatcher::checkFiles()
{
    // copy as Q_EMIT fileUnlocked might trigger a new insert
    const auto watchedPathsCopy = _watchedPaths;
    decltype(_watchedPaths) unlocked;
    for (const auto &p : watchedPathsCopy) {
        if (!FileSystem::isFileLocked(p.first, p.second)) {
            qCInfo(lcLockWatcher) << "Lock of" << p.first << p.second << "was released";
            Q_EMIT fileUnlocked(p.first, p.second);
            unlocked.insert(p);
        }
    }
    for (const auto &removed : std::as_const(unlocked)) {
        _watchedPaths.erase(removed);
    }
}
