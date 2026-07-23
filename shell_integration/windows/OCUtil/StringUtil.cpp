/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <locale>
#include <string>
#include <codecvt>

#include "StringUtil.h"

std::string StringUtil::toUtf8(const wchar_t *utf16, size_t len)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
    return converter.to_bytes(utf16, utf16+len);
}

std::wstring StringUtil::toUtf16(const char *utf8, size_t len)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
    return converter.from_bytes(utf8, utf8+len);
}
