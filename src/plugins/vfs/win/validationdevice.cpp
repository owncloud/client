/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#include "validationdevice.h"
#include "utility.h"

#include "common/filesystembase.h"
#include "common/utility.h"

using namespace OCC::FileSystem::SizeLiterals;

namespace OCC {

qint64 PlaceholderValidationDevice::size() const
{
    return _size;
}

qint64 PlaceholderValidationDevice::pos() const
{
    return _offset;
}

qint64 PlaceholderValidationDevice::readData(char *data, qint64 maxSize)
{
    if (_offset >= _size)
        return -1;
    if (maxSize == 0)
        return 0;
    auto opInfo = _opdata.toOpinfo();
    opInfo.Type = CF_OPERATION_TYPE_RETRIEVE_DATA;
    opInfo.SyncStatus = nullptr; // can convey extra user-visible text through this

    CF_OPERATION_PARAMETERS opParams = {};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(RetrieveData);
    opParams.RetrieveData.Flags = CF_OPERATION_RETRIEVE_DATA_FLAG_NONE;
    opParams.RetrieveData.Buffer = data;
    opParams.RetrieveData.Offset.QuadPart = _offset;
    // ensure length is a multiple of 4_KiB
    opParams.RetrieveData.Length.QuadPart = std::min<qint64>(maxSize - (maxSize % 4_KiB), _size - _offset);
    CfAPIUtil::assert_4_KiB_chunk(size(), opParams.RetrieveData.Offset.QuadPart, opParams.RetrieveData.Length.QuadPart);

    HRESULT ok = CfExecute(&opInfo, &opParams);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << "CfExecute error in readData" << Utility::formatWinError(ok);
        return -1;
    }

    auto readAmount = opParams.RetrieveData.ReturnedLength.QuadPart;
    _offset += readAmount;
    return readAmount;
}

bool PlaceholderValidationDevice::atEnd() const
{
    return _offset >= _size;
}

}
