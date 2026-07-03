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

#include <shlobj.h>

class OCOverlay : public IShellIconOverlayIdentifier

{
public:
    OCOverlay(int state);

    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
    IFACEMETHODIMP GetPriority(int *pPriority);
    IFACEMETHODIMP IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib);
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) Release();

protected:
    ~OCOverlay();

private:
    long _referenceCount;
    int _state;
};

