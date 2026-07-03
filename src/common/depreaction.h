/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <qglobal.h>

#ifdef Q_CC_MSVC
// _Pragma would require C99 ecm currently sets C90
#define OC_DISABLE_DEPRECATED_WARNING \
    __pragma(warning(push));          \
    __pragma(warning(disable : 4996))

#define OC_ENABLE_DEPRECATED_WARNING __pragma(warning(pop))
#else
#define OC_DISABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic push");   \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

#define OC_ENABLE_DEPRECATED_WARNING _Pragma("GCC diagnostic pop")
#endif
