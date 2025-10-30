/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under GPL 2.0 or later.
 */
// SPDX-License-Identifier: GPL-2.0-or-later

// this include should come before every other include
#include "../src/plugins/vfs/win/cfapi_includes.h"

#include "../src/plugins/vfs/win/utility.h"
#include "../src/plugins/vfs/win/vfs_win.h"

#include "testutils/syncenginetestutils.h"
#include <syncengine.h>

#include <QtTest>

#include <chrono>

using namespace std::chrono_literals;

using namespace OCC;

SyncFileItemPtr findItem(const QSignalSpy &spy, const QString &path)
{
    for (const QList<QVariant> &args : spy) {
        auto item = args[0].value<SyncFileItemPtr>();
        if (item->destination() == path)
            return item;
    }
    return SyncFileItemPtr(new SyncFileItem);
}

bool itemInstruction(const QSignalSpy &spy, const QString &path, const SyncInstructions instr)
{
    auto item = findItem(spy, path);
    return item->instruction() == instr;
}

SyncJournalFileRecord dbRecord(FakeFolder &folder, const QString &path)
{
    SyncJournalFileRecord record;
    folder.syncJournal().getFileRecord(path, &record);
    return record;
}

void markForDownload(FakeFolder &folder, const QByteArray &path)
{
    auto &journal = folder.syncJournal();
    SyncJournalFileRecord record;
    journal.getFileRecord(path, &record);
    if (!record.isValid())
        return;
    record._type = ItemTypeVirtualFileDownload;
    journal.setFileRecord(record);
    journal.schedulePathForRemoteDiscovery(record._path);
}

bool isPlaceholder(const QString &path)
{
    auto info = CfAPIUtil::getInfo<CF_PLACEHOLDER_STANDARD_INFO>(path);
    return bool(info);
}

int64_t isPlaceholderWithOnDiskSize(const QString &path)
{
    auto info = CfAPIUtil::getInfo<CF_PLACEHOLDER_STANDARD_INFO>(path);
    if (!info) {
        return -1;
    }
    return info.get().OnDiskDataSize.QuadPart;
}

bool hydratePlaceholderSynchronously(const QString &path)
{
    bool hydrationOk = false;
    std::unique_ptr<QThread> thread(QThread::create([&]() {
        auto handle = CfAPIUtil::getFileHandle(path, 0);
        if (!handle)
            return;

        LARGE_INTEGER start;
        LARGE_INTEGER end;
        start.QuadPart = 0;
        end.QuadPart = MAXLONGLONG;
        const HRESULT ok = CfHydratePlaceholder(handle.get(), start, end, CF_HYDRATE_FLAG_NONE, nullptr);
        if (FAILED(ok)) {
            qWarning() << "CfHydratePlaceholder failed:" << Utility::formatWinError(ok);
            hydrationOk = false;
        } else {
            hydrationOk = true;
        }
    }));

    QSignalSpy spy(thread.get(), &QThread::finished);
    thread->start();
    OC_ENFORCE(spy.wait(std::chrono::duration_cast<std::chrono::milliseconds>(1min).count()));
    return hydrationOk;
}

// Like VfsWin::setPinState but with control over recursion flag
bool setPinState(FakeFolder &folder, const QString &path, CF_PIN_STATE state, CF_SET_PIN_FLAGS flag)
{
    QString fsPath = folder.localPath() + path;
    auto handle = CfAPIUtil::getFileHandle(fsPath, 0);
    if (!handle)
        return false;

    HRESULT ok = CfSetPinState(handle.get(), state, flag, nullptr);
    return ok == 0;
}


