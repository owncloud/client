/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ocsynclib.h"

#include <QString>

#include <functional>
#include <qt_windows.h>

namespace OCC {
namespace Utility {
    class OCSYNC_EXPORT Handle
    {
    public:
        /**
         * A RAAI for Windows Handles
         */
        Handle() = default;
        explicit Handle(HANDLE h);
        explicit Handle(HANDLE h, std::function<void(HANDLE)> &&close);

        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;

        Handle(Handle &&other)
        {
            std::swap(_handle, other._handle);
            std::swap(_close, other._close);
        }

        Handle &operator=(Handle &&other)
        {
            if (this != &other) {
                std::swap(_handle, other._handle);
                std::swap(_close, other._close);
            }
            return *this;
        }

        ~Handle();

        HANDLE &handle() { return _handle; }

        void close();

        explicit operator bool() const { return _handle != INVALID_HANDLE_VALUE; }

        operator HANDLE() const { return _handle; }

    private:
        HANDLE _handle = INVALID_HANDLE_VALUE;
        std::function<void(HANDLE)> _close;
    };

    // Possibly refactor to share code with UnixTimevalToFileTime in c_time.c
    OCSYNC_EXPORT void UnixTimeToFiletime(time_t t, FILETIME *filetime);
    OCSYNC_EXPORT void FiletimeToLargeIntegerFiletime(const FILETIME *filetime, LARGE_INTEGER *hundredNSecs);
    OCSYNC_EXPORT void UnixTimeToLargeIntegerFiletime(time_t t, LARGE_INTEGER *hundredNSecs);

    OCSYNC_EXPORT QString formatWinError(long error);
}
}
