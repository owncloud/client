/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef AbstractSocketHandler_H
#define AbstractSocketHandler_H

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <windows.h>

class CommunicationSocket;

class OCClientInterface
{
public:
    struct ContextMenuInfo {
        std::vector<std::wstring> watchedDirectories;
        std::wstring contextMenuTitle;
        std::shared_ptr<HBITMAP> icon = {};
        struct MenuItem
        {
            std::wstring command, flags, title;
        };
        std::vector<MenuItem> menuItems;
    };
    static ContextMenuInfo FetchInfo(const std::wstring &files);
    [[nodiscard]] static bool SendRequest(const std::wstring &verb, const std::wstring &path);
};

#endif //ABSTRACTSOCKETHANDLER_H
