/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "cfapi_includes.h"

namespace OCC {
void CALLBACK callbackFetchData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackCancelFetchData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackValidateData(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackNotifyDehydrate(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackNotifyDehydrateCompletion(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackFetchPlaceholders(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackCancelFetchPlaceholders(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
void CALLBACK callbackRename(const CF_CALLBACK_INFO *info, const CF_CALLBACK_PARAMETERS *params);
}
