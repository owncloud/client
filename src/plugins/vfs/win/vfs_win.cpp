/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under GPL 2.0 or later.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

// this include should come before every other include
#include "cfapi_includes.h"

#include "vfs_win.h"
#include "vfs_win_p.h"


#include <QCoreApplication>
#include <QDir>
#include <QScopeGuard>
#include <QThread>

#include <filesystem>

#include <SearchAPI.h>    // needed for AddFolderToSearchIndexer

#include "account.h"
#include "common/checksums.h"
#include "common/result.h"
#include "common/utility.h"
#include "libsync/filesystem.h"
#include "owncloudpropagator_p.h" // for parseEtag()
#include "propagatedownload.h"
#include "propagatorjobs.h"

#include "csync.h"

#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.Provider.h>
#include <ppltasks.h>
#include <winerror.h>
#include <ShObjIdl_core.h>
#include <propsys.h>

#include "utility.h"
#include "validationdevice.h"
#include "hydrationdevice.h"
#include "callbacks.h"


using namespace OCC::FileSystem::SizeLiterals;

namespace {
const wchar_t* MSSearchIndex = L"SystemIndex";
}

Q_LOGGING_CATEGORY(lcVfs, "sync.vfs.win", QtDebugMsg)

using namespace OCC;
using namespace OCC::CfAPIUtil;

namespace {

std::wstring convertFullNativePathToFullyDecodedUrlW(const QString &path)
{
    // We assume that the path is in the form "c:\My Documents", so valid native path without any encoded characters.
    // Then the fully decoded URL is "file:///c:\My Documents".
    // Note: we are not using QUrl to first encode the path, and then fully-decode it. The problem with fully decoding
    //       some URLs is cannot be reliably represented as a fully-decoded string (see QUrl documentation). However,
    //       we already have a valid local path that is not encoded, we can use that directly.
    return QStringLiteral("file:///%1").arg(path).toStdWString();
}

// Tell windows that the hydration needs to be restarted, possibly with a new size
void restartHydration(const CfOpdata &opdata, const QString &filePath = {}, const qint64 &updatedFilesize = -1)
{
    auto opInfo = opdata.toOpinfo();
    opInfo.Type = CF_OPERATION_TYPE_RESTART_HYDRATION;

    CF_OPERATION_PARAMETERS opParams = {};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(RestartHydration);
    opParams.RestartHydration.Flags = CF_OPERATION_RESTART_HYDRATION_FLAG_NONE;

    // If we restart because there's a new size, send it along
    CF_FS_METADATA metadata;
    if (!filePath.isEmpty() && updatedFilesize != -1) {
        opParams.RestartHydration.FsMetadata = &metadata;
        metadata.FileSize.QuadPart = updatedFilesize;
        const auto result = getFileMetadata(filePath, &metadata.BasicInfo);
        if (!result) {
            qCWarning(lcVfs) << "CfExecute error on restart hydration" << Utility::formatWinError(result.error());
            return;
        }
    }

    HRESULT ok = CfExecute(&opInfo, &opParams);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << "CfExecute error on restart hydration" << Utility::formatWinError(ok);
        return;
    }
}

// Takes oldFile's pin state and sets it on newFile which is a new hydrated file
void propagatePinState(const QString &newFile, const QString &oldFile)
{
    const auto prevInfo = getInfo<CF_PLACEHOLDER_BASIC_INFO>(oldFile);
    if (!prevInfo) {
        if (prevInfo.error() != HRESULT_FROM_WIN32(ERROR_NOT_A_CLOUD_FILE))
            qCWarning(lcVfs) << Q_FUNC_INFO << ": couldn't get info for" << oldFile << Utility::formatWinError(prevInfo.error());
        return;
    }
    // It doesn't make sense to set EXCLUDED or UNPINNED on a new hydrated file
    CF_PIN_STATE state = prevInfo->PinState;
    if (state == CF_PIN_STATE_EXCLUDED || state == CF_PIN_STATE_UNPINNED)
        state = CF_PIN_STATE_UNSPECIFIED;

    const auto handle = getFileHandle(newFile, WRITE_DAC);
    OC_ASSERT(handle);
    const HRESULT ok = CfSetPinState(*handle, state, CF_SET_PIN_FLAG_NONE, nullptr);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << Q_FUNC_INFO << ": couldn't write pin state to file" << Utility::formatWinError(ok);
    }
}


OCC::Result<OCC::Vfs::ConvertToPlaceholderResult, QString> convertToPlaceholder(const QString &filename, const OCC::SyncFileItem &item, const QString &replacesFile)
{
    const QString longPath = FileSystem::longWinPath(filename);
    // Check whether it already is a placeholder
    auto info = getInfo<CF_PLACEHOLDER_BASIC_INFO>(longPath);
    if (info) {
        // already a cloud file
        return OCC::Vfs::ConvertToPlaceholderResult::Ok;
    } else if (info.error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        return OCC::Vfs::ConvertToPlaceholderResult::Ok;
    } else if (info.error() != HRESULT_FROM_WIN32(ERROR_NOT_A_CLOUD_FILE)) {
        qCWarning(lcVfs) << Q_FUNC_INFO << ": couldn't get info for" << filename << Utility::formatWinError(info.error());
    }

    // Get exclusive access
    Utility::Handle oplockHandle { INVALID_HANDLE_VALUE, &CfCloseHandle };
    HRESULT ok = CfOpenFileWithOplock(
        reinterpret_cast<const wchar_t *>(longPath.utf16()),
        CF_OPEN_FILE_FLAG_EXCLUSIVE,
        &oplockHandle.handle());
    if (FAILED(ok)) {
        const QString error = QStringLiteral("CfOpenWithOplock error %1 %2").arg(filename, Utility::formatWinError(ok));
        qCWarning(lcVfs) << error;
        if (ok == HRESULT_FROM_WIN32(ERROR_CANNOT_BREAK_OPLOCK) || ok == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION)) {
            return OCC::Vfs::ConvertToPlaceholderResult::Locked;
        }
        return error;
    }

    // Convert it
    USN usn;
    ok = CfConvertToPlaceholder(
        oplockHandle,
        item._fileId.constData(),
        item._fileId.size(),
        CF_CONVERT_FLAG_MARK_IN_SYNC,
        &usn,
        nullptr);
    if (FAILED(ok)) {
        const QString error = QStringLiteral("CfConvertToPlaceholder error %1 %2").arg(filename, Utility::formatWinError(ok));
        qCWarning(lcVfs) << error;
        return error;
    }

    // Propagate pin state, if any
    if (!replacesFile.isEmpty()) {
        propagatePinState(longPath, replacesFile);
    }
    return OCC::Vfs::ConvertToPlaceholderResult::Ok;
}

