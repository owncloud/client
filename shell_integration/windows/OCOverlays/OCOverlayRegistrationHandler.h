/*
 * SPDX-FileCopyrightText: 2000-2013 Liferay, Inc.
 * SPDX-FileCopyrightText: 2014-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <windows.h>

class __declspec(dllexport) OCOverlayRegistrationHandler 
{
    public:
        static HRESULT MakeRegistryEntries(const CLSID& clsid, PCWSTR fileType);
        static HRESULT RegisterCOMObject(PCWSTR modulePath, PCWSTR friendlyName, const CLSID& clsid);
        static HRESULT RemoveRegistryEntries(PCWSTR friendlyName);
        static HRESULT UnregisterCOMObject(const CLSID& clsid);
};
