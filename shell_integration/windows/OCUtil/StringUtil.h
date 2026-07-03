/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <windows.h>
#include <string>
#include <cassert>

class __declspec(dllexport) StringUtil {
public:
    static std::string toUtf8(const wchar_t *utf16, size_t len);
    static std::wstring toUtf16(const char *utf8, size_t len);

    template<class T>
    static bool begins_with(const T& input, const T& match)
    {
        return input.size() >= match.size()
            && std::equal(match.begin(), match.end(), input.begin());
    }

    static bool isDescendantOf(const std::wstring& child, const std::wstring& parent) {
        return isDescendantOf(child.c_str(), child.size(), parent.c_str(), parent.size());
    }

    static bool isDescendantOf(PCWSTR child, size_t childLength, const std::wstring& parent) {
        return isDescendantOf(child, childLength, parent.c_str(), parent.size());
    }

    static bool isDescendantOf(PCWSTR child, size_t childLength, PCWSTR parent, size_t parentLength) {
        if (!parentLength)
            return false;
        return (childLength == parentLength || childLength > parentLength && (child[parentLength] == L'\\' || child[parentLength - 1] == L'\\'))
            && wcsncmp(child, parent, parentLength) == 0;
    }

    static bool extractChunks(const std::wstring &source, std::wstring &secondChunk, std::wstring &thirdChunk) {
        auto statusBegin = source.find(L':', 0);
        assert(statusBegin != std::wstring::npos);

        auto statusEnd = source.find(L':', statusBegin + 1);
        if (statusEnd == std::wstring::npos) {
            // the command do not contains two colon?
            return false;
        }

        // Assume the caller extracted the chunk before the first colon.
        secondChunk = source.substr(statusBegin + 1, statusEnd - statusBegin - 1);
        thirdChunk = source.substr(statusEnd + 1);
        return true;
    }

    static bool extractChunks(const std::wstring &source, std::wstring &secondChunk, std::wstring &thirdChunk, std::wstring &forthChunk)
    {
        auto statusBegin = source.find(L':', 0);
        assert(statusBegin != std::wstring::npos);

        auto statusEnd = source.find(L':', statusBegin + 1);
        if (statusEnd == std::wstring::npos) {
            // the command do not contains two colon?
            return false;
        }

        auto thirdColon = source.find(L':', statusEnd + 1);
        if (statusEnd == std::wstring::npos) {
            // the command do not contains three colon?
            return false;
        }

        // Assume the caller extracted the chunk before the first colon.
        secondChunk = source.substr(statusBegin + 1, statusEnd - statusBegin - 1);
        thirdChunk = source.substr(statusEnd + 1, thirdColon - statusEnd - 1);
        forthChunk = source.substr(thirdColon + 1);
        return true;
    }
};
