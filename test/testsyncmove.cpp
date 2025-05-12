/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include "testutils/syncenginetestutils.h"
#include "testutils/testutils.h"

#include <QtTest>
#include <syncengine.h>
#include <theme.h>

using namespace OCC;

bool itemSuccessful(const ItemCompletedSpy &spy, const QString &path, const SyncInstructions instr)
{
    auto item = spy.findItem(path);
    return item->_status == SyncFileItem::Success && item->instruction() == instr;
}

bool itemConflict(const ItemCompletedSpy &spy, const QString &path)
{
    auto item = spy.findItem(path);
    return item->_status == SyncFileItem::Conflict && item->instruction() == CSYNC_INSTRUCTION_CONFLICT;
}

bool itemSuccessfulMove(const ItemCompletedSpy &spy, const QString &path)
{
    return itemSuccessful(spy, path, CSYNC_INSTRUCTION_RENAME);
}

QStringList findConflicts(const FileInfo &dir)
{
    QStringList conflicts;
    for (const auto &item : dir.children) {
        if (item.name.contains(QLatin1String("(conflicted copy"))) {
            conflicts.append(item.path());
        }
    }
    return conflicts;
}

bool expectAndWipeConflict(FileModifier &local, FileInfo state, const QString &path)
{
    PathComponents pathComponents(path);
    auto base = state.find(pathComponents.parentDirComponents());
    if (!base)
        return false;
    for (const auto &item : std::as_const(base->children)) {
        if (item.name.startsWith(pathComponents.fileName()) && item.name.contains(QLatin1String("(conflicted copy"))) {
            local.remove(item.path());
            return true;
        }
    }
    return false;
}