winrt::com_ptr<ISearchCrawlScopeManager> getSearchCrawlScopeManager()
{
    winrt::com_ptr<ISearchCrawlScopeManager> searchCrawlScopeManager;

    try {
        winrt::com_ptr<ISearchManager> searchManager;
        winrt::check_hresult(CoCreateInstance(__uuidof(CSearchManager), NULL, CLSCTX_SERVER, __uuidof(&searchManager), searchManager.put_void()));

        winrt::com_ptr<ISearchCatalogManager> searchCatalogManager;
        winrt::check_hresult(searchManager->GetCatalog(MSSearchIndex, searchCatalogManager.put()));

        winrt::check_hresult(searchCatalogManager->GetCrawlScopeManager(searchCrawlScopeManager.put()));
    } catch (...) {
        qCWarning(lcVfs) << "Failed to get CrawlScopeManager:" << Utility::formatWinError(winrt::to_hresult());
    }
    return searchCrawlScopeManager;
}
}

// Tell windows that the fetch_data request has failed
void HydrationContext::transferDataError(NTSTATUS status)
{
    auto opInfo = opdata.toOpinfo();
    opInfo.Type = CF_OPERATION_TYPE_TRANSFER_DATA;

    CF_OPERATION_PARAMETERS opParams = {};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(TransferData);
    opParams.TransferData.CompletionStatus = status;

    // the error affects the whole file, not a transfer chunk
    opParams.TransferData.Offset.QuadPart = 0;
    opParams.TransferData.Length.QuadPart = hydrationDevice->fileSize();
    assert_4_KiB_chunk(hydrationDevice->fileSize(), opParams.TransferData.Offset.QuadPart, opParams.TransferData.Length.QuadPart);

    HRESULT ok = CfExecute(&opInfo, &opParams);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << "CfExecute error on signaling fetch error" << Utility::formatWinError(ok);
        return;
    }
}

void VfsWinPrivate::startFetchData(CfOpdata opdata, const QString &targetPath, quint64 fetchStart, quint64 fetchEnd, const QByteArray &fileId)
{
    // Get account-relative path to file
    auto &params = q->params();
    auto journal = params.journal;
    SyncJournalFileRecord record;
    journal->getFileRecordsByFileId(fileId, [&](const SyncJournalFileRecord &r) {
        Q_ASSERT(!record.isValid());
        record = r;
        // TODO: How about multiple results?
    });
    if (!record.isValid()) {
        qCWarning(lcVfs) << "Failed to find record for" << fileId;
        return;
    }
    QString accountRelativePath = params.remotePath + QString::fromUtf8(record._path);

    QMap<QByteArray, QByteArray> headers;

    // Make sure we hydrate only with the expected amount of data: we get into
    // trouble if the remote and local file size don't match up: either Windows
    // will wait for more data that isn't coming, or Windows will reject us
    // feeding more data than it expects.
    if (static_cast<quint64>(record._fileSize) != fetchEnd) {
        qCInfo(lcVfs) << "DB size and placeholder size don't match up, restarting…"
                      << accountRelativePath << record._fileSize << fetchEnd;
        restartHydration(opdata, targetPath, record._fileSize);
        return;
    }

    auto hydrationContext = new HydrationContext(q);

    auto hydrationDevice = new PlaceholderHydrationDevice(opdata, fetchStart, fetchEnd);
    hydrationDevice->setParent(hydrationContext);
    hydrationDevice->open(QIODevice::WriteOnly);


    // We don't mind if we download a newer revision of the file: intentionally use empty etag
    auto getJob = new GETFileJob(params.account, params.baseUrl(), accountRelativePath, hydrationDevice, headers, {}, fetchStart, hydrationContext);
    getJob->setExpectedContentLength(fetchEnd - fetchStart);
    getJob->setPriority(QNetworkRequest::HighPriority);

    // TODO: This could be a resume where the hydration context already exists? Unsure.
    // this case would work as-is, but maybe it'd be good to keep track of retried hydrations.
    hydrationContext->journal = journal;
    hydrationContext->opdata = opdata;
    hydrationContext->record = record;
    hydrationContext->filesystemPath = targetPath; // ignore the filename from the db record, it might have been renamed (and we fetched the record based on the fileId).
    hydrationContext->hydrationDevice = hydrationDevice;
    hydrationContext->downloadJob = getJob;
    setHydrationContext(opdata.transferKey.QuadPart, hydrationContext);

    QObject::connect(getJob, &GETFileJob::finishedSignal, hydrationContext, &HydrationContext::downloadJobDone);
    getJob->start();
}

void HydrationContext::downloadJobDone()
{
    if (!journal) {
        qCWarning(lcVfs) << "journal was destroyed before hydration finished";
        delete this;
        return;
    }
    auto job = downloadJob;
    if (!job) {
        qCWarning(lcVfs) << "GET job destroyed before downloadJobDone called";
        delete this;
        return;
    }

    // The job is done and will deleteLater() itself. Also, we might wipe this
    // hydrationContext on error. If the job is parented to this it would be
    // deleted then and we'd call deleteLater() on a dead instance.
    downloadJob->setParent(nullptr);

    // Handle errors first:
    if (job->reply()->error() != QNetworkReply::NoError || job->errorStatus() != SyncFileItem::NoStatus) {
        if (isAborted) {
            qCDebug(lcVfs) << "GET job was aborted";
            return;
        }

        // TODO: more elaborate error handling needed
        qCInfo(lcVfs) << "GET job error" << record._path << "with error code" << job->httpStatusCode() << job->errorString();

        if (job->httpStatusCode() == 404) {
            qCDebug(lcVfs) << "Reporting file not in sync";
            transferDataError(STATUS_CLOUD_FILE_NOT_IN_SYNC);
            // TODO: schedule a sync
        } else if (job->contentLength() != -1 && job->contentLength() != job->expectedContentLength()) {
            Q_ASSERT(job->resumeStart() == 0);
            const auto newFileSize = job->resumeStart() + job->contentLength();
            if (newFileSize == 0 && record._fileSize != 0) {
                // the file might now be a folder
                qCWarning(lcVfs) << "Metadata appears to be outdated, abort hydration" << record._path;
                Q_EMIT pluginInstance()->needSync();
                transferDataError(STATUS_CLOUD_FILE_NOT_IN_SYNC);
            } else {
                qCInfo(lcVfs) << "Data size and placeholder size don't match up, restarting…" << record._path;
                record._fileSize = newFileSize;
                journal->setFileRecord(record);
                restartHydration(opdata, filesystemPath, record._fileSize);
            }
        } else {
            qCDebug(lcVfs) << "Reporting generic download failed";
            transferDataError(STATUS_CLOUD_FILE_UNSUCCESSFUL);
        }

        delete this;
        return;
    }

    // Download was successful!

    // Update etag and mtime, similar to PropagateDownload
    if (!job->etag().isEmpty()) {
        Q_ASSERT(job->etag() == Utility::normalizeEtag(job->etag()));
        record._etag = job->etag().toUtf8();
    }
    if (job->lastModified()) {
        record._modtime = job->lastModified();
    }

    // Find checksum header
    // TODO: Duplicated with PropagateDownload
    transmissionChecksumHeader = findBestChecksum(job->reply()->rawHeader(checkSumHeaderC));
    const auto contentMd5Header = job->reply()->rawHeader(contentMd5HeaderC);
    if (transmissionChecksumHeader.isEmpty() && !contentMd5Header.isEmpty()) {
        transmissionChecksumHeader = "MD5:" + contentMd5Header;
    }

    // signal that validation may start now
    isDownloadDone = true;
    Q_EMIT downloadDone();
    // TODO: maybe also start a timer, so we can release the hydration in case the
    // windows validation callback never fires?
}

