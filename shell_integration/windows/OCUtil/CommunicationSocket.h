/*
 * SPDX-FileCopyrightText: 2000-2013 Liferay, Inc.
 * SPDX-FileCopyrightText: 2014-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef COMMUNICATIONSOCKET_H
#define COMMUNICATIONSOCKET_H

#pragma once

#pragma warning (disable : 4251)

#include <string>
#include <vector>
#include <WinSock2.h>

class __declspec(dllexport) CommunicationSocket
{
public:
    static std::wstring DefaultPipePath();

    CommunicationSocket();
    ~CommunicationSocket();

    bool Connect(const std::wstring& pipename);
    bool Close();

    [[nodiscard]] bool SendMsg(const std::wstring &) const;
    [[nodiscard]] bool ReadLine(std::wstring *) const;

    HANDLE Event() { return _pipe; }

private:    
    HANDLE _pipe;
    mutable std::vector<char> _buffer;
    bool _connected;

    mutable OVERLAPPED _overlapped = {};
};

#endif
