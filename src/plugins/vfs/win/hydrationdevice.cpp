/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under GPL 2.0 or later.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#include "hydrationdevice.h"

#include "common/filesystembase.h"
#include "common/utility.h"

using namespace OCC::FileSystem::SizeLiterals;

namespace {
/**
 * The only requirement is that both offset and length are 4KB aligned unless the range described ends on the logical file size (EoF),
 * in which case, the length is not required to be 4KB aligned as long as the resulting range ends on or beyond the logical file size.
 *
 * https://learn.microsoft.com/en-us/windows/win32/api/cfapi/ns-cfapi-cf_operation_parameters
 */
constexpr qsizetype minChunkSizeC = 4_KiB * 1000;
}

namespace OCC {

PlaceholderHydrationDevice::PlaceholderHydrationDevice(
    CfAPIUtil::CfOpdata opdata,
    quint64 startOffset,
    quint64 size)
    : _opdata(opdata)
    , _offset(startOffset)
    , _size(size)
{
    // reserve a buffer with twice the size
    _buffer.reserve(minChunkSizeC * 2);
    _progressCompleted.QuadPart = startOffset;
}

qint64 PlaceholderHydrationDevice::writeData(const char *data, qint64 maxSize)
{
    _buffer.append(data, maxSize);

    // Feed data if it has an acceptable size or if it's the final chunk.
    while (!_buffer.isEmpty() && (_buffer.size() >= minChunkSizeC || _buffer.size() == _size - _offset)) {
        auto feedAmount = std::min<uint64_t>(_buffer.size(), minChunkSizeC);

        auto opInfo = _opdata.toOpinfo();
        opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
        opInfo.SyncStatus = nullptr; // can convey extra user-visible text through this

        CF_OPERATION_PARAMETERS opParams = {};
        opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
        opParams.TransferData.Flags = CF_OPERATION_TRANSFER_DATA_FLAG_NONE;
        opParams.TransferData.Offset.QuadPart = _offset;
        opParams.TransferData.Length.QuadPart = feedAmount;
        opParams.TransferData.CompletionStatus = STATUS_SUCCESS;
        opParams.TransferData.Buffer = _buffer.constData();

        HRESULT ok = CfExecute(&opInfo, &opParams);
        if (FAILED(ok)) {
            qCWarning(lcVfs) << "CfExecute error in writeData" << Utility::formatWinError(ok);
            return -1;
        }

        _offset += feedAmount;
        // inplace righ()
        const auto newSize = _buffer.size() - feedAmount;
        memcpy(_buffer.data(), _buffer.data() + feedAmount, newSize);
        _buffer.resize(newSize);
    }

    // refresh Windows Copy Dialog progress anyhow

    LARGE_INTEGER progressTotal = {};
    progressTotal.QuadPart = _size;

    _progressCompleted.QuadPart += maxSize;


    HRESULT ok = CfReportProviderProgress(_opdata.connectionKey, _opdata.transferKey, progressTotal, _progressCompleted);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << "CfReportProviderProgress error in writeData" << Utility::formatWinError(ok);
    }
    return maxSize;
}

void PlaceholderHydrationDevice::close()
{
    if (_buffer.size() > 0) {
        qCCritical(lcVfs) << "Closed PlaceholderHydrationDevice while buffer still filled" << _buffer.size() << _offset << _size;
    }
    QIODevice::close();
}

}