void VfsWinPrivate::cancelFetchData(TransferKey transferKey, const QString &path)
{
    auto context = hydrationContext(transferKey);
    if (!context) {
        qCWarning(lcVfs) << "trying to cancel unknown hydration" << path << transferKey;
        return;
    }

    qCInfo(lcVfs) << "request to cancel hydration of" << context->filesystemPath;

    // Abort the download job
    context->abort();

    // It's possible it's being used from another thread currently
    delete context;
}

void VfsWinPrivate::startValidateData(CfOpdata opdata, const QString &path, quint64 offset, quint64 length, quint64 fileSize)
{
    auto context = hydrationContext(opdata.transferKey.QuadPart);
    if (!context) {
        // TODO: better error signaling?
        qCWarning(lcVfs) << "don't know about the hydration, restarting" << path;
        restartHydration(opdata);
        return;
    }
    context->opdata = opdata;

    if (offset != 0 || length != fileSize) {
        qCWarning(lcVfs) << "we don't support partial validation, validate the whole file";
    }

    // Make sure fetching is entirely done (including the finished() signal)
    // before going to the validation step
    auto continueValidation = [context, fileSize]() {
        context->ensureContentChecksum(fileSize);
    };
    if (context->isDownloadDone) {
        continueValidation();
    } else {
        QObject::connect(context, &HydrationContext::downloadDone, context, continueValidation);
    }
}

HydrationContext::HydrationContext(VfsWin *parent)
    : QObject(parent)
{

}

HydrationContext::~HydrationContext()
{
}

void HydrationContext::ensureContentChecksum(quint64 fileSize)
{
    const auto checksumType = pluginInstance()->params().account->capabilities().preferredUploadChecksumType();
    // If we can reuse the transmission checksum as the content checksum, skip ahead
    const auto checksumHeader = ChecksumHeader::parseChecksumHeader(transmissionChecksumHeader);
    if (!transmissionChecksumHeader.isEmpty() && checksumHeader.type() == checksumType) {
        record._checksumHeader = transmissionChecksumHeader;
        validateTransmissionChecksum(fileSize);
        return;
    }

    // Otherwise, compute the content checksum, then proceed

    auto device = std::make_unique<PlaceholderValidationDevice>(opdata, 0, fileSize);
    device->open(QIODevice::ReadOnly);

    auto compute = new ComputeChecksum(this);
    compute->setChecksumType(checksumType);
    connect(compute, &ComputeChecksum::done, this, [this, fileSize](CheckSums::Algorithm checksumType, const QByteArray &checksum) {
        record._checksumHeader = ChecksumHeader(checksumType, checksum).makeChecksumHeader();
        validateTransmissionChecksum(fileSize);
    });
    compute->start(std::move(device));
}

void HydrationContext::validateTransmissionChecksum(quint64 fileSize)
{
    auto opInfo = opdata.toOpinfo();
    opInfo.Type = CF_OPERATION_TYPE_ACK_DATA;
    opInfo.SyncStatus = nullptr; // can convey extra user-visible text through this

    CF_OPERATION_PARAMETERS opParams = {};
    opParams.ParamSize = CF_SIZE_OF_OP_PARAM(AckData);
    opParams.AckData.Flags = CF_OPERATION_ACK_DATA_FLAG_NONE;
    opParams.AckData.CompletionStatus = STATUS_SUCCESS;

    // we validate the whole file, not a singe chunk
    opParams.AckData.Offset.QuadPart = 0;
    opParams.AckData.Length.QuadPart = fileSize;
    assert_4_KiB_chunk(fileSize, opParams.AckData.Offset.QuadPart, opParams.AckData.Length.QuadPart);

    auto device = std::make_unique<PlaceholderValidationDevice>(opdata, 0, fileSize);
    device->open(QIODevice::ReadOnly);

    auto validator = new ValidateChecksumHeader(this);
    QObject::connect(validator, &ValidateChecksumHeader::validated, this, [this, opInfo, opParams](CheckSums::Algorithm checksumType, const QByteArray &checksum) mutable {
        Q_UNUSED(checksumType);
        Q_UNUSED(checksum);
        qCInfo(lcVfs) << "validation success:" << record._path;

        HRESULT ok = CfExecute(&opInfo, &opParams);
        if (FAILED(ok)) {
            qCWarning(lcVfs) << "CfExecute error" << Utility::formatWinError(ok);
            delete this;
            return;
        }

        // Set placeholder file mtime to db mtime
        FileSystem::setModTime(filesystemPath, record._modtime);

        // Update the inode, similar to SyncFileItem::toSyncJournalFileRecordWithInode()
        FileSystem::getInode(filesystemPath, &record._inode);

        // The file is no longer virtual
        record._type = ItemTypeFile;

        // Update db entry
        if (journal)
            journal->setFileRecord(record);

        pluginInstance()->fileStatusChanged(filesystemPath, SyncFileStatus::StatusUpToDate);
        delete this;
    });
    QObject::connect(validator, &ValidateChecksumHeader::validationFailed, this, [this, opInfo, opParams](const QString &msg) mutable {
        qCInfo(lcVfs) << "validation failed:" << record._path << msg;

        opParams.AckData.CompletionStatus = STATUS_CLOUD_FILE_VALIDATION_FAILED;
        HRESULT ok = CfExecute(&opInfo, &opParams);
        if (FAILED(ok)) {
            qCWarning(lcVfs) << "CfExecute error" << Utility::formatWinError(ok);
            delete this;
            return;
        }

        delete this;
    });
    validator->start(std::move(device), transmissionChecksumHeader);
}

