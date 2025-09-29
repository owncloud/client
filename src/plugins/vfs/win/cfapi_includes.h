/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifdef _WINDOWS_
#error Windows.h was already included
#endif
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <ntstatus.h>
#include <winnt.h>
#include <winternl.h>
#include <comdef.h>

#include <cfapi.h>
