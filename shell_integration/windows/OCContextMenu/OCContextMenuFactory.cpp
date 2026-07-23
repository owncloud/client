/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "OCContextMenuFactory.h"
#include "OCContextMenu.h"
#include <new>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


extern long g_cDllRef;


OCContextMenuFactory::OCContextMenuFactory() : m_cRef(1)
{
    InterlockedIncrement(&g_cDllRef);
}

OCContextMenuFactory::~OCContextMenuFactory()
{
    InterlockedDecrement(&g_cDllRef);
}


// IUnknown methods

IFACEMETHODIMP OCContextMenuFactory::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(OCContextMenuFactory, IClassFactory),
        {nullptr},
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) OCContextMenuFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) OCContextMenuFactory::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef) {
        delete this;
    }
    return cRef;
}


// IClassFactory methods

IFACEMETHODIMP OCContextMenuFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;

    // pUnkOuter is used for aggregation. We do not support it in the sample.
    if (pUnkOuter == nullptr) {
        hr = E_OUTOFMEMORY;

        // Create the COM component.
        OCContextMenu *pExt = new (std::nothrow) OCContextMenu();
        if (pExt) {
            // Query the specified interface.
            hr = pExt->QueryInterface(riid, ppv);
            pExt->Release();
        }
    }

    return hr;
}

IFACEMETHODIMP OCContextMenuFactory::LockServer(BOOL fLock)
{
    if (fLock)  {
        InterlockedIncrement(&g_cDllRef);
    } else {
        InterlockedDecrement(&g_cDllRef);
    }
    return S_OK;
}