VfsWin::VfsWin(QObject *parent)
    : Vfs(parent)
    , d_ptr(new VfsWinPrivate(this))
{
}

VfsWin::~VfsWin()
{
    if (d_ptr->addToSearchIndexerThread) {
        d_ptr->addToSearchIndexerThread->wait();
    }
}

Vfs::Mode VfsWin::mode() const
{
    return WindowsCfApi;
}

// the canonical sync root path
QString VfsWinPrivate::syncRootPath() const
{
    return QDir::toNativeSeparators(Utility::stripTrailingSlash(q->params().filesystemPath));
}

// Register as a sync provider with cfapi as well as with the explorer integration
void VfsWinPrivate::registerFolder(const VfsSetupParams &params)
{
    const QString syncRoot = syncRootPath();
    const auto syncRootW = syncRoot.toStdWString();
    _registrationId = [&params, &syncRoot] {
        QString id;

        // For the StorageProviderSyncRootManager the id seems to be the primary key.
        // There's probably a length limit though, so make reasonably sure folders have
        // unique ids by using a hash.
        auto hash = QCryptographicHash::hash(syncRoot.toUtf8(), QCryptographicHash::Sha1);
        QString sid = getSidAsString();

        if (sid.isEmpty()) {
            // Something went wrong with the secure token API. Fall back:
            id = params.providerName + QLatin1Char('_') + QString::fromUtf8(hash.toBase64());
        } else {
            // The format is described in: https://learn.microsoft.com/en-us/uwp/api/windows.storage.provider.storageprovidersyncrootinfo.id?view=winrt-22621#windows-storage-provider-storageprovidersyncrootinfo-id
            id = QStringLiteral("%1!%2!%3").arg(params.providerName, sid, QString::fromUtf8(hash.toBase64()));
        }

        return id.toStdWString();
    }();

    AddFolderToSearchIndexer(syncRoot);
    //
    // First set up common policy information for both
    //
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Storage::Provider;
    StorageProviderSyncRootInfo providerInfo;
    // The calls below are "putters". They set the values on the `providerInfo`, but they do no
    // additional processing. The "getters" have the same method name, but take no parameters.

    // hydrate fully when hydration is requested, before handing data to the user - also means validation will always see the whole file
    providerInfo.HydrationPolicy(StorageProviderHydrationPolicy::Full);
    providerInfo.HydrationPolicyModifier(
        StorageProviderHydrationPolicyModifier::ValidationRequired
        | StorageProviderHydrationPolicyModifier::AutoDehydrationAllowed);
    // the whole tree is always available, no dynamic ls queries
    providerInfo.PopulationPolicy(StorageProviderPopulationPolicy::AlwaysFull);

    // maybe more?
    providerInfo.InSyncPolicy(StorageProviderInSyncPolicy::FileLastWriteTime);

    // no hardlinks
    providerInfo.HardlinkPolicy(StorageProviderHardlinkPolicy::None);
    //
    // Windows::Storage::Provider registration
    //

    providerInfo.AllowPinning(true);

    // It's unclear what Context is for
    Streams::DataWriter contextWriter;
    contextWriter.WriteString(params.providerName.toStdWString());
    providerInfo.Context(contextWriter.DetachBuffer());

    const auto providerNameW = [params] {
        QString out = params.multipleAccountsRegistered ? tr("%1 - %2").arg(params.providerDisplayName, params.account->displayNameWithHost())
                                                        : params.providerDisplayName;
        if (params.remotePath != QLatin1Char('/')) {
            out = tr("%1 - %2").arg(out, params.remotePath);
        }
        return out.toStdWString();
    }();
    const auto providerVersionW = params.providerVersion.toString().toStdWString();
    providerInfo.DisplayNameResource(providerNameW.c_str());
    providerInfo.Version(providerVersionW.c_str());

    // A path to an icon resource for the custom state of a file or folder.
    const auto applicationPathW = QCoreApplication::instance()->applicationFilePath().toStdWString();
    providerInfo.IconResource(applicationPathW.c_str());

    // An identifier for the sync root.
    providerInfo.Id(_registrationId.c_str());

    // The protection mode of the sync root registration.
    providerInfo.ProtectionMode(StorageProviderProtectionMode::Unknown);

    // GUID that represents the ID of the storage provider.
    // This is a randomly generated guid.
    // Disabled because using the ProviderId getter/setter causes crashes
//    providerInfo.ProviderId(winrt::guid(
//            0x3210d540, 0xaa0f, 0x428b, {0x91, 0xd2, 0x04, 0xcf, 0x37, 0x9c, 0xea, 0xe8}));

    // A Uri to a cloud storage recycle bin.
    //providerInfo.RecycleBinUri(L"recycle bin");

    providerInfo.ShowSiblingsAsGroup(params.groupInSidebar());

    // Prepage the key from the shell property store we'll use to determine
    // availability.
    PROPERTYKEY key;
    auto hr = PSGetPropertyKeyFromName(L"System.StorageProviderState", &key);
    if (FAILED(hr)) {
        Q_EMIT q->error(tr("Could not find StorageProviderState property %1").arg(Utility::formatWinError(hr)));
        return;
    } else {
        storageProviderStateKey = std::make_unique<PROPERTYKEY>(key);
    }

    using namespace winrt::Windows::Foundation;
    auto getfolderop = StorageFolder::GetFolderFromPathAsync(syncRootW);
    _registrationState = RegistrationState::Registering;
    getfolderop.Completed([providerInfo, syncRoot, syncRootW, params, this](const IAsyncOperation<StorageFolder> &result, AsyncStatus status) {
        QScopeGuard markRegistrationEnded([this, syncRoot]() { _registrationState = RegistrationState::FinishedRegistration; });

        if (status != AsyncStatus::Completed) {
            qCWarning(lcVfs) << "Could not find StorageFolder for" << syncRoot << "error:" << Utility::formatWinError(result.ErrorCode());
            Q_EMIT q->error(tr("Could not find StorageFolder for %1 error: %2").arg(syncRoot, Utility::formatWinError(result.ErrorCode())));
            return;
        }

        StorageFolder f(nullptr);
        try {
            f = result.GetResults();
            providerInfo.Path(f);
        } catch (winrt::hresult_error const& ex) {
            qCWarning(lcVfs) << "Could not retrieve StorageFolder for" << syncRoot << "error:" << ex.code() << hstringToQString(ex.message());
            Q_EMIT q->error(tr("Could not retrieve StorageFolder for %1 %2 (0x%3)").arg(syncRoot, hstringToQString(ex.message()), QString::number(ex.code(), 16)));
            return;
        }

        // Code below this point cannot run in parallel: the calls below will first search for existing sync root
        // information, and as a second step it will register the new provider for the folder. If multiple
        // sync connections are being created in short order (e.g. when setting up a new account), it is possible
        // that a second call to `registerFolder` will start running this registration. This can result in
        // multiple side-bar entries in the windows explorer.
        static QMutex registrationMutex;
        QMutexLocker registrationLock(&registrationMutex);

        try {
            auto previousInfo = StorageProviderSyncRootManager::GetSyncRootInformationForFolder(f);
            const QString oldId = hstringToQString(previousInfo.Id());
            if (!oldId.isEmpty()) {
                const QString context = [&] {
                    auto cxt = previousInfo.Context();
                    if (cxt) {
                        return QString::fromUtf8(reinterpret_cast<char *>(cxt.data()), cxt.Length());
                    }
                    return QString {};
                }();

                qCInfo(lcVfs) << "Found a registered sync root for" << QJsonObject { //
                    { QStringLiteral("SyncRoot"), syncRoot }, { QStringLiteral("Id"), oldId }, //
                    { QStringLiteral("Path"), hstringToQString(previousInfo.Path().Path()) }, //
                    { QStringLiteral("DisplayName"), hstringToQString(previousInfo.DisplayNameResource()) }, //
                    { QStringLiteral("Version"), hstringToQString(previousInfo.Version()) }, //
                    { QStringLiteral("Context"), context }
                };
                if (providerInfo.Id() != previousInfo.Id()) {
                    qCInfo(lcVfs) << "New id" << hstringToQString(providerInfo.Id()) << "Old id" << oldId;
                    // old setups used owncloud for every branding
                    if (oldId.startsWith(QLatin1String("owncloud_")) || oldId.startsWith(params.providerName)) {
                        if (previousInfo.Path().Path() != providerInfo.Path().Path()) {
                            qCWarning(lcVfs) << "Path did not match" << hstringToQString(previousInfo.Path().Path()) << "!=" << hstringToQString(providerInfo.Path().Path());
                            Q_EMIT q->error(tr("%1 is managed by another sync client").arg(hstringToQString(previousInfo.Path().Path())));
                            return;
                        }
                    } else {
                        qCWarning(lcVfs) << "Id did not match" << hstringToQString(previousInfo.Id()) << "!=" << oldId;
                        Q_EMIT q->error(tr("The folder is used by a different client: %1").arg(hstringToQString(previousInfo.DisplayNameResource())));
                        return;
                    }
                    _registrationId = previousInfo.Id();
                    providerInfo.Id(previousInfo.Id());
                    qCInfo(lcVfs) << "Keeping legacy id" << _registrationId;
                }
                const QString oldVersion = hstringToQString(previousInfo.Version());
                if (QVersionNumber::fromString(oldVersion) > params.providerVersion) {
                    qCWarning(lcVfs) << "Downgrading the folder from" << oldVersion << "to" << params.providerVersion << "is not supported";
                    Q_EMIT q->error(tr("Downgrading the folder from %1 to %2 is not supported").arg(oldVersion, params.providerVersion.toString()));
                    return;
                }
            }
        } catch (winrt::hresult_error const& ex) {
            qCDebug(lcVfs) << "Could not check for previous StorageFolder for" << syncRoot << ":" << ex.code() << hstringToQString(ex.message());
        }

        try {
            StorageProviderSyncRootManager::Register(providerInfo);
        } catch (winrt::hresult_error const& ex) {
            qCWarning(lcVfs) << "Error registering StorageProvider for" << params.filesystemPath << QString::number(ex.code()) << hstringToQString(ex.message());
            Q_EMIT q->error(tr("Error registering StorageProvider for %1: %2 (0x%3)").arg(params.filesystemPath, hstringToQString(ex.message()), QString::number(ex.code(), 16)));
            return;
        }

        //
        // Connect to the sync root: Must be done before the process can interact with it
        // also registers callbacks
        //
        // clang-format off
        const CF_CALLBACK_REGISTRATION callbacks[] = {
            { CF_CALLBACK_TYPE_FETCH_DATA, &callbackFetchData },
            { CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, &callbackCancelFetchData },
            { CF_CALLBACK_TYPE_VALIDATE_DATA, &callbackValidateData },
            { CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE, &callbackNotifyDehydrate },
            { CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION, &callbackNotifyDehydrateCompletion },
            // If we supported on-demand directory population we'd need to properly implement these:
            // at the moment we generate the full directory tree
            { CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, &callbackFetchPlaceholders },
            { CF_CALLBACK_TYPE_CANCEL_FETCH_PLACEHOLDERS, &callbackCancelFetchPlaceholders },
            { CF_CALLBACK_TYPE_NOTIFY_RENAME, &callbackRename },
            // There are more interesting callbacks for delete, open, close
            CF_CALLBACK_REGISTRATION_END
        };
        // clang-format on

        HRESULT ok = CfConnectSyncRoot(
            syncRootW.c_str(),
            callbacks,
            this, // use VfsWinPrivate as callback context
            CF_CONNECT_FLAG_BLOCK_SELF_IMPLICIT_HYDRATION // don't let antivirus implicitly hydrate (or something, a bit unclear)
                | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH, // request that callbacks get the full path
            &_connectionKey);
        if (FAILED(ok)) {
            Q_EMIT q->error(tr("Unable to connect sync root: %1 error: %2").arg(syncRoot, Utility::formatWinError(ok)));
            return;
        }
        // We have not discovered what the status ends up being used for
        //CfUpdateSyncProviderStatus(d->_connectionKey, CF_PROVIDER_STATUS_IDLE);
        if (addToSearchIndexerThread) {
            addToSearchIndexerThread->wait();
        }
        Q_EMIT q->started();
    });
}

