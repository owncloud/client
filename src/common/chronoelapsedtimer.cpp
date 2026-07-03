/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "chronoelapsedtimer.h"

#include <QtGlobal>

using namespace OCC::Utility;

ChronoElapsedTimer::ChronoElapsedTimer()
    : _start(std::chrono::steady_clock::now())
{
}

void ChronoElapsedTimer::reset()
{
    _start = std::chrono::steady_clock::now();
    _end = {};
}

void ChronoElapsedTimer::stop()
{
    Q_ASSERT(_end == std::chrono::steady_clock::time_point {});
    _end = std::chrono::steady_clock::now();
}

std::chrono::nanoseconds ChronoElapsedTimer::duration() const
{
    if (_end != std::chrono::steady_clock::time_point {}) {
        return _end - _start;
    }
    return std::chrono::steady_clock::now() - _start;
}
