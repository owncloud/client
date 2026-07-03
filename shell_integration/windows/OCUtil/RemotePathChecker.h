/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef PATHCHECKER_H
#define PATHCHECKER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

#pragma once    

class __declspec(dllexport) RemotePathChecker {
public:
    enum FileState {
        // Order synced with OCOverlay
        StateError = 0,
        StateOk, StateOkSWM,
        StateSync,
        StateWarning,
        StateNone
    };
    RemotePathChecker();
    ~RemotePathChecker();
    std::shared_ptr<const std::vector<std::wstring>> WatchedDirectories() const;
    bool IsMonitoredPath(const wchar_t* filePath, int* state);

private:
    FileState _StrToFileState(const std::wstring &str);
    std::mutex _mutex;
    std::atomic<bool> _stop;

    // Everything here is protected by the _mutex

    /** The list of paths we need to query. The main thread fill this, and the worker thread
     * send that to the socket. */
    std::queue<std::wstring> _pending;

    std::unordered_map<std::wstring, FileState> _cache;
    // The vector is const since it will be accessed from multiple threads through OCOverlay::IsMemberOf.
    // Each modification needs to be made onto a copy and then atomically replaced in the shared_ptr.
    std::shared_ptr<const std::vector<std::wstring>> _watchedDirectories;
    bool _connected;


    // The main thread notifies when there are new items in _pending
    //std::condition_variable _newQueries;
    HANDLE _newQueries;

    std::thread _thread;
    void workerThreadLoop();
};

#endif