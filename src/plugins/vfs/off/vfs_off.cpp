/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "vfs_off.h"

#include "filesystem.h"
#include "syncfileitem.h"

using namespace OCC;

VfsOff::VfsOff(QObject *parent)
    : Vfs(parent)
{
}

VfsOff::~VfsOff() = default;

Vfs::Mode VfsOff::mode() const
{
    return Vfs::Off;
}

void VfsOff::stop() { }

void VfsOff::unregisterFolder() { }

bool VfsOff::socketApiPinStateActionsShown() const
{
    return false;
}

Result<void, QString> VfsOff::createPlaceholder(const SyncFileItem &)
{
    return {};
}

bool VfsOff::needsMetadataUpdate(const SyncFileItem &)
{
    return false;
}

bool VfsOff::isDehydratedPlaceholder(const QString &)
{
    return false;
}

bool VfsOff::statTypeVirtualFile(csync_file_stat_t *, void *)
{
    return false;
}

bool VfsOff::setPinState(const QString &, PinState)
{
    return true;
}

Optional<PinState> VfsOff::pinState(const QString &)
{
    return PinState::AlwaysLocal;
}

Vfs::AvailabilityResult VfsOff::availability(const QString &)
{
    return VfsItemAvailability::AlwaysLocal;
}

void VfsOff::startImpl(const VfsSetupParams &)
{
    Q_EMIT started();
}

Result<Vfs::ConvertToPlaceholderResult, QString> VfsOff::updateMetadata(const SyncFileItem &item, const QString &filePath, const QString &replacesFile)
{
    Q_UNUSED(replacesFile)

    if (!item.isDirectory()) {
        const bool isReadOnly = !item._remotePerm.isNull() && !item._remotePerm.hasPermission(RemotePermissions::CanWrite);
        FileSystem::setFileReadOnlyWeak(filePath, isReadOnly);
    }
    return { ConvertToPlaceholderResult::Ok };
}

void VfsOff::onFileStatusChanged(const QString &, SyncFileStatus)
{
}
