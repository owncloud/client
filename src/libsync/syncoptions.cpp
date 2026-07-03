/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "syncoptions.h"
#include "common/utility.h"

#include <QRegularExpression>

using namespace OCC;

SyncOptions::SyncOptions(Vfs *vfs)
    : _vfs(vfs)
{
}

SyncOptions::~SyncOptions()
{
}

bool SyncOptions::isValid() const
{
    return !_vfs.isNull();
}