// If the local (client) folder where the cloud file placeholders are created
// is not under the User folder (i.e. Documents, Photos, etc), then it is required
// to add the folder to the Search Indexer. This is because the properties for
// the cloud file state/progress are cached in the indexer, and if the folder isn't
// indexed, attempts to get the properties on items will not return the expected values.
void VfsWinPrivate::AddFolderToSearchIndexer(const QString& folder)
{
    class Worker : public QThread
    {
    public:
        Worker(const QString &folder)
            : _folder(folder)
        {
        }

        void run() override
        {
            const std::wstring url(convertFullNativePathToFullyDecodedUrlW(_folder));

            winrt::com_ptr<ISearchCrawlScopeManager> searchCrawlScopeManager = getSearchCrawlScopeManager();

            if (searchCrawlScopeManager) {
                try {
                    winrt::check_hresult(searchCrawlScopeManager->AddDefaultScopeRule(url.data(), TRUE, FOLLOW_FLAGS::FF_INDEXCOMPLEXURLS));
                    winrt::check_hresult(searchCrawlScopeManager->SaveAll());

                    qCDebug(lcVfs) << "Successfully called AddFolderToSearchIndexer on" << url;
                } catch (...) {
                    // winrt::to_hresult() will eat the exception if it is a result of winrt::check_hresult,
                    // otherwise the exception will get rethrown and this method will crash out as it should
                    qCWarning(lcVfs) << "Failed on call to AddFolderToSearchIndexer for" << url << " with" << Utility::formatWinError(winrt::to_hresult());
                }
            }
        }

    private:
        QString _folder;
    };

    addToSearchIndexerThread.reset(new Worker(folder));
    addToSearchIndexerThread->start();
}

