/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <qglobal.h>

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102921
// we hope it will be fixed in GCC >= 13
#if !defined(Q_CC_GNU) || Q_CC_GNU >= 1300
#define constexpr_list constexpr
#else
#define constexpr_list inline
#endif