class TestWinVfs : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testPlaceholderCreationAndHydration()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };
        fakeFolder.account()->setCapabilities({fakeFolder.account()->url(), TestUtils::testCapabilities(CheckSums::Algorithm::SHA1)});

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), &SyncEngine::itemCompleted);
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        auto someDate = QDateTime(QDate(1984, 07, 30), QTime(1,3,2));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a2"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a3"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a4"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a5"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a6"), 0_B);
        fakeFolder.remoteModifier().setModTime(QStringLiteral("A/a1"), someDate);
        fakeFolder.remoteModifier().setModTime(QStringLiteral("A/a2"), someDate);
        fakeFolder.remoteModifier().setModTime(QStringLiteral("A/a3"), someDate);
        QString localA1 = fakeFolder.localPath() + QStringLiteral("A/a1");
        QString localA2 = fakeFolder.localPath() + QStringLiteral("A/a2");
        QString localA3 = fakeFolder.localPath() + QStringLiteral("A/a3");
        QString localA4 = fakeFolder.localPath() + QStringLiteral("A/a4");
        QString localA5 = fakeFolder.localPath() + QStringLiteral("A/a5");
        QString localA6 = fakeFolder.localPath() + QStringLiteral("A/a6");

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA1));
        QCOMPARE(QFileInfo(localA1).lastModified(), someDate);
        QCOMPARE(QFileInfo(localA1).size(), 5_MiB);
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/a1")));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_NEW));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a1"))._type, ItemTypeVirtualFile);
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("A/a6"))->isDehydratedPlaceholder);
        // we assume further placeholders are created correctly
        cleanup();

        // We listen to the Vfs hydration signals to check.

        //
        // Check that basic hydration works
        //
        OperationCounter counter(fakeFolder);
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA1));
        QVERIFY(hydratePlaceholderSynchronously(localA1));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(localA1));
        QCOMPARE(counter.nGET, 1);

        QCOMPARE(isPlaceholderWithOnDiskSize(localA1), 5_MiB);
        QCOMPARE(QFileInfo(localA1).lastModified(), someDate);
        QCOMPARE(QFileInfo(localA1).size(), 5_MiB);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a1"))._type, ItemTypeFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a1"))._checksumHeader, "SHA1:8886930866e6faf7558cda88305d92d13ee26cc9");
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a1")), PinState::Unspecified); // since it was an implicit hydration

        //
        // Is the remote etag propagated if it changed?
        //

        fakeFolder.remoteModifier().setContents(QStringLiteral("A/a2"), 5_MiB, 'M');

        counter.reset();
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA2));
        QVERIFY(hydratePlaceholderSynchronously(localA2));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(localA2));
        QCOMPARE(counter.nGET, 1);

        QCOMPARE(isPlaceholderWithOnDiskSize(localA2), 5_MiB);
        QCOMPARE(QFileInfo(localA2).lastModified(), someDate);
        QCOMPARE(QFileInfo(localA2).size(), 5_MiB);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a2"))._type, ItemTypeFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a2"))._etag, fakeFolder.remoteModifier().find(QStringLiteral("A/a2"))->etag);

        //
        // What if the remote file size changed?
        //

        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a3"));

        counter.reset();
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA3));
        QVERIFY(hydratePlaceholderSynchronously(localA3));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(localA3));
        QCOMPARE(counter.nGET, 2); // the download was restarted as we detected a size missmatch between the local and the remote size

        QCOMPARE(isPlaceholderWithOnDiskSize(localA3), 5_MiB + 1_B);
        QCOMPARE(QFileInfo(localA3).lastModified(), someDate);
        QCOMPARE(QFileInfo(localA3).size(), 5_MiB + 1_B);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a3"))._type, ItemTypeFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a3"))._etag, fakeFolder.remoteModifier().find(QStringLiteral("A/a3"))->etag);

        //
        // Check if a 0 (zero) byte file can be a) a placeholder before hydration,
        // and b) is considered *not* to be a placeholder after hydration.
        //
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("A/a6"))->isDehydratedPlaceholder);
        QVERIFY(hydratePlaceholderSynchronously(localA6));
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("A/a6"))); // check that the file/placeholder (still) exists
        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("A/a6"))->isDehydratedPlaceholder);

        //
        // Do network errors get signaled promptly?
        //

        fakeFolder.serverErrorPaths().append(QStringLiteral("A/a4"));
        QVERIFY(!hydratePlaceholderSynchronously(localA4));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA4));

        fakeFolder.serverErrorPaths().clear();
        QVERIFY(hydratePlaceholderSynchronously(localA4));

        //
        // Checksum validation errors are caught?
        //

        QByteArray checksumValue;
        QObject parent;
        fakeFolder.setServerOverride([&](QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *) -> QNetworkReply * {
            if (op == QNetworkAccessManager::GetOperation) {
                auto reply = new FakeGetReply(fakeFolder.remoteModifier(), op, request, &parent);
                if (!checksumValue.isNull())
                    reply->setRawHeader("OC-Checksum", checksumValue);
                return reply;
            }
            return nullptr;
        });

        checksumValue = "SHA1:bad";

        QVERIFY(!hydratePlaceholderSynchronously(localA5));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA5));

        // a second hydration attempt doesn't change anything
        QVERIFY(!hydratePlaceholderSynchronously(localA5));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA5));

        checksumValue = "MD5:b8fc857a25e7958868c2f003d5e0952d";
        QVERIFY(hydratePlaceholderSynchronously(localA5));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a5"))._checksumHeader, "SHA1:8886930866e6faf7558cda88305d92d13ee26cc9");

        fakeFolder.setServerOverride(nullptr);

        // everything is hydrated and in sync
        // this also checks the file contents (first byte at least)
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
    }

    // Normal hydration-through-sync still works
    void testSyncHydration()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a2"), 5_MiB, 'A');
        QString localA1 = fakeFolder.localPath() + QStringLiteral("A/a1");
        QString localA2 = fakeFolder.localPath() + QStringLiteral("A/a2");

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(localA1));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a1"))._type, ItemTypeVirtualFile);
        cleanup();

        //
        // Mark for download and check it works
        //

        markForDownload(fakeFolder, "A/a1");
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/a2"), PinState::AlwaysLocal));

        CF_FS_METADATA metadata;
        QVERIFY(CfAPIUtil::getFileMetadata(fakeFolder.localPath() + QStringLiteral("A/a2"), &metadata.BasicInfo));
        QVERIFY(metadata.BasicInfo.FileAttributes & FILE_ATTRIBUTE_PINNED);
        QVERIFY(!(metadata.BasicInfo.FileAttributes & FILE_ATTRIBUTE_UNPINNED));


        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a2")), PinState::AlwaysLocal);

        QCOMPARE(isPlaceholderWithOnDiskSize(localA1), 5_MiB);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a1"))._type, ItemTypeFile);
        QCOMPARE(isPlaceholderWithOnDiskSize(localA2), 5_MiB);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("A/a2"))._type, ItemTypeFile);
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_SYNC));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a2"), CSYNC_INSTRUCTION_SYNC));
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a1")), PinState::Unspecified); // since no attribute got propagated
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a2")), PinState::AlwaysLocal);
        cleanup();

        //
        // If remote changes get propagated, the pin state stays unchanged
        //

        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a1"));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a2"));
        QVERIFY(fakeFolder.syncOnce());
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a1")), PinState::Unspecified); // since no attribute got propagated
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("A/a2")), PinState::AlwaysLocal);
        cleanup();
    }

    // Placeholder moving and deleting gets propagated
    void testMoveDelete()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/b1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("f1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("q1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/d1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/d2"), 5_MiB, 'A');
        QVERIFY(fakeFolder.syncOnce());
        cleanup();

        fakeFolder.localModifier().rename(QStringLiteral("f1"), QStringLiteral("f2"));
        fakeFolder.localModifier().rename(QStringLiteral("A/a1"), QStringLiteral("a1"));
        fakeFolder.localModifier().remove(QStringLiteral("A/d1"));
        fakeFolder.remoteModifier().rename(QStringLiteral("q1"), QStringLiteral("q2"));
        fakeFolder.remoteModifier().rename(QStringLiteral("A/b1"), QStringLiteral("b1"));
        fakeFolder.remoteModifier().remove(QStringLiteral("A/d2"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("f2"), CSYNC_INSTRUCTION_RENAME));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("a1"), CSYNC_INSTRUCTION_RENAME));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("q2"), CSYNC_INSTRUCTION_RENAME));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("b1"), CSYNC_INSTRUCTION_RENAME));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/d1"), CSYNC_INSTRUCTION_REMOVE));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/d2"), CSYNC_INSTRUCTION_REMOVE));

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("f2"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("a1"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("q2"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("b1"))));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/d1"))));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/d2"))));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("f2"))._type, ItemTypeVirtualFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("a1"))._type, ItemTypeVirtualFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("q2"))._type, ItemTypeVirtualFile);
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("b1"))._type, ItemTypeVirtualFile);
        QVERIFY(!dbRecord(fakeFolder, QStringLiteral("A/d1")).isValid());
        QVERIFY(!dbRecord(fakeFolder, QStringLiteral("A/d2")).isValid());
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("f2")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("a1")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("A/d1")));
        cleanup();
    }

    // Placeholders update size/mtime if remote metadata changes
    void testMetadataUpdate()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        auto someDate = QDateTime(QDate(1984, 07, 30), QTime(1,3,2));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().setModTime(QStringLiteral("A/a1"), someDate);
        QVERIFY(fakeFolder.syncOnce());

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_NEW));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a1"))));
        QCOMPARE(QFileInfo(l(QStringLiteral("A/a1"))).size(), 5_MiB);
        QCOMPARE(QFileInfo(l(QStringLiteral("A/a1"))).lastModified(), someDate);
        cleanup();

        // Change the remote metadata and check whether it propagates

        auto otherDate = QDateTime(QDate(2000, 03, 03), QTime(2,5,4));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a1"));
        fakeFolder.remoteModifier().setModTime(QStringLiteral("A/a1"), otherDate);

        QVERIFY(fakeFolder.syncOnce());

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_UPDATE_METADATA));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a1"))));
        QCOMPARE(QFileInfo(l(QStringLiteral("A/a1"))).size(), 5_MiB + 1_B);
        QCOMPARE(QFileInfo(l(QStringLiteral("A/a1"))).lastModified(), otherDate);
        cleanup();

        // No spurious sync
        QVERIFY(fakeFolder.syncOnce());
        QVERIFY(completeSpy.isEmpty());
        cleanup();
    }

    // Various kinds of conflicts
    void testConflicts()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/localdelete"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/localreplace1"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/localreplace2"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/localreplace3"), 5_MiB, 'A');
        fakeFolder.remoteModifier().insert(QStringLiteral("A/localreplace4"), 5_MiB, 'A');
        QVERIFY(fakeFolder.syncOnce());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/localdelete"))));
        cleanup();

        // Now try remote/local conflicts

        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/localdelete"));
        // localreplace1/2 aren't actual conflicts, but switch out the placeholder in an
        // interesting way
        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/localreplace3"));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/localreplace4"));
        fakeFolder.localModifier().remove(QStringLiteral("A/localdelete"));
        fakeFolder.localModifier().remove(QStringLiteral("A/localreplace1"));
        fakeFolder.localModifier().remove(QStringLiteral("A/localreplace2"));
        fakeFolder.localModifier().remove(QStringLiteral("A/localreplace3"));
        fakeFolder.localModifier().remove(QStringLiteral("A/localreplace4"));
        fakeFolder.localModifier().insert(QStringLiteral("A/localreplace1"), 1_MiB, 'B');
        fakeFolder.localModifier().insert(QStringLiteral("A/localreplace2"), 5_MiB, 'A');
        fakeFolder.localModifier().insert(QStringLiteral("A/localreplace3"), 1_MiB, 'B');
        fakeFolder.localModifier().insert(QStringLiteral("A/localreplace4"), 5_MiB, 'A');

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/localdelete"), CSYNC_INSTRUCTION_NEW));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/localdelete"))));
        QCOMPARE(QFileInfo(l(QStringLiteral("A/localdelete"))).size(), 5_MiB + 1_B);

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/localreplace1"), CSYNC_INSTRUCTION_SYNC));
        // isn't a placeholder at all

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/localreplace2"), CSYNC_INSTRUCTION_SYNC)); // no sync?
        // isn't a placeholder at all

        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/localreplace3"), CSYNC_INSTRUCTION_CONFLICT));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/localreplace4"), CSYNC_INSTRUCTION_CONFLICT));

        cleanup();
    }

    // Check what happens if there's a rename and a pin/hydration change
    // Check SyncVirtualFiles::testRenameVirtual2 which does the same for suffix vfs
    void testRenameVirtual2()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };
        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        fakeFolder.remoteModifier().insert(QStringLiteral("case3"), 128_B, 'C');
        fakeFolder.remoteModifier().insert(QStringLiteral("case4"), 256_B, 'C');
        fakeFolder.remoteModifier().insert(QStringLiteral("case5"), 256_B, 'C');
        fakeFolder.remoteModifier().insert(QStringLiteral("case6"), 256_B, 'C');
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("case4"), PinState::AlwaysLocal));
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("case6"), PinState::AlwaysLocal));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("case3"))));
        QCOMPARE(isPlaceholderWithOnDiskSize(l(QStringLiteral("case4"))), 256);
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("case5"))));
        cleanup();

        // Case 1: foo -> bar (tested elsewhere)
        // Case 2: foo.oc -> bar.oc (tested elsewhere)

        // Case 3: foo -> bar (foo starts dehydrated, pin state changed to alwayslocal)
        fakeFolder.localModifier().rename(QStringLiteral("case3"), QStringLiteral("case3-rename"));
        QVERIFY(fakeFolder.applyLocalModificationsWithoutSync());
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("case3-rename"), PinState::AlwaysLocal));

        // Case 4: foo -> bar (foo starts hydrated, pin state changed to onlineonly)
        fakeFolder.localModifier().rename(QStringLiteral("case4"), QStringLiteral("case4-rename"));
        QVERIFY(fakeFolder.applyLocalModificationsWithoutSync());
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("case4-rename"), PinState::OnlineOnly));

        // Case 5: foo -> bar (foo starts dehydrated, db changed to ITVFDownload)
        fakeFolder.localModifier().rename(QStringLiteral("case5"), QStringLiteral("case5-rename"));
        markForDownload(fakeFolder, "case5");

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        // Case 3: the rename went though, pin state change still on file
        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("case3")));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("case3-rename"))));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("case3")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("case3-rename")));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("case3-rename"), CSYNC_INSTRUCTION_RENAME));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("case3-rename"))._type, ItemTypeVirtualFile);
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("case3-rename")), PinState::AlwaysLocal);

        // Case 4: the rename went though, pin state change still on file
        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("case4")));
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("case4-rename")));
        QCOMPARE(isPlaceholderWithOnDiskSize(l(QStringLiteral("case4-rename"))), 256);
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("case4")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("case4-rename")));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("case4-rename"), CSYNC_INSTRUCTION_RENAME));
        QCOMPARE(dbRecord(fakeFolder, QStringLiteral("case4-rename"))._type, ItemTypeFile);
        QCOMPARE(*fakeFolder.vfs()->pinState(QStringLiteral("case4-rename")), PinState::OnlineOnly);

        // at this point it'd be nice if the client had anotherSyncNeeded set, to
        // make sure the pin state change gets applied!

        // Case 5: the rename went though, hydration is forgotten
        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("case5")));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("case5-rename"))));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("case5")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("case5-rename")));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("case5-rename"), CSYNC_INSTRUCTION_RENAME));
        QVERIFY(dbRecord(fakeFolder, QStringLiteral("case5-rename"))._type == ItemTypeVirtualFile);
    }

    // Dehydration via sync works, when going through file attributes
    void testSyncDehydrationViaUnpin()
    {
        FakeFolder fakeFolder { FileInfo::A12_B12_C12_S12(), Vfs::WindowsCfApi };
        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));
        auto cleanup = [&]() {
            completeSpy.clear();
        };
        cleanup();

        //
        // Mark for dehydration and check
        //


        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/a1"), PinState::OnlineOnly));

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/a2"), PinState::OnlineOnly));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a2"));
        // expect: normal dehydration

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("B/b1"), PinState::OnlineOnly));
        fakeFolder.remoteModifier().remove(QStringLiteral("B/b1"));
        // expect: local removal

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("B/b2"), PinState::OnlineOnly));
        fakeFolder.remoteModifier().rename(QStringLiteral("B/b2"), QStringLiteral("B/b3"));
        // expect: B/b2 is renamed to B/b3, later maybe dehydration?

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("C/c1"), PinState::OnlineOnly));
        fakeFolder.localModifier().appendByte(QStringLiteral("C/c1"));
        // expect: no dehydration, upload of c1

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("C/c2"), PinState::OnlineOnly));
        fakeFolder.localModifier().appendByte(QStringLiteral("C/c2"));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("C/c2"));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("C/c2"));
        // expect: no dehydration, conflict

        // A new not-in-db file that's nevertheless unpinned
        fakeFolder.localModifier().insert(QStringLiteral("C/n"));
        QVERIFY(fakeFolder.applyLocalModificationsWithoutSync());
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("C/n"), PinState::OnlineOnly));
        // expect: uploaded, later maybe dehydration?

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a1"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a2"))));

        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("B/b1")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("B/b1")));
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("B/b1"), CSYNC_INSTRUCTION_REMOVE));

        QVERIFY(!fakeFolder.currentLocalState().find(QStringLiteral("B/b2")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("B/b2")));
        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("B/b3")))); // because the non-virtual was renamed
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("B/b3"), CSYNC_INSTRUCTION_RENAME));
        // TODO: b3 still has the UNPINNED state?

        QCOMPARE(isPlaceholderWithOnDiskSize(l(QStringLiteral("C/c1"))), 25);
        QCOMPARE(fakeFolder.currentRemoteState().find(QStringLiteral("C/c1"))->contentSize, 25);
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("C/c1"), CSYNC_INSTRUCTION_SYNC));

        QCOMPARE(isPlaceholderWithOnDiskSize(l(QStringLiteral("C/c2"))), 26);
        QCOMPARE(fakeFolder.currentRemoteState().find(QStringLiteral("C/c2"))->contentSize, 26);
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("C/c2"), CSYNC_INSTRUCTION_CONFLICT));

        QCOMPARE(isPlaceholderWithOnDiskSize(l(QStringLiteral("C/n"))), 64);
        QCOMPARE(fakeFolder.currentRemoteState().find(QStringLiteral("C/n"))->contentSize, 64);
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("C/n"), CSYNC_INSTRUCTION_NEW));
        // TODO: n still has UNPINNED state?
        cleanup();
    }

    void testWipePlaceholders()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };
        auto l = [&](const QString &p) { return QString(fakeFolder.localPath() + p); };

        // Create a baseline

        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A/B"));
        fakeFolder.remoteModifier().insert(QStringLiteral("f1"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a3"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/B/b1"));
        fakeFolder.localModifier().mkdir(QStringLiteral("A"));
        fakeFolder.localModifier().mkdir(QStringLiteral("A/B"));
        fakeFolder.localModifier().insert(QStringLiteral("f2"));
        fakeFolder.localModifier().insert(QStringLiteral("A/a2"));
        fakeFolder.localModifier().insert(QStringLiteral("A/B/b2"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("f1"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a1"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/a3"))));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(l(QStringLiteral("A/B/b1"))));

        // Make local changes to a3
        fakeFolder.localModifier().remove(QStringLiteral("A/a3"));
        fakeFolder.localModifier().insert(QStringLiteral("A/a3"), 100_B);
        QVERIFY(fakeFolder.applyLocalModificationsWithoutSync());

        fakeFolder.vfs()->wipeDehydratedVirtualFiles();
        fakeFolder.vfs()->stop();
        fakeFolder.vfs()->unregisterFolder();

        QVERIFY(!QFile::exists(l(QStringLiteral("f1"))));
        QVERIFY(!QFile::exists(l(QStringLiteral("A/a1"))));
        QVERIFY(QFile::exists(l(QStringLiteral("A/a3"))));
        QVERIFY(!QFile::exists(l(QStringLiteral("A/B/b1"))));

        // Check that syncing with vfs disabled is fine
        auto vfsOff = QSharedPointer<Vfs>(VfsPluginManager::instance().createVfsFromPlugin(Vfs::Off).release());
        QVERIFY(vfsOff);
        fakeFolder.switchToVfs(vfsOff);
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        auto conflicts = fakeFolder.syncJournal().conflictRecordPaths();
        QCOMPARE(conflicts.size(), 1);
        QFile::remove(l(QString::fromUtf8(conflicts[0])));
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
    }

    void testAvailability()
    {
        FakeFolder fakeFolder { FileInfo(), Vfs::WindowsCfApi, true };
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a2"));
        fakeFolder.syncOnce();
        QCOMPARE(*fakeFolder.vfs()->availability(QString()), VfsItemAvailability::AllDehydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A")), VfsItemAvailability::AllDehydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a1")), VfsItemAvailability::AllDehydrated);

        // The Vfs property is only accessible when it's not "sync pending"
        auto updateSyncState = [&] {
            fakeFolder.vfs()->fileStatusChanged(fakeFolder.localPath() + QStringLiteral("A/a1"), SyncFileStatus(SyncFileStatus::StatusUpToDate));
            fakeFolder.vfs()->fileStatusChanged(fakeFolder.localPath() + QStringLiteral("A/a2"), SyncFileStatus(SyncFileStatus::StatusUpToDate));
            fakeFolder.vfs()->fileStatusChanged(fakeFolder.localPath() + QStringLiteral("A"), SyncFileStatus(SyncFileStatus::StatusUpToDate));
        };

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A"), PinState::AlwaysLocal));
        fakeFolder.syncOnce();
        updateSyncState();
        QCOMPARE(*fakeFolder.vfs()->availability(QString()), VfsItemAvailability::AllHydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A")), VfsItemAvailability::AlwaysLocal);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a1")), VfsItemAvailability::AlwaysLocal);

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/a1"), PinState::Unspecified));
        fakeFolder.syncOnce();
        updateSyncState();
        QCOMPARE(*fakeFolder.vfs()->availability(QString()), VfsItemAvailability::AllHydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A")), VfsItemAvailability::AllHydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a1")), VfsItemAvailability::AllHydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a2")), VfsItemAvailability::AlwaysLocal);

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A"), PinState::OnlineOnly));
        fakeFolder.syncOnce();
        updateSyncState();
        QCOMPARE(*fakeFolder.vfs()->availability(QString()), VfsItemAvailability::AllDehydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A")), VfsItemAvailability::AllDehydrated);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a1")), VfsItemAvailability::AllDehydrated);

        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/a1"), PinState::AlwaysLocal));
        fakeFolder.syncOnce();
        updateSyncState();
        QCOMPARE(*fakeFolder.vfs()->availability(QString()), VfsItemAvailability::Mixed);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A")), VfsItemAvailability::Mixed);
        QCOMPARE(*fakeFolder.vfs()->availability(QStringLiteral("A/a1")), VfsItemAvailability::AlwaysLocal);
    }

    // Check that previously hydrated files become placeholders on sync
    void testConvertToPlaceholders()
    {
        FakeFolder fakeFolder { FileInfo {}, Vfs::Off };
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/a1"), 64_B);
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        fakeFolder.switchToVfs(QSharedPointer<Vfs>(new VfsWin));
        QSignalSpy completeSpy(&fakeFolder.syncEngine(), SIGNAL(itemCompleted(const SyncFileItemPtr &)));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_UPDATE_METADATA));
        QCOMPARE(isPlaceholderWithOnDiskSize(QString(fakeFolder.localPath() + QStringLiteral("A/a1"))), 64_B);

        completeSpy.clear();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QVERIFY(itemInstruction(completeSpy, QStringLiteral("A/a1"), CSYNC_INSTRUCTION_NONE));
        QCOMPARE(isPlaceholderWithOnDiskSize(QString(fakeFolder.localPath() + QStringLiteral("A/a1"))), 64_B);
    }

    void testIsPlaceholder()
    {
        FakeFolder fakeFolder = { FileInfo {}, Vfs::WindowsCfApi };
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/zeroBytesHydrated"), 0_B);
        fakeFolder.remoteModifier().insert(QStringLiteral("A/zeroBytesDeHydrated"), 0_B);
        fakeFolder.remoteModifier().insert(QStringLiteral("A/Hydrated"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/DeHydrated"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/zeroBytesHydrated"), PinState::AlwaysLocal));
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/zeroBytesDeHydrated"), PinState::OnlineOnly));
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/Hydrated"), PinState::AlwaysLocal));
        QVERIFY(fakeFolder.vfs()->setPinState(QStringLiteral("A/DeHydrated"), PinState::OnlineOnly));
        QVERIFY(fakeFolder.syncOnce());

        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(fakeFolder.localPath() + QStringLiteral("A/zeroBytesHydrated")));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(fakeFolder.localPath() + QStringLiteral("A/zeroBytesDeHydrated")));

        QVERIFY(!fakeFolder.vfs()->isDehydratedPlaceholder(fakeFolder.localPath() + QStringLiteral("A/Hydrated")));
        QVERIFY(fakeFolder.vfs()->isDehydratedPlaceholder(fakeFolder.localPath() + QStringLiteral("A/DeHydrated")));
    }
};

QTEST_GUILESS_MAIN(TestWinVfs)
#include "testwinvfs.moc"