// Note: on Windows this is done asynchornously.
void VfsWin::startImpl(const VfsSetupParams &params)
{
    Q_D(VfsWin);
    d->registerFolder(params);
}

void VfsWin::stop()
{
    Q_D(VfsWin);

    // The last bit of registering a folder is done asynchronously. This might still
    // be pending or running, so make sure it has ended.
    // todo: #30
    QEventLoop wait;
    while (d->_registrationState == VfsWinPrivate::RegistrationState::Registering) {
        wait.processEvents(QEventLoop::AllEvents, 100);
    }

    CfDisconnectSyncRoot(d->_connectionKey);
}

void VfsWin::unregisterFolder()
{
    Q_D(VfsWin);
    using SyncRootManager = winrt::Windows::Storage::Provider::StorageProviderSyncRootManager;
    try {
        SyncRootManager::Unregister(d->_registrationId);
    } catch (winrt::hresult_error const &ex) {
        qCWarning(lcVfs) << "Could not Unregister() sync root:" << hstringToQString(ex.message());
    }

    // Remove the folder from the search index
    const std::wstring url(convertFullNativePathToFullyDecodedUrlW(d->syncRootPath()));
    winrt::com_ptr<ISearchCrawlScopeManager> searchCrawlScopeManager = getSearchCrawlScopeManager();

    if (searchCrawlScopeManager) {
        try {
            winrt::check_hresult(searchCrawlScopeManager->RemoveDefaultScopeRule(url.data()));
        } catch (...) {
            qCWarning(lcVfs) << "Failed to remove DefaultSearchScopeRule for" << url << "with" << Utility::formatWinError(winrt::to_hresult());
        }
    } else {
        qCWarning(lcVfs) << "Unable to get the SearchCrawlScopeManager to remove the path" << url;
    }
}

OCC::Result<OCC::Vfs::ConvertToPlaceholderResult, QString> VfsWin::updateMetadata(const SyncFileItem &item, const QString &filePath, const QString &replacesFile)
{
    {
        const auto result = convertToPlaceholder(filePath, item, replacesFile);
        if (!result || result.get() != OCC::Vfs::ConvertToPlaceholderResult::Ok) {
            return result;
        }
    }
    // File system metadata to be updated for the placeholder. Values of 0 for the metadata indicate there are no updates.
    CF_FS_METADATA metadata = {};
    if (item._direction == SyncFileItem::Down) {
        metadata.FileSize.QuadPart = item._size;
        Utility::UnixTimeToLargeIntegerFiletime(item._modtime, &metadata.BasicInfo.ChangeTime);
        metadata.BasicInfo.LastWriteTime = metadata.BasicInfo.ChangeTime;
        metadata.BasicInfo.LastAccessTime = metadata.BasicInfo.ChangeTime;

        if (!item.isDirectory()) {
            if (!item._remotePerm.isNull() && !item._remotePerm.hasPermission(RemotePermissions::CanWrite)) {
                CF_FS_METADATA metadataOld = {};
                auto result = getFileMetadata(filePath, &metadataOld.BasicInfo);
                if (!result) {
                    return OCC::Utility::formatWinError(result.error());
                }
                metadata.BasicInfo.FileAttributes = metadataOld.BasicInfo.FileAttributes & FILE_ATTRIBUTE_READONLY;
            }
        }
    }
    // get handle
    auto fileHandle = getFileHandle(filePath, WRITE_DAC);
    if (!fileHandle) {
        return OCC::Utility::formatWinError(fileHandle.error());
    }

    CF_UPDATE_FLAGS flags = CF_UPDATE_FLAG_NONE;
    // dehydrate the file and mark it as done
    if (!item.isDirectory() && item._type == ItemTypeVirtualFileDehydration) {
        flags |= CF_UPDATE_FLAG_MARK_IN_SYNC | CF_UPDATE_FLAG_DEHYDRATE;
    }

    const auto ok = CfUpdatePlaceholder(
        *fileHandle, &metadata,
        item._fileId.constData(), static_cast<DWORD>(item._fileId.size()), // fileid
        nullptr, 0, // no dehydration
        flags,
        nullptr, // no usn
        nullptr); // no overlapped

    if (FAILED(ok)) {
        const auto error = Utility::formatWinError(ok);
        qCWarning(lcVfs) << Q_FUNC_INFO << filePath << error;
        return error;
    }
    return ConvertToPlaceholderResult::Ok;
}

