/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once
#include <comdef.h>
#include <sstream>

namespace OCShell {

template <typename T = std::wstring>
void log(const std::wstring &msg, const T &error = {})
{
    std::wstringstream tmp;
    tmp << L"ownCloud ShellExtension: " << msg;
    if (!error.empty()) {
        tmp << L" " << error.data();
    }
    OutputDebugStringW(tmp.str().data());
}
inline void logWinError(const std::wstring &msg, const DWORD error = GetLastError())
{
    log(msg, std::wstring(_com_error(error).ErrorMessage()));
}

}