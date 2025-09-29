/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#include "utility.h"

#include <processthreadsapi.h>
#include <sddl.h>

using namespace OCC;

namespace {
// Taken from the CloudMirror example from Microsoft
std::unique_ptr<TOKEN_USER> getTokenInformation()
{
    std::unique_ptr<TOKEN_USER> tokenInfo;

    // get the tokenHandle from current thread/process if it's null
    auto tokenHandle { GetCurrentThreadEffectiveToken() }; // Pseudo token, don't free.

    DWORD tokenInfoSize { 0 };
    if (!::GetTokenInformation(tokenHandle, TokenUser, nullptr, 0, &tokenInfoSize)) {
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            tokenInfo.reset(reinterpret_cast<TOKEN_USER *>(new char[tokenInfoSize]));
            if (!::GetTokenInformation(tokenHandle, TokenUser, tokenInfo.get(), tokenInfoSize, &tokenInfoSize)) {
                qCInfo(lcVfs) << "GetTokenInformation failed";
            }
        } else {
            qCInfo(lcVfs) << "GetTokenInformation failed";
        }
    }
    return tokenInfo;
}

// Taken from the CloudMirror example from Microsoft
QString convertSidToQString(_In_ PSID sid)
{
    winrt::com_array<wchar_t> stringBuffer;
    if (ConvertSidToStringSidW(sid, winrt::put_abi(stringBuffer))) {
        // winrt::put_abi does not set the size for stringBuffer
        return QString::fromWCharArray(stringBuffer.data());
    } else {
        return {};
    }
};
} // anonymous namespace

CfAPIUtil::CfOpdata::CfOpdata()
    : correlationVector(nullptr)
{
}

CfAPIUtil::CfOpdata::CfOpdata(const CF_CALLBACK_INFO *info)
    : connectionKey(info->ConnectionKey)
    , transferKey(info->TransferKey)
    , correlationVector(info->CorrelationVector)
{
}

CF_OPERATION_INFO CfAPIUtil::CfOpdata::toOpinfo() const
{
    auto opInfo = CF_OPERATION_INFO();
    opInfo.StructSize = sizeof(opInfo);
    opInfo.ConnectionKey = connectionKey;
    opInfo.TransferKey = transferKey;
    opInfo.CorrelationVector = correlationVector;
    return opInfo;
}

Result<void, DWORD> OCC::CfAPIUtil::getFileMetadata(const Utility::Handle &fileHandle, FILE_BASIC_INFO *info)
{
    if (!GetFileInformationByHandleEx(fileHandle, FileBasicInfo, info, sizeof(*info))) {
        return GetLastError();
    }
    return {};
}

Result<void, DWORD> OCC::CfAPIUtil::getFileMetadata(const QString &filePath, FILE_BASIC_INFO *info)
{
    auto fileHandle = OCC::CfAPIUtil::getFileHandle(filePath, 0);
    if (!fileHandle) {
        return fileHandle.error();
    }
    return OCC::CfAPIUtil::getFileMetadata(*fileHandle, info);
}

Result<Utility::Handle, HRESULT> OCC::CfAPIUtil::getFileHandle(const QString &filePath, DWORD access)
{
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD creationDisp = OPEN_EXISTING;
    DWORD attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS;

    auto longPath = FileSystem::longWinPath(filePath).toStdWString();

    HANDLE fileHandle = CreateFileW(
        longPath.data(),
        access,
        shareMode,
        NULL,
        creationDisp,
        attributes,
        NULL);
    auto out = Utility::Handle(fileHandle);
    if (out) {
        return std::move(out);
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

QString CfAPIUtil::getSidAsString()
{
    std::unique_ptr<TOKEN_USER> tokenInfo(getTokenInformation());
    return convertSidToQString(tokenInfo->User.Sid);
}

template <>
Result<CF_PLACEHOLDER_BASIC_INFO, HRESULT> CfAPIUtil::getInfo(HANDLE handle)
{
    return getInfo<CF_PLACEHOLDER_BASIC_INFO>(handle, CF_PLACEHOLDER_INFO_BASIC);
}

template <>
Result<CF_PLACEHOLDER_STANDARD_INFO, HRESULT> CfAPIUtil::getInfo(HANDLE handle)
{
    return getInfo<CF_PLACEHOLDER_STANDARD_INFO>(handle, CF_PLACEHOLDER_INFO_STANDARD);
}
