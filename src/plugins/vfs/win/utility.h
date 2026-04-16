/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under GPL 2.0 or later.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "cfapi_includes.h"

#include "common/filesystembase.h"
#include "common/result.h"
#include "common/utility.h"
#include "common/utility_win.h"

#include <QString>
#include <QLoggingCategory>

#include <winrt/Windows.Foundation.h>

using namespace OCC::FileSystem::SizeLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcVfs)


// Horrible macro magic to help to initialise CF_OPERATION_PARAMETERS.ParamSize
// see: https://github.com/microsoft/Windows-classic-samples/blob/27ffb0811ca761741502feaefdb591aebf592193/Samples/CloudMirror/CloudMirror/FileCopierWithProgress.cpp#L38
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#define CF_SIZE_OF_OP_PARAM(field) (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + FIELD_SIZE(CF_OPERATION_PARAMETERS, field))
namespace OCC::CfAPIUtil {

/// Holds data that callbacks provide and that needs to be passed into CfExecute
struct CfOpdata
{
    CfOpdata();

    explicit CfOpdata(const CF_CALLBACK_INFO *info);

    CF_OPERATION_INFO toOpinfo() const;

    CF_CONNECTION_KEY connectionKey;
    LARGE_INTEGER transferKey;
    PCORRELATION_VECTOR correlationVector;
};

inline QString hstringToQString(const winrt::hstring &str)
{
    const std::wstring_view view { str };
    // qt6 will take size_t
    return QString::fromWCharArray(view.data(), view.size());
}


Result<OCC::Utility::Handle, HRESULT> getFileHandle(const QString &filePath, DWORD access);

template <typename INFO>
[[nodiscard]] Result<INFO, HRESULT> getInfo(HANDLE handle, CF_PLACEHOLDER_INFO_CLASS type)
{
    auto info = INFO();
    DWORD resultsize = 0;
    HRESULT ok = CfGetPlaceholderInfo(handle, type, &info, sizeof(info), &resultsize);
    // INFO has a variable lenght, we are however only interested in the fixed size members
    if (SUCCEEDED(ok) || ok == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
        return std::move(info);
    return ok;
}


template <typename INFO>
[[nodiscard]] Result<INFO, HRESULT> getInfo(HANDLE handle)
{
    static_assert(std::is_same<INFO, void>::value, "Unreachable");
}

template <>
[[nodiscard]] Result<CF_PLACEHOLDER_BASIC_INFO, HRESULT> getInfo(HANDLE handle);

template <>
[[nodiscard]] Result<CF_PLACEHOLDER_STANDARD_INFO, HRESULT> getInfo(HANDLE handle);


template <typename INFO>
[[nodiscard]] Result<INFO, HRESULT> getInfo(const QString &fileName)
{
    auto handle = getFileHandle(fileName, 0);
    if (!handle) {
        // If the source file doesn't exist, there's nothing to propagate
        if (handle.error() != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            qCWarning(lcVfs) << Q_FUNC_INFO << ": couldn't get handle for" << fileName << Utility::formatWinError(handle.error());
        }
        return handle.error();
    }
    return getInfo<INFO>(handle.get());
}

[[nodiscard]] Result<void, DWORD> getFileMetadata(const Utility::Handle &fileHandle, FILE_BASIC_INFO *info);

[[nodiscard]] Result<void, DWORD> getFileMetadata(const QString &filePath, FILE_BASIC_INFO *info);

QString getSidAsString();

/**
 *
 * Assert that we follow https://learn.microsoft.com/en-us/windows/win32/api/cfapi/ns-cfapi-cf_operation_parameters
 * We either folow the alignment or we passed the whole file size.
 * @param size The size of the complete file
 * @param offset Offset for the current chunk
 * @param length  Length of the current chunk
 */
inline void assert_4_KiB_chunk(size_t size, size_t offset, size_t length)
{
    // the offset must be 0 or a multiple of 4_KiB
    Q_ASSERT(offset % 4_KiB == 0);
    Q_ASSERT(
        // length is a multiple of 4_KiB
        (length % 4_KiB == 0)
        // we queried the rest of the file, here the 4_KiB rule does not apply
        || (offset + length == size));
}
}
