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

class __declspec(dllexport) OCContextMenuRegHandler
{
public:
    static HRESULT MakeRegistryEntries(const CLSID& clsid, PCWSTR fileType);
    static HRESULT RegisterCOMObject(PCWSTR modulePath, PCWSTR friendlyName, const CLSID& clsid);
    static HRESULT RemoveRegistryEntries(PCWSTR friendlyName);
    static HRESULT UnregisterCOMObject(const CLSID& clsid);

    static HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid, PCWSTR pszFriendlyName, PCWSTR pszThreadModel);
    static HRESULT UnregisterInprocServer(const CLSID& clsid);

    static HRESULT RegisterShellExtContextMenuHandler(PCWSTR pszFileType, const CLSID& clsid, PCWSTR pszFriendlyName);
    static HRESULT UnregisterShellExtContextMenuHandler(PCWSTR pszFileType, PCWSTR pszFriendlyName);
};
