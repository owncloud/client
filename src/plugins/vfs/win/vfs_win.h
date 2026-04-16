/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under GPL 2.0 or later.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QScopedPointer>

#include "common/vfs.h"
#include "common/plugin.h"

namespace OCC {

class VfsWinPrivate;

class VfsWin : public Vfs
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VfsWin)
    const QScopedPointer<VfsWinPrivate> d_ptr;

public:
    explicit VfsWin(QObject *parent = nullptr);
    ~VfsWin();

    Mode mode() const override;

    void stop() override;
    void unregisterFolder() override;

    [[nodiscard]] bool socketApiPinStateActionsShown() const override { return false; }

    [[nodiscard]] Result<void, QString> createPlaceholder(const SyncFileItem &item) override;

    /// Returns true for files that aren't placeholders yet
    [[nodiscard]] bool needsMetadataUpdate(const SyncFileItem &item) override;

    [[nodiscard]] bool isDehydratedPlaceholder(const QString &filePath) override;
    [[nodiscard]] bool statTypeVirtualFile(csync_file_stat_t *stat, void *stat_data) override;

    // Pin states exist as file attributes on disk
    [[nodiscard]] bool setPinState(const QString &relFilePath, PinState state) override;
    [[nodiscard]] Optional<PinState> pinState(const QString &filePath) override;

    [[nodiscard]] AvailabilityResult availability(const QString &folderPath) override;

public Q_SLOTS:
    void fileStatusChanged(const QString &systemFileName, SyncFileStatus status) override;

protected:
    [[nodiscard]] Result<ConvertToPlaceholderResult, QString> updateMetadata(const SyncFileItem &item, const QString &filePath, const QString &replacesFile = {}) override;
    void startImpl(const VfsSetupParams &params) override;
};



} // namespace OCC