Result<void, QString> VfsWinPrivate::createPlaceholderInternal(const SyncFileItem &item, const FILE_BASIC_INFO &basicInfo)
{
    const auto path = std::filesystem::path(syncRootPath().toStdWString()) / item._file.toStdWString();
    const auto name = path.filename().wstring();
    const auto dirPath = path.parent_path().wstring();

    CF_PLACEHOLDER_CREATE_INFO placeholder = {};
    placeholder.RelativeFileName = name.data();
    placeholder.FsMetadata.FileSize.QuadPart = static_cast<LONGLONG>(item._size);
    placeholder.FsMetadata.BasicInfo = basicInfo;
    placeholder.FileIdentity = item._fileId.constData();
    placeholder.FileIdentityLength = static_cast<DWORD>(item._fileId.size());
    placeholder.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;
    // Result and CreateUsn are filled later

    DWORD nProcessed = 0;
    HRESULT ok = CfCreatePlaceholders(
        dirPath.data(),
        &placeholder,
        1,
        CF_CREATE_FLAG_NONE,
        &nProcessed);
    if (FAILED(ok)) {
        const auto error = QStringLiteral("CfCreatePlaceholders error %1 : %2 for %3").arg(Utility::formatWinError(ok), Utility::formatWinError(placeholder.Result), item._file);
        qCWarning(lcVfs) << error;
        return error;
    }

    // It's unclear how the PINNED/UNPINNED file attribute in FsMetadata.BasicInfo
    // interacts with the pin state that is inherited through the parent folder for
    // new files.
    // Since it never makes sense for a placeholder to inherit the PINNED attribute
    // from its parent we remove it here if necessary. Inheriting UNSPECIFIED or
    // UNPINNED makes sense.
    FILE_BASIC_INFO fileInfo;
    if (getFileMetadata(q->params().filesystemPath + item._file, &fileInfo)) {
        if (fileInfo.FileAttributes & FILE_ATTRIBUTE_PINNED) {
            if (!q->setPinState(item._file, PinState::Unspecified)) {
                return QStringLiteral("Failed to set pin state");
            }
        }
    }
    return {};
}

Result<void, QString> VfsWin::createPlaceholder(const SyncFileItem &item)
{
    Q_D(VfsWin);

    auto basicInfo = FILE_BASIC_INFO();
    basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    Utility::UnixTimeToLargeIntegerFiletime(item._modtime, &basicInfo.ChangeTime);
    basicInfo.LastAccessTime = basicInfo.ChangeTime;
    basicInfo.LastWriteTime = basicInfo.ChangeTime;
    basicInfo.CreationTime = basicInfo.ChangeTime;

    return d->createPlaceholderInternal(item, basicInfo);
}

bool VfsWin::needsMetadataUpdate(const SyncFileItem &item)
{
    QString fsPath = params().filesystemPath + item._file;
    auto handle = getFileHandle(fsPath, 0);
    if (!handle)
        return false;
    return !getInfo<CF_PLACEHOLDER_BASIC_INFO>(*handle);
}

bool VfsWin::isDehydratedPlaceholder(const QString &filePath)
{
    auto handle = getFileHandle(filePath, 0);
    if (!handle) {
        return false;
    }
    FILE_ATTRIBUTE_TAG_INFO info = {};
    if (!GetFileInformationByHandleEx(*handle, FileAttributeTagInfo, &info, sizeof(info))) {
        return false;
    }
    CF_PLACEHOLDER_STATE state = CfGetPlaceholderStateFromAttributeTag(info.FileAttributes, info.ReparseTag);
    if (state == CF_PLACEHOLDER_STATE_NO_STATES) {
        // This is not a placeholder
        return false;
    }
    // no partial data, for us that's a download in progress
    return state & (CF_PLACEHOLDER_STATE_PARTIAL | CF_PLACEHOLDER_STATE_PARTIALLY_ON_DISK);
}

bool VfsWin::statTypeVirtualFile(csync_file_stat_t *stat, void *stat_data)
{
    auto ffd = reinterpret_cast<WIN32_FIND_DATA *>(stat_data);

    // Directories always go though as directories, even if they are placeholder-directories
    if (ffd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        stat->type = ItemTypeDirectory;
        return true;
    }

    const CF_PLACEHOLDER_STATE placeholder = CfGetPlaceholderStateFromFindData(ffd);
    const bool pinBit = ffd->dwFileAttributes & FILE_ATTRIBUTE_PINNED;
    const bool unpinBit = ffd->dwFileAttributes & FILE_ATTRIBUTE_UNPINNED;
    if (placeholder & CF_PLACEHOLDER_STATE_PLACEHOLDER) {
        if (placeholder & CF_PLACEHOLDER_STATE_PARTIAL) {
            if (pinBit && !unpinBit) {
                stat->type = ItemTypeVirtualFileDownload;
            } else {
                stat->type = ItemTypeVirtualFile;
            }
        } else {
            if (unpinBit && !pinBit) {
                stat->type = ItemTypeVirtualFileDehydration;
            } else {
                stat->type = ItemTypeFile;
            }
        }
        return true;
    }
    return false;
}

bool VfsWin::setPinState(const QString &relFilePath, PinState state)
{
    const QString fsPath = params().filesystemPath + relFilePath;

    auto handle = getFileHandle(fsPath, WRITE_DAC);
    if (!handle) {
        if (state == PinState::Inherited && (handle.error() == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || handle.error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))) {
            // PropagateLocalRename calls the function for the no longer exisitng file path
            return true;
        }
        qCWarning(lcVfs) << Q_FUNC_INFO << ": could not make handle for" << fsPath << Utility::formatWinError(handle.error());
        return false;
    }

    CF_PIN_STATE cfState = CF_PIN_STATE_UNSPECIFIED;
    switch (state) {
    case PinState::Inherited:
        cfState = CF_PIN_STATE_INHERIT;
        break;
    case PinState::AlwaysLocal:
        cfState = CF_PIN_STATE_PINNED;
        break;
    case PinState::OnlineOnly:
        cfState = CF_PIN_STATE_UNPINNED;
        break;
    case PinState::Unspecified:
        cfState = CF_PIN_STATE_UNSPECIFIED;
        break;
    }

    const HRESULT ok = CfSetPinState(*handle, cfState, CF_SET_PIN_FLAG_RECURSE, nullptr);
    if (FAILED(ok)) {
        qCWarning(lcVfs) << Q_FUNC_INFO << ": could not set pin state on" << fsPath << Utility::formatWinError(ok);
        return false;
    }
    return true;
}

Optional<PinState> VfsWin::pinState(const QString &relFilePath)
{
    QString fsPath = params().filesystemPath + relFilePath;

    // TODO: These functions need tests

    FILE_BASIC_INFO fileInfo;
    if (!getFileMetadata(fsPath, &fileInfo)) {
        return {};
    }

    bool pinBit = fileInfo.FileAttributes & FILE_ATTRIBUTE_PINNED;
    bool unpinBit = fileInfo.FileAttributes & FILE_ATTRIBUTE_UNPINNED;
    if (pinBit && !unpinBit)
        return PinState::AlwaysLocal;
    if (unpinBit && !pinBit)
        return PinState::OnlineOnly;
    return PinState::Unspecified;
}

