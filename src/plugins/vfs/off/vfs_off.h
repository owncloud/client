/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QScopedPointer>

#include "common/plugin.h"
#include "common/vfs.h"

namespace OCC {

class VfsOff : public Vfs
{
    Q_OBJECT

public:
    VfsOff(QObject *parent = nullptr);
    ~VfsOff() override;

    Mode mode() const override;

    void stop() override;
    void unregisterFolder() override;

    bool socketApiPinStateActionsShown() const override;

    Result<void, QString> createPlaceholder(const SyncFileItem &) override;

    bool needsMetadataUpdate(const SyncFileItem &) override;
    bool isDehydratedPlaceholder(const QString &) override;
    bool statTypeVirtualFile(csync_file_stat_t *, void *) override;

    bool setPinState(const QString &, PinState) override;
    Optional<PinState> pinState(const QString &) override;
    AvailabilityResult availability(const QString &) override;

public Q_SLOTS:
    void onFileStatusChanged(const QString &, SyncFileStatus) override;

protected:
    Result<ConvertToPlaceholderResult, QString> updateMetadata(const SyncFileItem &, const QString &, const QString &) override;
    void startImpl(const VfsSetupParams &) override;
};


class OffVfsPluginFactory : public QObject, public DefaultPluginFactory<VfsOff>
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.owncloud.PluginFactory" FILE "vfspluginmetadata.json")
    Q_INTERFACES(OCC::PluginFactory)
};

} // namespace OCC
