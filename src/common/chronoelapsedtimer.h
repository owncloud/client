/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once
#include "ocsynclib.h"

#include <chrono>

namespace OCC::Utility {

/**
 * Meassure time using std::chrono::steady_clock
 */
class OCSYNC_EXPORT ChronoElapsedTimer
{
public:
    ChronoElapsedTimer();

    /**
     * Resets the timer
     */
    void reset();
    /**
     * Stops the timer
     */
    void stop();
    /**
     * Returns the elapsed time.
     * If the timer is stopped it is the time between start and stop of the timer.
     */
    std::chrono::nanoseconds duration() const;

private:
    std::chrono::steady_clock::time_point _start = {};
    std::chrono::steady_clock::time_point _end = {};
};

}