Vfs::AvailabilityResult VfsWin::availability(const QString &folderPath)
{
    Q_D(VfsWin);

    auto hydrationStatus = params().journal->hasHydratedOrDehydratedFiles(folderPath.toUtf8());
    if (!hydrationStatus)
        return AvailabilityError::DbError;
    if (hydrationStatus->hasDehydrated) {
        if (hydrationStatus->hasHydrated) {
            return VfsItemAvailability::Mixed;
        } else {
            // We don't have the info needed to ever mark something as OnlineOnly
            return VfsItemAvailability::AllDehydrated;
        }
    }
    if (!hydrationStatus->hasHydrated)
        return AvailabilityError::NoSuchItem;

    // Determine whether it's recursively pinned or not.
    // To do that, we query the shell property system, which has an indexer to determine
    // that fact for the icons that the Explorer shows.

    if (!d->storageProviderStateKey) {
        // If the key couldn't be retrieved there already was a warning during setup.
        return VfsItemAvailability::AllHydrated;
    }

    QString fsPath = params().filesystemPath + folderPath;
    // WARNING: The property system doesn't like the new paths that
    // FileSystem::longWinPath() would generate
    auto nFsPath = QDir::toNativeSeparators(fsPath).toStdWString();

    // Read the combined availability property from the shell property model
    // First, get a property store for the item
    IPropertyStore *pps = nullptr;
    auto hr = SHGetPropertyStoreFromParsingName(
        nFsPath.c_str(), NULL, GPS_BESTEFFORT | GPS_DELAYCREATION, IID_PPV_ARGS(&pps));
    if (FAILED(hr)) {
        qCWarning(lcVfs) << "Could not create property store for item"
                         << fsPath << Utility::formatWinError(hr);
        return VfsItemAvailability::AllHydrated;
    }
    auto releasePps = qScopeGuard([&] { pps->Release(); });

    // And then read and interpret the relevant property
    PROPVARIANT value = {};
    hr = pps->GetValue(*d->storageProviderStateKey, &value);
    if (FAILED(hr)) {
        qCWarning(lcVfs) << "Could not access StorageProviderState property"
                         << fsPath << Utility::formatWinError(hr);
        return VfsItemAvailability::AllHydrated;
    }
    if (value.vt == VT_UI4) {
        enum WinShellAvailabilities : unsigned short {
            WinShellAvailableLocally = 2,
            WinShellAlwaysAvailableLocally = 3,
        };

        if (value.uiVal == WinShellAlwaysAvailableLocally)
            return VfsItemAvailability::AlwaysLocal;
    } else if (value.vt != VT_EMPTY) {
        qCWarning(lcVfs) << "Unexpected StorageProviderState property type"
                         << fsPath << value.vt;
    }
    return VfsItemAvailability::AllHydrated;
}

static Result<CF_PLACEHOLDER_BASIC_INFO, HRESULT> getBasicPlaceholderInfo(const QString &systemFileName)
{
    auto handle = getFileHandle(systemFileName, 0);
    if (!handle) {
        return handle.error();
    }

    return getInfo<CF_PLACEHOLDER_BASIC_INFO>(*handle);
}

void VfsWin::fileStatusChanged(const QString &systemFileName, SyncFileStatus status)
{
    auto placeholderInfo = getBasicPlaceholderInfo(systemFileName);
    if (!placeholderInfo) {
        // This slot will often be called for nonexistant files, that's not an error.
        if (placeholderInfo.error() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || placeholderInfo.error() == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
            return;
        } else if (placeholderInfo.error() != HRESULT_FROM_WIN32(ERROR_NOT_A_CLOUD_FILE)) {
            qCWarning(lcVfs) << Q_FUNC_INFO << ": could not retrieve placeholder info for" << systemFileName
                             << Utility::formatWinError(placeholderInfo.error());
        }
        return;
    }

    // Check if something changed
    const bool isExcluded = status.tag() == SyncFileStatus::StatusExcluded;
    const bool wasExcluded = placeholderInfo->PinState == CF_PIN_STATE_EXCLUDED;
    const bool isInSync = status.tag() == SyncFileStatus::StatusUpToDate;
    const bool wasInSync = placeholderInfo->InSyncState == CF_IN_SYNC_STATE_IN_SYNC;

    if (isExcluded && wasExcluded) {
        // Still excluded, so nothing changed
        return;
    } else if (!isExcluded && !wasExcluded && isInSync == wasInSync) {
        // exclusion state the same (not excluded), and sync state the same, so nothing changed
        return;
    }
    // else: either the exclusion state or the in-sync state is not the same.

    // We need to write new metadata, so open a writable handle to the file
    auto handle = getFileHandle(systemFileName, WRITE_DAC);
    if (!handle) {
        qCCritical(lcVfs) << "Error setting file sync status: could not make writable handle for" << systemFileName << ":"
                          << Utility::formatWinError(handle.error());
        return;
    }

    // Set inclusion state:
    if (isExcluded != wasExcluded) {
        const CF_PIN_STATE newPinState = isExcluded ? CF_PIN_STATE_EXCLUDED : CF_PIN_STATE_INHERIT;
        const HRESULT res = CfSetPinState(*handle, newPinState, CF_SET_PIN_FLAG_RECURSE, nullptr);
        if (FAILED(res)) {
            qCCritical(lcVfs) << "Error setting sync exclusion state for" << systemFileName << ":" << Utility::formatWinError(res);
            return;
        }
    }

    if (isExcluded) {
        // We don't need to set the sync state for an excluded file
        return;
    }

    const CF_IN_SYNC_STATE newSyncState = isInSync ? CF_IN_SYNC_STATE_IN_SYNC : CF_IN_SYNC_STATE_NOT_IN_SYNC;
    const HRESULT res = CfSetInSyncState(*handle, newSyncState, CF_SET_IN_SYNC_FLAG_NONE, nullptr);
    if (FAILED(res)) {
        qCCritical(lcVfs) << "Error setting file in-sync state for" << systemFileName << ":" << Utility::formatWinError(res);
    }
}


QString CfCallbackInfoHelper::getFullPath(const CF_CALLBACK_INFO *info, const wchar_t *path)
{
    return QDir::cleanPath(QString::fromWCharArray(info->VolumeDosName) + QString::fromWCharArray(path));
}

QString CfCallbackInfoHelper::getFullNormalizedPath(const CF_CALLBACK_INFO *info)
{
    return CfCallbackInfoHelper::getFullPath(info, info->NormalizedPath);
}