class TestSyncMove : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase_data()
    {
        QTest::addColumn<Vfs::Mode>("vfsMode");
        QTest::addColumn<bool>("filesAreDehydrated");

        QTest::newRow("Vfs::Off") << Vfs::Off << false;

        if (VfsPluginManager::instance().isVfsPluginAvailable(Vfs::WindowsCfApi)) {
            QTest::newRow("Vfs::WindowsCfApi dehydrated") << Vfs::WindowsCfApi << true;

            // TODO: the hydrated version will fail due to an issue in the winvfs plugin, so leave it disabled for now.
            // QTest::newRow("Vfs::WindowsCfApi hydrated") << Vfs::WindowsCfApi << false;
        } else if (Utility::isWindows()) {
            qWarning("Skipping Vfs::WindowsCfApi");
        }
    }

    void testRemoteChangeInMovedFolder()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        // issue #5192
        FakeFolder fakeFolder(FileInfo { QString(), { FileInfo { QStringLiteral("folder"), { FileInfo { QStringLiteral("folderA"), { { QStringLiteral("file.txt"), 400 } } }, QStringLiteral("folderB") } } } }, vfsMode, filesAreDehydrated);

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        // Edit a file in a moved directory.
        fakeFolder.remoteModifier().setContents(QStringLiteral("folder/folderA/file.txt"), FileInfo::DefaultFileSize, 'a');
        fakeFolder.remoteModifier().rename(QStringLiteral("folder/folderA"), QStringLiteral("folder/folderB/folderA"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        auto oldState = fakeFolder.currentLocalState();
        QVERIFY(oldState.find(QStringLiteral("folder/folderB/folderA/file.txt")));
        QVERIFY(!oldState.find(QStringLiteral("folder/folderA/file.txt")));

        // This sync should not remove the file
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(fakeFolder.currentLocalState(), oldState);
    }

    void testSelectiveSyncMovedFolder()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        // issue #5224
        FakeFolder fakeFolder(FileInfo { QString(), { FileInfo { QStringLiteral("parentFolder"), { FileInfo { QStringLiteral("subFolderA"), { { QStringLiteral("fileA.txt"), 400 } } }, FileInfo { QStringLiteral("subFolderB"), { { QStringLiteral("fileB.txt"), 400 } } } } } } }, vfsMode, filesAreDehydrated);

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        auto expectedServerState = fakeFolder.currentRemoteState();

        // Remove subFolderA with selectiveSync:
        fakeFolder.syncEngine().journal()->setSelectiveSyncList(SyncJournalDb::SelectiveSyncBlackList, {QStringLiteral("parentFolder/subFolderA/")});
        fakeFolder.syncEngine().journal()->schedulePathForRemoteDiscovery(QByteArrayLiteral("parentFolder/subFolderA/"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        {
            // Nothing changed on the server
            QCOMPARE(fakeFolder.currentRemoteState(), expectedServerState);
            // The local state should not have subFolderA
            auto remoteState = fakeFolder.currentRemoteState();
            remoteState.remove(QStringLiteral("parentFolder/subFolderA"));
            QCOMPARE(fakeFolder.currentLocalState(), remoteState);
        }

        // Rename parentFolder on the server
        fakeFolder.remoteModifier().rename(QStringLiteral("parentFolder"), QStringLiteral("parentFolderRenamed"));
        expectedServerState = fakeFolder.currentRemoteState();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        {
            QCOMPARE(fakeFolder.currentRemoteState(), expectedServerState);
            auto remoteState = fakeFolder.currentRemoteState();
            // The subFolderA should still be there on the server.
            QVERIFY(remoteState.find(QStringLiteral("parentFolderRenamed/subFolderA/fileA.txt")));
            // But not on the client because of the selective sync
            remoteState.remove(QStringLiteral("parentFolderRenamed/subFolderA"));
            QCOMPARE(fakeFolder.currentLocalState(), remoteState);
        }

        // Rename it again, locally this time.
        fakeFolder.localModifier().rename(QStringLiteral("parentFolderRenamed"), QStringLiteral("parentThirdName"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        {
            auto remoteState = fakeFolder.currentRemoteState();
            // The subFolderA should still be there on the server.
            QVERIFY(remoteState.find(QStringLiteral("parentThirdName/subFolderA/fileA.txt")));
            // But not on the client because of the selective sync
            remoteState.remove(QStringLiteral("parentThirdName/subFolderA"));
            QCOMPARE(fakeFolder.currentLocalState(), remoteState);

            expectedServerState = fakeFolder.currentRemoteState();
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync()); // This sync should do nothing
            QCOMPARE(completeSpy.count(), 0);

            QCOMPARE(fakeFolder.currentRemoteState(), expectedServerState);
            QCOMPARE(fakeFolder.currentLocalState(), remoteState);
        }
    }

    void testLocalMoveDetection()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        fakeFolder.account()->setCapabilities({fakeFolder.account()->url(), TestUtils::testCapabilities(CheckSums::Algorithm::ADLER32)});

        OperationCounter counter(fakeFolder);

        // For directly editing the remote checksum
        FileInfo &remoteInfo = fakeFolder.remoteModifier();
        // Simple move causing a remote rename
        fakeFolder.localModifier().rename(QStringLiteral("A/a1"), QStringLiteral("A/a1m"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);
        counter.reset();

        // Move-and-change, mtime+size, causing a upload and delete
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("A/a2"))->isDehydratedPlaceholder
            == filesAreDehydrated); // no-one touched it, so the hydration state should be the same as the initial state
        auto mt = fakeFolder.currentLocalState().find(QStringLiteral("A/a2"))->lastModified();
        QVERIFY(mt.toSecsSinceEpoch() + 1 < QDateTime::currentDateTime().toSecsSinceEpoch());
        fakeFolder.localModifier().rename(QStringLiteral("A/a2"), QStringLiteral("A/a2m"));
        fakeFolder.localModifier().setContents(QStringLiteral("A/a2m"), fakeFolder.remoteModifier().contentSize + 1, 'x');
        fakeFolder.localModifier().setModTime(QStringLiteral("A/a2m"), mt.addSecs(1));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QVERIFY(!fakeFolder.currentLocalState()
                     .find(QStringLiteral("A/a2m"))
                     ->isDehydratedPlaceholder); // We overwrote all data in the file, so whatever the state was before, it is no longer dehydrated
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        QCOMPARE(counter.nGET, filesAreDehydrated ? 1 : 0); // on winvfs, with a dehydrated file, the os will try to hydrate the file before we write to it. When the file is hydrated, it doesn't need to be fetched.
        QCOMPARE(counter.nMOVE, 0); // we cannot detect moves (and we didn't implement it yet in winvfs), so ...
        QCOMPARE(counter.nDELETE, 1); // ... the file just disappears, and ...
        QCOMPARE(counter.nPUT, 1); // ... another file (with just 1 byte difference) appears somewhere else. Coincidence.
        counter.reset();
        // Move-and-change, mtime+content only
        fakeFolder.localModifier().rename(QStringLiteral("B/b1"), QStringLiteral("B/b1m"));
        fakeFolder.localModifier().setContents(QStringLiteral("B/b1m"), FileModifier::DefaultFileSize, 'C');
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nDELETE, 1);
        counter.reset();

        // Move-and-change, size+content only
        auto mtime = fakeFolder.remoteModifier().find(QStringLiteral("B/b2"))->lastModified();
        fakeFolder.localModifier().rename(QStringLiteral("B/b2"), QStringLiteral("B/b2m"));
        fakeFolder.localModifier().appendByte(QStringLiteral("B/b2m"));
        fakeFolder.localModifier().setModTime(QStringLiteral("B/b2m"), mtime);
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        if (vfsMode == Vfs::Off) {
            QCOMPARE(counter.nGET, 0); // b2m is detected as a *new* file, so we don't need to fetch the contents
        } else {
            QCOMPARE(counter.nGET, 1); // b2 is accessed, so we get a callback to download the file
        }
        QCOMPARE(counter.nMOVE, 0); // content differs, so not a move
        QCOMPARE(counter.nPUT, 1); // upload b2m
        QCOMPARE(counter.nDELETE, 1); // delete b2
        counter.reset();

        // WinVFS handles this just fine.
        if (vfsMode == Vfs::Off) {
            // Move-and-change, content only -- c1 has no checksum, so we fail to detect this!
            // NOTE: This is an expected failure.
            mtime = fakeFolder.remoteModifier().find(QStringLiteral("C/c1"))->lastModified();
            auto size = fakeFolder.currentRemoteState().find(QStringLiteral("C/c1"))->contentSize;
            fakeFolder.localModifier().rename(QStringLiteral("C/c1"), QStringLiteral("C/c1m"));
            fakeFolder.localModifier().setContents(QStringLiteral("C/c1m"), size, 'C');
            fakeFolder.localModifier().setModTime(QStringLiteral("C/c1m"), mtime);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            QCOMPARE(counter.nPUT, 0);
            QCOMPARE(counter.nDELETE, 0);
            QVERIFY(fakeFolder.currentLocalState() != remoteInfo);
            counter.reset();

            // cleanup, and upload a file that will have a checksum in the db
            fakeFolder.localModifier().remove(QStringLiteral("C/c1m"));
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            QCOMPARE(counter.nDELETE, 1);
            counter.reset();
        } else {
            // no rename happened, nothing to clean up
        }

        fakeFolder.localModifier().insert(
            QStringLiteral("C/c3"), 13_B, 'E'); // 13, because c1 (and c2) have a size of 24 bytes, so this file is clearly different
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nDELETE, 0);
        counter.reset();

        // Move-and-change, content only, this time while having a checksum
        mtime = fakeFolder.remoteModifier().find(QStringLiteral("C/c3"))->lastModified();
        fakeFolder.localModifier().rename(QStringLiteral("C/c3"), QStringLiteral("C/c3m"));
        fakeFolder.localModifier().setContents(QStringLiteral("C/c3m"), FileModifier::DefaultFileSize, 'C');
        fakeFolder.localModifier().setModTime(QStringLiteral("C/c3m"), mtime);
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nDELETE, 1);
        QCOMPARE(fakeFolder.currentLocalState(), remoteInfo);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(remoteInfo));
        counter.reset();
    }

    void testDuplicateFileId_data()
    {
        QTest::addColumn<QString>("prefix");

        // There have been bugs related to how the original
        // folder and the folder with the duplicate tree are
        // ordered. Test both cases here.
        QTest::newRow("first ordering") << "O"; // "O" > "A"
        QTest::newRow("second ordering") << "0"; // "0" < "A"
    }

    // If the same folder is shared in two different ways with the same
    // user, the target user will see duplicate file ids. We need to make
    // sure the move detection and sync still do the right thing in that
    // case.
    void testDuplicateFileId()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);
        QFETCH(QString, prefix);

        if (filesAreDehydrated) {
            QSKIP("This test expects to be able to modify local files on disk, which does not work with dehydrated files.");
        }

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        auto &remote = fakeFolder.remoteModifier();

        remote.mkdir(QStringLiteral("A/W"));
        remote.insert(QStringLiteral("A/W/w1"));
        remote.mkdir(QStringLiteral("A/Q"));

        // Duplicate every entry in A under O/A
        remote.mkdir(prefix);
        remote.children[prefix].addChild(remote.children[QStringLiteral("A")]);

        // This already checks that the rename detection doesn't get
        // horribly confused if we add new files that have the same
        // fileid as existing ones
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        OperationCounter counter(fakeFolder);

        // Try a remote file move
        remote.rename(QStringLiteral("A/a1"), QStringLiteral("A/W/a1m"));
        remote.rename(prefix + QStringLiteral("/A/a1"), prefix + QStringLiteral("/A/W/a1m"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nGET, 0);

        // And a remote directory move
        remote.rename(QStringLiteral("A/W"), QStringLiteral("A/Q/W"));
        remote.rename(prefix + QStringLiteral("/A/W"), prefix + QStringLiteral("/A/Q/W"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nGET, 0);

        // Partial file removal (in practice, A/a2 may be moved to O/a2, but we don't care)
        remote.rename(prefix + QStringLiteral("/A/a2"), prefix + QStringLiteral("/a2"));
        remote.remove(QStringLiteral("A/a2"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nGET, 0);

        // Local change plus remote move at the same time
        fakeFolder.localModifier().appendByte(prefix + QStringLiteral("/a2"));
        remote.rename(prefix + QStringLiteral("/a2"), prefix + QStringLiteral("/a3"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nGET, 1);
        counter.reset();

        // remove localy, and remote move at the same time
        fakeFolder.localModifier().remove(QStringLiteral("A/Q/W/a1m"));
        remote.rename(QStringLiteral("A/Q/W/a1m"), QStringLiteral("A/Q/W/a1p"));
        remote.rename(prefix + QStringLiteral("/A/Q/W/a1m"), prefix + QStringLiteral("/A/Q/W/a1p"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nGET, 1);
        counter.reset();
    }

    void testMovePropagation()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        auto &local = fakeFolder.localModifier();
        auto &remote = fakeFolder.remoteModifier();

        OperationCounter counter(fakeFolder);

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        counter.reset();

        // Move
        {
            local.rename(QStringLiteral("A/a1"), QStringLiteral("A/a1m"));
            remote.rename(QStringLiteral("B/b1"), QStringLiteral("B/b1m"));
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            QCOMPARE(counter.nGET, 0);
            QCOMPARE(counter.nPUT, 0);
            QCOMPARE(counter.nMOVE, 1);
            QCOMPARE(counter.nDELETE, 0);
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("A/a1m")));
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("B/b1m")));
            QCOMPARE(completeSpy.findItem(QStringLiteral("A/a1m"))->_file, QStringLiteral("A/a1"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("A/a1m"))->_renameTarget, QStringLiteral("A/a1m"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("B/b1m"))->_file, QStringLiteral("B/b1"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("B/b1m"))->_renameTarget, QStringLiteral("B/b1m"));
            counter.reset();
        }

        // Touch+Move on same side
        local.rename(QStringLiteral("A/a2"), QStringLiteral("A/a2m"));
        local.setContents(QStringLiteral("A/a2m"), FileModifier::DefaultFileSize, 'A');
        remote.rename(QStringLiteral("B/b2"), QStringLiteral("B/b2m"));
        remote.setContents(QStringLiteral("B/b2m"), FileModifier::DefaultFileSize, 'A');
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 1);
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nDELETE, 1);
        QCOMPARE(remote.find(QStringLiteral("A/a2m"))->contentChar, 'A');
        QCOMPARE(remote.find(QStringLiteral("B/b2m"))->contentChar, 'A');
        counter.reset();

        // Touch+Move on opposite sides
        local.rename(QStringLiteral("A/a1m"), QStringLiteral("A/a1m2"));
        remote.setContents(QStringLiteral("A/a1m"), FileModifier::DefaultFileSize, 'B');
        remote.rename(QStringLiteral("B/b1m"), QStringLiteral("B/b1m2"));
        local.setContents(QStringLiteral("B/b1m"), FileModifier::DefaultFileSize, 'B');

        if (filesAreDehydrated) {
            // When a placeholder is accessed, a hydration request is made. This will fail, because
            // the file (b1m) is no longer available on the server. We then return an error, so the
            // helper will fail to do the local modifications.
            QVERIFY(!fakeFolder.applyLocalModificationsAndSync());
            QSKIP("Behaviour for dehydrated files is different at this point compared to no-VFS");
        }

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 2);
        QCOMPARE(counter.nPUT, 2);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nDELETE, 0);

        // All these files existing afterwards is debatable. Should we propagate
        // the rename in one direction and grab the new contents in the other?
        // Currently there's no propagation job that would do that, and this does
        // at least not lose data.
        QVERIFY(remote.find(QStringLiteral("A/a1m"))->contentChar == 'B');
        QVERIFY(remote.find(QStringLiteral("B/b1m"))->contentChar == 'B');
        QVERIFY(remote.find(QStringLiteral("A/a1m2"))->contentChar == 'W');
        QVERIFY(remote.find(QStringLiteral("B/b1m2"))->contentChar == 'W');
        QCOMPARE(remote.find(QStringLiteral("A/a1m"))->contentChar, 'B');
        QCOMPARE(remote.find(QStringLiteral("B/b1m"))->contentChar, 'B');
        QCOMPARE(remote.find(QStringLiteral("A/a1m2"))->contentChar, 'W');
        QCOMPARE(remote.find(QStringLiteral("B/b1m2"))->contentChar, 'W');
        counter.reset();

        // Touch+create on one side, move on the other
        {
            local.appendByte(QStringLiteral("A/a1m"));
            local.insert(QStringLiteral("A/a1mt"));
            remote.rename(QStringLiteral("A/a1m"), QStringLiteral("A/a1mt"));
            remote.appendByte(QStringLiteral("B/b1m"));
            remote.insert(QStringLiteral("B/b1mt"));
            local.rename(QStringLiteral("B/b1m"), QStringLiteral("B/b1mt"));
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            // First check the counters:
            QCOMPARE(counter.nGET, 3);
            QCOMPARE(counter.nPUT, 1);
            QCOMPARE(counter.nMOVE, 0);
            QCOMPARE(counter.nDELETE, 0);
            // Ok, now we can remove the conflicting files. This needs disk access, so it might trigger server interaction. (Hence checking the counters before we do this.)
            QVERIFY(expectAndWipeConflict(local, fakeFolder.currentLocalState(), QStringLiteral("A/a1mt")));
            QVERIFY(expectAndWipeConflict(local, fakeFolder.currentLocalState(), QStringLiteral("B/b1mt")));
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            // Now we can compare the clean-up states:
            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
            QVERIFY(itemSuccessful(completeSpy, QStringLiteral("A/a1m"), CSYNC_INSTRUCTION_NEW));
            QVERIFY(itemSuccessful(completeSpy, QStringLiteral("B/b1m"), CSYNC_INSTRUCTION_NEW));
            QVERIFY(itemConflict(completeSpy, QStringLiteral("A/a1mt")));
            QVERIFY(itemConflict(completeSpy, QStringLiteral("B/b1mt")));
            counter.reset();
        }

        // Create new on one side, move to new on the other
        {
            local.insert(QStringLiteral("A/a1N"), 13_B);
            remote.rename(QStringLiteral("A/a1mt"), QStringLiteral("A/a1N"));
            remote.insert(QStringLiteral("B/b1N"), 13_B);
            local.rename(QStringLiteral("B/b1mt"), QStringLiteral("B/b1N"));
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            // First check the counters:
            QCOMPARE(counter.nGET, 2);
            QCOMPARE(counter.nPUT, 0);
            QCOMPARE(counter.nMOVE, 0);
            QCOMPARE(counter.nDELETE, 1);
            // Ok, now we can remove the conflicting files. This needs disk access, so it might trigger server interaction. (Hence checking the counters before we do this.)
            QVERIFY(expectAndWipeConflict(local, fakeFolder.currentLocalState(), QStringLiteral("A/a1N")));
            QVERIFY(expectAndWipeConflict(local, fakeFolder.currentLocalState(), QStringLiteral("B/b1N")));
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            // Now we can compare the clean-up states:
            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
            QVERIFY(itemSuccessful(completeSpy, QStringLiteral("A/a1mt"), CSYNC_INSTRUCTION_REMOVE));
            QVERIFY(itemSuccessful(completeSpy, QStringLiteral("B/b1mt"), CSYNC_INSTRUCTION_REMOVE));
            QVERIFY(itemConflict(completeSpy, QStringLiteral("A/a1N")));
            QVERIFY(itemConflict(completeSpy, QStringLiteral("B/b1N")));
            counter.reset();
        }

        // Local move, remote move
        local.rename(QStringLiteral("C/c1"), QStringLiteral("C/c1mL"));
        remote.rename(QStringLiteral("C/c1"), QStringLiteral("C/c1mR"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        // end up with both files
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 1);
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nDELETE, 0);

        // Rename/rename conflict on a folder
        counter.reset();
        remote.rename(QStringLiteral("C"), QStringLiteral("CMR"));
        local.rename(QStringLiteral("C"), QStringLiteral("CML"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        // End up with both folders
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 3); // 3 files in C
        QCOMPARE(counter.nPUT, 3);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nDELETE, 0);
        counter.reset();

        // Folder move
        {
            local.rename(QStringLiteral("A"), QStringLiteral("AM"));
            remote.rename(QStringLiteral("B"), QStringLiteral("BM"));
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
            QCOMPARE(counter.nGET, 0);
            QCOMPARE(counter.nPUT, 0);
            QCOMPARE(counter.nMOVE, 1);
            QCOMPARE(counter.nDELETE, 0);
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("AM")));
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("BM")));
            QCOMPARE(completeSpy.findItem(QStringLiteral("AM"))->_file, QStringLiteral("A"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("AM"))->_renameTarget, QStringLiteral("AM"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("BM"))->_file, QStringLiteral("B"));
            QCOMPARE(completeSpy.findItem(QStringLiteral("BM"))->_renameTarget, QStringLiteral("BM"));
            counter.reset();
        }

        // Folder move with contents touched on the same side
        {
            local.setContents(QStringLiteral("AM/a2m"), FileModifier::DefaultFileSize, 'C');
            // We must change the modtime for it is likely that it did not change between sync.
            // (Previous version of the client (<=2.5) would not need this because it was always doing
            // checksum comparison for all renames. But newer version no longer does it if the file is
            // renamed because the parent folder is renamed)
            local.setModTime(QStringLiteral("AM/a2m"), QDateTime::currentDateTimeUtc().addDays(3));
            local.rename(QStringLiteral("AM"), QStringLiteral("A2"));
            remote.setContents(QStringLiteral("BM/b2m"), FileModifier::DefaultFileSize, 'C');
            remote.rename(QStringLiteral("BM"), QStringLiteral("B2"));
            ItemCompletedSpy completeSpy(fakeFolder);
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
            QCOMPARE(counter.nGET, 1);
            QCOMPARE(counter.nPUT, 1);
            QCOMPARE(counter.nMOVE, 1);
            QCOMPARE(counter.nDELETE, 0);
            QCOMPARE(remote.find(QStringLiteral("A2/a2m"))->contentChar, 'C');
            QCOMPARE(remote.find(QStringLiteral("B2/b2m"))->contentChar, 'C');
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("A2")));
            QVERIFY(itemSuccessfulMove(completeSpy, QStringLiteral("B2")));
            counter.reset();
        }

        // Folder rename with contents touched on the other tree
        remote.setContents(QStringLiteral("A2/a2m"), FileModifier::DefaultFileSize, 'D');
        // setContents alone may not produce updated mtime if the test is fast
        // and since we don't use checksums here, that matters.
        remote.appendByte(QStringLiteral("A2/a2m"));
        local.rename(QStringLiteral("A2"), QStringLiteral("A3"));
        local.setContents(QStringLiteral("B2/b2m"), FileModifier::DefaultFileSize, 'D');
        local.appendByte(QStringLiteral("B2/b2m"));
        remote.rename(QStringLiteral("B2"), QStringLiteral("B3"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 1);
        QCOMPARE(counter.nPUT, 1);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(remote.find(QStringLiteral("A3/a2m"))->contentChar, 'D');
        QCOMPARE(remote.find(QStringLiteral("B3/b2m"))->contentChar, 'D');
        counter.reset();

        // Folder rename with contents touched on both ends
        remote.setContents(QStringLiteral("A3/a2m"), FileModifier::DefaultFileSize, 'R');
        remote.appendByte(QStringLiteral("A3/a2m"));
        local.setContents(QStringLiteral("A3/a2m"), FileModifier::DefaultFileSize, 'L');
        local.appendByte(QStringLiteral("A3/a2m"));
        local.appendByte(QStringLiteral("A3/a2m"));
        local.rename(QStringLiteral("A3"), QStringLiteral("A4"));
        remote.setContents(QStringLiteral("B3/b2m"), FileModifier::DefaultFileSize, 'R');
        remote.appendByte(QStringLiteral("B3/b2m"));
        local.setContents(QStringLiteral("B3/b2m"), FileModifier::DefaultFileSize, 'L');
        local.appendByte(QStringLiteral("B3/b2m"));
        local.appendByte(QStringLiteral("B3/b2m"));
        remote.rename(QStringLiteral("B3"), QStringLiteral("B4"));
        QThread::sleep(1); // This test is timing-sensitive. No idea why, it's probably the modtime on the client side.
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        qDebug() << counter;
        QCOMPARE(counter.nGET, 2);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);
        auto currentLocal = fakeFolder.currentLocalState();
        auto conflicts = findConflicts(currentLocal.children[QStringLiteral("A4")]);
        QCOMPARE(conflicts.size(), 1);
        for (const auto &c : std::as_const(conflicts)) {
            QCOMPARE(currentLocal.find(c)->contentChar, 'L');
            local.remove(c);
        }
        conflicts = findConflicts(currentLocal.children[QStringLiteral("B4")]);
        QCOMPARE(conflicts.size(), 1);
        for (const auto &c : std::as_const(conflicts)) {
            QCOMPARE(currentLocal.find(c)->contentChar, 'L');
            local.remove(c);
        }
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(remote.find(QStringLiteral("A4/a2m"))->contentChar, 'R');
        QCOMPARE(remote.find(QStringLiteral("B4/b2m"))->contentChar, 'R');
        counter.reset();

        // Rename a folder and rename the contents at the same time
        local.rename(QStringLiteral("A4/a2m"), QStringLiteral("A4/a2m2"));
        local.rename(QStringLiteral("A4"), QStringLiteral("A5"));
        remote.rename(QStringLiteral("B4/b2m"), QStringLiteral("B4/b2m2"));
        remote.rename(QStringLiteral("B4"), QStringLiteral("B5"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 2);
        QCOMPARE(counter.nDELETE, 0);
    }

    // These renames can be troublesome on windows
    void testRenameCaseOnly()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        auto &local = fakeFolder.localModifier();
        auto &remote = fakeFolder.remoteModifier();

        OperationCounter counter(fakeFolder);

        local.rename(QStringLiteral("A/a1"), QStringLiteral("A/A1"));
        remote.rename(QStringLiteral("A/a2"), QStringLiteral("A/A2"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), remote);
        QCOMPARE(printDbData(fakeFolder.dbState()), printDbData(fakeFolder.currentRemoteState()));
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);
    }

    // Check interaction of moves with file type changes
    void testMoveAndTypeChange()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        auto &local = fakeFolder.localModifier();
        auto &remote = fakeFolder.remoteModifier();

        // Touch on one side, rename and mkdir on the other
        {
            local.appendByte(QStringLiteral("A/a1"));
            remote.rename(QStringLiteral("A/a1"), QStringLiteral("A/a1mq"));
            remote.mkdir(QStringLiteral("A/a1"));
            remote.appendByte(QStringLiteral("B/b1"));
            local.rename(QStringLiteral("B/b1"), QStringLiteral("B/b1mq"));
            local.mkdir(QStringLiteral("B/b1"));
            ItemCompletedSpy completeSpy(fakeFolder);
            if (filesAreDehydrated) {
                // the dehydrating the placeholder failed as the metadata is out of sync
                QSignalSpy spy(fakeFolder.vfs().get(), &Vfs::needSync);
                QVERIFY(!fakeFolder.applyLocalModificationsAndSync());
                QVERIFY(spy.count() == 1);
                QVERIFY(fakeFolder.syncOnce());
                QVERIFY(fakeFolder.applyLocalModificationsAndSync());
                QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            } else {
                QVERIFY(fakeFolder.applyLocalModificationsAndSync());
                QCOMPARE(findConflicts(fakeFolder.currentLocalState().children[QStringLiteral("A")]).size(), 1);
                QCOMPARE(findConflicts(fakeFolder.currentLocalState().children[QStringLiteral("B")]).size(), 1);
            }
        }
    }

    // https://github.com/owncloud/client/issues/6629#issuecomment-402450691
    // When a file is moved and the server mtime was not in sync, the local mtime should be kept
    void testMoveAndMTimeChange()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        OperationCounter counter(fakeFolder);

        // Changing the mtime on the server (without invalidating the etag)
        fakeFolder.remoteModifier().find(QStringLiteral("A/a1"))->setLastModified(QDateTime::currentDateTime().addSecs(-50000));
        fakeFolder.remoteModifier().find(QStringLiteral("A/a2"))->setLastModified(QDateTime::currentDateTime().addSecs(-40000));

        // Move a few files
        fakeFolder.remoteModifier().rename(QStringLiteral("A/a1"), QStringLiteral("A/a1_server_renamed"));
        fakeFolder.localModifier().rename(QStringLiteral("A/a2"), QStringLiteral("A/a2_local_renamed"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);

        // Another sync should do nothing
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 1);
        QCOMPARE(counter.nDELETE, 0);

        // Check that everything other than the mtime is still equal:
        QVERIFY(fakeFolder.currentLocalState().equals(fakeFolder.currentRemoteState(), FileInfo::IgnoreLastModified));
    }

    // Test for https://github.com/owncloud/client/issues/6694
    void testInvertFolderHierarchy()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A/Empty"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A/Empty/Foo"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("C/AllEmpty"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("C/AllEmpty/Bar"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/Empty/f1"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/Empty/Foo/f2"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("C/AllEmpty/f3"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("C/AllEmpty/Bar/f4"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        OperationCounter counter(fakeFolder);

        // "Empty" is after "A", alphabetically
        fakeFolder.localModifier().rename(QStringLiteral("A/Empty"), QStringLiteral("Empty"));
        fakeFolder.localModifier().rename(QStringLiteral("A"), QStringLiteral("Empty/A"));

        // "AllEmpty" is before "C", alphabetically
        fakeFolder.localModifier().rename(QStringLiteral("C/AllEmpty"), QStringLiteral("AllEmpty"));
        fakeFolder.localModifier().rename(QStringLiteral("C"), QStringLiteral("AllEmpty/C"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);

        // Now, the revert, but "crossed"
        fakeFolder.localModifier().rename(QStringLiteral("Empty/A"), QStringLiteral("A"));
        fakeFolder.localModifier().rename(QStringLiteral("AllEmpty/C"), QStringLiteral("C"));
        fakeFolder.localModifier().rename(QStringLiteral("Empty"), QStringLiteral("C/Empty"));
        fakeFolder.localModifier().rename(QStringLiteral("AllEmpty"), QStringLiteral("A/AllEmpty"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);

        // Reverse on remote
        fakeFolder.remoteModifier().rename(QStringLiteral("A/AllEmpty"), QStringLiteral("AllEmpty"));
        fakeFolder.remoteModifier().rename(QStringLiteral("C/Empty"), QStringLiteral("Empty"));
        fakeFolder.remoteModifier().rename(QStringLiteral("C"), QStringLiteral("AllEmpty/C"));
        fakeFolder.remoteModifier().rename(QStringLiteral("A"), QStringLiteral("Empty/A"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
    }

    void testDeepHierarchy_data()
    {
        QTest::addColumn<bool>("local");
        QTest::newRow("remote") << false;
        QTest::newRow("local") << true;
    }

    void testDeepHierarchy()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);
        QFETCH(bool, local);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        FileModifier &modifier = local ? fakeFolder.localModifier() : fakeFolder.remoteModifier();

        modifier.mkdir(QStringLiteral("FolA"));
        modifier.mkdir(QStringLiteral("FolA/FolB"));
        modifier.mkdir(QStringLiteral("FolA/FolB/FolC"));
        modifier.mkdir(QStringLiteral("FolA/FolB/FolC/FolD"));
        modifier.mkdir(QStringLiteral("FolA/FolB/FolC/FolD/FolE"));
        modifier.insert(QStringLiteral("FolA/FileA.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FileB.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FileC.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FolD/FileD.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FolD/FolE/FileE.txt"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        OperationCounter counter(fakeFolder);

        modifier.insert(QStringLiteral("FolA/FileA2.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FileB2.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FileC2.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FolD/FileD2.txt"));
        modifier.insert(QStringLiteral("FolA/FolB/FolC/FolD/FolE/FileE2.txt"));
        modifier.rename(QStringLiteral("FolA"), QStringLiteral("FolA_Renamed"));
        modifier.rename(QStringLiteral("FolA_Renamed/FolB"), QStringLiteral("FolB_Renamed"));
        modifier.rename(QStringLiteral("FolB_Renamed/FolC"), QStringLiteral("FolA"));
        modifier.rename(QStringLiteral("FolA/FolD"), QStringLiteral("FolA/FolD_Renamed"));
        modifier.mkdir(QStringLiteral("FolB_Renamed/New"));
        modifier.rename(QStringLiteral("FolA/FolD_Renamed/FolE"), QStringLiteral("FolB_Renamed/New/FolE"));
        auto expected = local ? fakeFolder.currentLocalState() : fakeFolder.currentRemoteState();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(counter.nDELETE, local ? 1 : 0); // FolC was is renamed to an existing name, so it is not considered as renamed
        // There was 5 inserts
        QCOMPARE(counter.nGET, local || filesAreDehydrated ? 0 : 5);
        QCOMPARE(counter.nPUT, local ? 5 : 0);
    }

    void renameOnBothSides()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        OperationCounter counter(fakeFolder);

        // Test that renaming a file within a directory that was renamed on the other side actually do a rename.

        // 1) move the folder alphabeticaly before
        fakeFolder.remoteModifier().rename(QStringLiteral("A/a1"), QStringLiteral("A/a1m"));
        fakeFolder.localModifier().rename(QStringLiteral("A"), QStringLiteral("_A"));
        fakeFolder.localModifier().rename(QStringLiteral("B/b1"), QStringLiteral("B/b1m"));
        fakeFolder.remoteModifier().rename(QStringLiteral("B"), QStringLiteral("_B"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentRemoteState(), fakeFolder.currentRemoteState());
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("_A/a1m")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("_B/b1m")));
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 2);
        counter.reset();

        // 2) move alphabetically after
        fakeFolder.remoteModifier().rename(QStringLiteral("_A/a2"), QStringLiteral("_A/a2m"));
        fakeFolder.localModifier().rename(QStringLiteral("_B/b2"), QStringLiteral("_B/b2m"));
        fakeFolder.localModifier().rename(QStringLiteral("_A"), QStringLiteral("S/A"));
        fakeFolder.remoteModifier().rename(QStringLiteral("_B"), QStringLiteral("S/B"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentRemoteState(), fakeFolder.currentRemoteState());
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("S/A/a2m")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("S/B/b2m")));
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nPUT, 0);
        QCOMPARE(counter.nMOVE, 2);
    }

    void moveFileToDifferentFolderOnBothSides()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        OperationCounter counter(fakeFolder);

        QCOMPARE(fakeFolder.currentLocalState().find(QStringLiteral("B/b1"))->isDehydratedPlaceholder, filesAreDehydrated);
        QCOMPARE(fakeFolder.currentLocalState().find(QStringLiteral("B/b2"))->isDehydratedPlaceholder, filesAreDehydrated);

        // Test that moving a file within to different folder on both side does the right thing.

        fakeFolder.remoteModifier().rename(QStringLiteral("B/b1"), QStringLiteral("A/b1"));
        fakeFolder.localModifier().rename(QStringLiteral("B/b1"), QStringLiteral("C/b1"));

        fakeFolder.localModifier().rename(QStringLiteral("B/b2"), QStringLiteral("A/b2"));
        fakeFolder.remoteModifier().rename(QStringLiteral("B/b2"), QStringLiteral("C/b2"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentRemoteState(), fakeFolder.currentRemoteState());
        // The easy checks: the server always has the data, so it can successfully move the files:
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/b1")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("C/b2")));
        // Either the client has hydrated files, in which case it will upload the data to the target locations;
        // or the files were dehydrated, so it has to remove the files. (No data-loss in the latter case: the
        // files were dehydrated, so there was no data anyway.)
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("C/b1")) || filesAreDehydrated);
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/b2")) || filesAreDehydrated);

        QCOMPARE(counter.nMOVE, 0); // Unfortunately, we can't really make a move in this case
        QCOMPARE(counter.nGET, filesAreDehydrated ? 0 : 2);
        QCOMPARE(counter.nPUT, filesAreDehydrated ? 0 : 2);
        QCOMPARE(counter.nDELETE, 0);
        counter.reset();
    }

    // Test that deletes don't run before renames
    void testRenameParallelism()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder({ FileInfo {} }, vfsMode, filesAreDehydrated);
        fakeFolder.remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("A/file"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        {
            auto localState = fakeFolder.currentLocalState();
            FileInfo *localFile = localState.find(QStringLiteral("A/file"));
            QVERIFY(localFile != nullptr); // check if the file exists
            QCOMPARE(vfsMode == Vfs::Off || localFile->isDehydratedPlaceholder, vfsMode == Vfs::Off || filesAreDehydrated);

            auto remoteState = fakeFolder.currentRemoteState();
            FileInfo *remoteFile = remoteState.find(QStringLiteral("A/file"));
            QVERIFY(remoteFile != nullptr);
            QCOMPARE(localFile->lastModified(), remoteFile->lastModified());

            QCOMPARE(localState, remoteState);
        }

        fakeFolder.localModifier().mkdir(QStringLiteral("B"));
        fakeFolder.localModifier().rename(QStringLiteral("A/file"), QStringLiteral("B/file"));
        fakeFolder.localModifier().remove(QStringLiteral("A"));
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        {
            auto localState = fakeFolder.currentLocalState();
            QVERIFY(localState.find(QStringLiteral("A/file")) == nullptr); // check if the file is gone
            QVERIFY(localState.find(QStringLiteral("A")) == nullptr); // check if the directory is gone
            FileInfo *localFile = localState.find(QStringLiteral("B/file"));
            QVERIFY(localFile != nullptr); // check if the file exists
            QCOMPARE(vfsMode == Vfs::Off || localFile->isDehydratedPlaceholder, vfsMode == Vfs::Off || filesAreDehydrated);

            auto remoteState = fakeFolder.currentRemoteState();
            FileInfo *remoteFile = remoteState.find(QStringLiteral("B/file"));
            QVERIFY(remoteFile != nullptr);
            QCOMPARE(localFile->lastModified(), remoteFile->lastModified());

            QCOMPARE(localState, remoteState);
        }
    }

    void testMovedWithError_data()
    {
        QTest::addColumn<Vfs::Mode>("vfsMode");

        QTest::newRow("Vfs::Off") << Vfs::Off;
        QTest::newRow("Vfs::WithSuffix") << Vfs::WithSuffix;
#ifdef Q_OS_WIN32
        if (VfsPluginManager::instance().isVfsPluginAvailable(Vfs::WindowsCfApi)) {
            QTest::newRow("Vfs::WindowsCfApi") << Vfs::WindowsCfApi;
        } else {
            qWarning("Skipping Vfs::WindowsCfApi");
        }
#endif
    }

    void testMovedWithError()
    {
        QFETCH(Vfs::Mode, vfsMode);
        const auto getName = [vfsMode] (const QString &s)
        {
            if (vfsMode == Vfs::WithSuffix)
            {
                return QStringLiteral("%1%2").arg(s, Theme::instance()->appDotVirtualFileSuffix());
            }
            return s;
        };
        const QString src = QStringLiteral("folder/folderA/file.txt");
        const QString dest = QStringLiteral("folder/folderB/file.txt");
        FakeFolder fakeFolder{ FileInfo{ QString(), { FileInfo{ QStringLiteral("folder"), { FileInfo{ QStringLiteral("folderA"), { { QStringLiteral("file.txt"), 400 } } }, QStringLiteral("folderB") } } } } };
        auto syncOpts = fakeFolder.syncEngine().syncOptions();
        syncOpts._parallelNetworkJobs = 0;
        fakeFolder.syncEngine().setSyncOptions(syncOpts);

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        if (vfsMode != Vfs::Off)
        {
            auto vfs = QSharedPointer<Vfs>(VfsPluginManager::instance().createVfsFromPlugin(vfsMode).release());
            QVERIFY(vfs);
            fakeFolder.switchToVfs(vfs);
            fakeFolder.syncJournal().internalPinStates().setForPath("", PinState::OnlineOnly);

            // make files virtual
            QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        }


        fakeFolder.serverErrorPaths().append(src, 403);
        fakeFolder.localModifier().rename(getName(src), getName(dest));
        QVERIFY(fakeFolder.currentRemoteState().find(src));
        QVERIFY(!fakeFolder.currentRemoteState().find(dest));

        // sync1 file gets detected as error, instruction is still NEW_FILE
        QVERIFY(!fakeFolder.applyLocalModificationsAndSync());

        // sync2 file is in error state, checkErrorBlacklisting sets instruction to IGNORED
        QVERIFY(!fakeFolder.applyLocalModificationsAndSync());

        if (vfsMode != Vfs::Off)
        {
            fakeFolder.syncJournal().internalPinStates().setForPath("", PinState::AlwaysLocal);
            QVERIFY(!fakeFolder.applyLocalModificationsAndSync());
        }

        QVERIFY(!fakeFolder.currentLocalState().find(src));
        QVERIFY(fakeFolder.currentLocalState().find(getName(dest)));
        if (vfsMode == Vfs::WithSuffix)
        {
            // the placeholder was not restored as it is still in error state
            QVERIFY(!fakeFolder.currentLocalState().find(dest));
        }
        QVERIFY(fakeFolder.currentRemoteState().find(src));
        QVERIFY(!fakeFolder.currentRemoteState().find(dest));
    }

};

QTEST_GUILESS_MAIN(TestSyncMove)
#include "testsyncmove.moc"
