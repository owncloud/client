/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#include "callbacks.h"
#include "libsync/syncengine.h"

#include "vfs_win_p.h"

#define ENUM_CASE_NAME(X)             \
    case X:                           \
        return QString::fromUtf8(#X); \
        break;

namespace {
QString dehydrationReasonToName(CF_CALLBACK_DEHYDRATION_REASON reason)
{
    switch (reason) {
        ENUM_CASE_NAME(CF_CALLBACK_DEHYDRATION_REASON_NONE);
        ENUM_CASE_NAME(CF_CALLBACK_DEHYDRATION_REASON_USER_MANUAL);
        ENUM_CASE_NAME(CF_CALLBACK_DEHYDRATION_REASON_SYSTEM_LOW_SPACE);
        ENUM_CASE_NAME(CF_CALLBACK_DEHYDRATION_REASON_SYSTEM_INACTIVITY);
        ENUM_CASE_NAME(CF_CALLBACK_DEHYDRATION_REASON_SYSTEM_OS_UPGRADE);
    }
    Q_UNREACHABLE();
}
}

namespace OCC {

// WARNING: Callbacks in threadpool: multiple and parallel calls are possible
void CALLBACK callbackFetchData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    qCInfo(lcVfs) << Q_FUNC_INFO;
    const auto callbackContext = reinterpret_cast<VfsWinPrivate *>(info->CallbackContext);
    //    const auto transferKey = info->TransferKey.QuadPart;
    const auto opdata = CfAPIUtil::CfOpdata(info);

    // Note: Resuming works as expected: the file is already partially hydrated
    // we will see resumeStart > 0 here.
    auto resumeStart = params->FetchData.RequiredFileOffset.QuadPart;
    auto resumeEnd = resumeStart + params->FetchData.RequiredLength.QuadPart;

    if (params->FetchData.OptionalFileOffset.QuadPart == resumeEnd) {
        resumeEnd += params->FetchData.OptionalLength.QuadPart;
        qCInfo(lcVfs) << Q_FUNC_INFO << "Performing an optional extended hydration";
    } else if (OC_ENSURE(params->FetchData.OptionalFileOffset.QuadPart == 0)) {
        // noop
    }

    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    auto fileId = QByteArray(reinterpret_cast<const char *>(info->FileIdentity), info->FileIdentityLength);
    qCInfo(lcVfs) << "fetch data request" << callbackContext->q->params().filesystemPath << path << fileId
                  << params->FetchData.RequiredFileOffset.QuadPart << params->FetchData.RequiredLength.QuadPart
                  << params->FetchData.OptionalFileOffset.QuadPart << params->FetchData.OptionalLength.QuadPart
                  << "Resume:" << resumeStart << "to" << resumeEnd << "size:" << (resumeEnd - resumeStart);

    // Jump to gui thread
    QMetaObject::invokeMethod(callbackContext, [callbackContext, resumeStart, resumeEnd, path, fileId, opdata]() {
        callbackContext->startFetchData(opdata, path, resumeStart, resumeEnd, fileId);
    }, Qt::QueuedConnection);
}

void CALLBACK callbackCancelFetchData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    Q_UNUSED(params)

    qCInfo(lcVfs) << Q_FUNC_INFO;
    const auto callbackContext = reinterpret_cast<VfsWinPrivate *>(info->CallbackContext);
    const auto transferKey = info->TransferKey.QuadPart;
    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    qCInfo(lcVfs) << "fetch data request abort for" << path;

    // Jump to gui thread
    QMetaObject::invokeMethod(callbackContext, [callbackContext, transferKey, path]() {
        callbackContext->cancelFetchData(transferKey, path);
    }, Qt::QueuedConnection);
}

void CALLBACK callbackValidateData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    qCInfo(lcVfs) << Q_FUNC_INFO;
    // TODO: Even if this fails the local file will be read as a hydrated placeholder afterwards! That's annoying!

    const auto callbackContext = reinterpret_cast<VfsWinPrivate *>(info->CallbackContext);
    auto opdata = CfAPIUtil::CfOpdata(info);
    auto offset = params->ValidateData.RequiredFileOffset.QuadPart;
    auto length = params->ValidateData.RequiredLength.QuadPart;
    auto fileSize = info->FileSize.QuadPart;

    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    qCInfo(lcVfs) << "request to validate data" << path << offset << length;

    // Jump to gui thread
    QMetaObject::invokeMethod(callbackContext, [callbackContext, opdata, offset, length, fileSize, path]() mutable {
        callbackContext->startValidateData(opdata, path, offset, length, fileSize);
    }, Qt::QueuedConnection);
}

void CALLBACK callbackNotifyDehydrate(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    qCInfo(lcVfs) << Q_FUNC_INFO << "dehydration of" << path << "requested, reason " << dehydrationReasonToName(params->Dehydrate.Reason);
}

void CALLBACK callbackNotifyDehydrateCompletion(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    qCInfo(lcVfs) << Q_FUNC_INFO << "dehydration of" << path << "completed, reason " << dehydrationReasonToName(params->DehydrateCompletion.Reason);
}

void CALLBACK callbackFetchPlaceholders(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    Q_UNUSED(params);
    qCInfo(lcVfs) << Q_FUNC_INFO << path;
}

void CALLBACK callbackCancelFetchPlaceholders(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    Q_UNUSED(params)

    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    qCInfo(lcVfs) << Q_FUNC_INFO << path;
}

void CALLBACK callbackRename(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params)
{
    const auto callbackContext = reinterpret_cast<VfsWinPrivate *>(info->CallbackContext);
    const QString path = CfCallbackInfoHelper::getFullNormalizedPath(info);
    const QString newPath = CfCallbackInfoHelper::getFullPath(info, params->Rename.TargetPath);
    CF_OPERATION_PARAMETERS opParams = {};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(AckRename);
    opParams.AckRename.CompletionStatus = STATUS_SUCCESS;

    CF_SYNC_STATUS *syncStatus = nullptr;
    qCInfo(lcVfs) << Q_FUNC_INFO << path << "to" << newPath;
    VfsWin *vfs = callbackContext->q;
    if (vfs->isDehydratedPlaceholder(path) && vfs->params().syncEngine()->isExcluded(newPath)) {
        qCWarning(lcVfs) << "Preventing rename of" << path << "to" << newPath << "as it would create a stray placeholder";
        opParams.AckRename.CompletionStatus = STATUS_CLOUD_FILE_NOT_SUPPORTED;
    }
    const auto opdata = CfAPIUtil::CfOpdata(info);
    auto opInfo = opdata.toOpinfo();
    opInfo.Type = CF_OPERATION_TYPE_ACK_RENAME;
    opInfo.SyncStatus = syncStatus;

    HRESULT ok = CfExecute(&opInfo, &opParams);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << "CfExecute error on signaling fetch error" << Utility::formatWinError(ok);
        return;
    }
}

} // OCC namespace
