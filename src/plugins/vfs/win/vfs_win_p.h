/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "vfs_win.h"
#include "utility.h"
#include "hydrationdevice.h"

#include "propagatedownload.h"
#include "common/syncjournalfilerecord.h"

#include <QThread>

namespace OCC {

/** Information that is passed between fetch-data and validate-data callbacks
 *
 * For example, when the validation callback is triggered, we need to know the
 * checksum header that was retrieved while downloading the file's data.
 */
class HydrationContext : public QObject
{
    Q_OBJECT
public:
    HydrationContext(VfsWin *parent);
    ~HydrationContext();

    // TODO: prefix member vars
    CfAPIUtil::CfOpdata opdata;
    QPointer<SyncJournalDb> journal;
    SyncJournalFileRecord record;
    QString filesystemPath;
    QByteArray transmissionChecksumHeader;
    PlaceholderHydrationDevice *hydrationDevice = nullptr;
    QPointer<GETFileJob> downloadJob;
    bool isDownloadDone = false;
    bool isAborted = false;

    void abort() {
        isAborted = true;
        if (downloadJob && downloadJob->reply()) {
            downloadJob->reply()->abort();
        }
    }

    VfsWin *pluginInstance() const
    {
        return qobject_cast<VfsWin*>(parent());
    }

public Q_SLOTS:
    /// Takes care of setting record._checksumHeader, then proceeds to validateTransmissionChecksum()
    void ensureContentChecksum(quint64 fileSize);

    /// Validates the transmission checksum and wraps up
    void validateTransmissionChecksum(quint64 fileSize);

    /// Extract important data from download job response, then signals downloadDone()
    void downloadJobDone();

    /// Send an error code to Windows
    void transferDataError(NTSTATUS status);

Q_SIGNALS:
    void downloadDone();
};

using TransferKey = quint64;

// This is also used as the call context when registering a sync root
class VfsWinPrivate : public QObject
{
    Q_OBJECT

public:
    VfsWinPrivate(VfsWin *q)
        : q(q)
        , _registrationState(RegistrationState::Unregistered)
    {
    }

    enum RegistrationState { Unregistered, Registering, FinishedRegistration };

    VfsWin *q;
    QAtomicInt _registrationState;
    CF_CONNECTION_KEY _connectionKey;

    // System.StorageProviderState key
    std::unique_ptr<PROPERTYKEY> storageProviderStateKey;

    std::wstring _registrationId;

    QScopedPointer<QThread> addToSearchIndexerThread;

    HydrationContext *hydrationContext(const TransferKey key)
    {
        return _hydrations.value(key);
    }

    void setHydrationContext(const TransferKey key, HydrationContext *context)
    {
        _hydrations.insert(key, context);
        connect(context, &HydrationContext::destroyed, this, [key, this] {
            _hydrations.remove(key);
        });
    }

    void registerFolder(const VfsSetupParams &params);

    void AddFolderToSearchIndexer(const QString& folder);

    // The path used for sync root registration
    QString syncRootPath() const;

    [[nodiscard]] Result<void, QString> createPlaceholderInternal(const SyncFileItem &item, const FILE_BASIC_INFO &basicInfo);

    // The following are triggered from callbacks
    void startFetchData(CfAPIUtil::CfOpdata opdata, const QString &targetPath, quint64 fetchStart, quint64 fetchEnd,
        const QByteArray &fileId);
    void cancelFetchData(TransferKey transferKey, const QString &path);
    void startValidateData(CfAPIUtil::CfOpdata opdata, const QString &path, quint64 offset, quint64 length,
        quint64 fileSize);

private:
    // Multiple hydrations can run at the same time and need to exchange data
    // between the fetch-data phase and the validate-data phase. They store their
    // info here. Must only be accessed from the gui thread!
    QHash<TransferKey, HydrationContext *> _hydrations;
};

namespace CfCallbackInfoHelper {

    QString getFullPath(const CF_CALLBACK_INFO *info, const wchar_t *path);
    QString getFullNormalizedPath(const CF_CALLBACK_INFO *info);

} // CfCallbackInfoHelper namespace
} // OCC namespace
