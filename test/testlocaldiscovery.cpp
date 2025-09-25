/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include "testutils/syncenginetestutils.h"
#include "testutils/testutils.h"

#include <syncengine.h>
#include <localdiscoverytracker.h>

#include <QtTest>

using namespace OCC;

class TestLocalDiscovery : public QObject
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

    /// Check correct behavior when local discovery is partially drawn from the db
    void testLocalDiscoveryStyle()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);

        LocalDiscoveryTracker tracker;
        connect(&fakeFolder.syncEngine(), &SyncEngine::itemCompleted, &tracker, &LocalDiscoveryTracker::slotItemCompleted);
        connect(&fakeFolder.syncEngine(), &SyncEngine::finished, &tracker, &LocalDiscoveryTracker::slotSyncFinished);

        // More subdirectories are useful for testing
        fakeFolder.localModifier().mkdir(QStringLiteral("A/X"));
        fakeFolder.localModifier().mkdir(QStringLiteral("A/Y"));
        fakeFolder.localModifier().insert(QStringLiteral("A/X/x1"));
        fakeFolder.localModifier().insert(QStringLiteral("A/Y/y1"));
        tracker.addTouchedPath(QStringLiteral("A/X"));

        tracker.startSyncFullDiscovery();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QVERIFY(tracker.localDiscoveryPaths().empty());

        // Test begins
        fakeFolder.localModifier().insert(QStringLiteral("A/a3"));
        fakeFolder.localModifier().insert(QStringLiteral("A/X/x2"));
        fakeFolder.localModifier().insert(QStringLiteral("A/Y/y2"));
        fakeFolder.localModifier().insert(QStringLiteral("B/b3"));
        fakeFolder.remoteModifier().insert(QStringLiteral("C/c3"));
        fakeFolder.remoteModifier().appendByte(QStringLiteral("C/c1"));
        tracker.addTouchedPath(QStringLiteral("A/X"));

        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::DatabaseAndFilesystem, tracker.localDiscoveryPaths());
        tracker.startSyncPartialDiscovery();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/a3")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/X/x2")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("A/Y/y2")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("B/b3")));
        QVERIFY(fakeFolder.currentLocalState().find(QStringLiteral("C/c3")));
        QCOMPARE(fakeFolder.syncEngine().lastLocalDiscoveryStyle(), LocalDiscoveryStyle::DatabaseAndFilesystem);
        QVERIFY(tracker.localDiscoveryPaths().empty());

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QCOMPARE(fakeFolder.syncEngine().lastLocalDiscoveryStyle(), LocalDiscoveryStyle::FilesystemOnly);
        QVERIFY(tracker.localDiscoveryPaths().empty());
    }

    void testLocalDiscoveryDecision()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        auto &engine = fakeFolder.syncEngine();

        QVERIFY(engine.shouldDiscoverLocally(QString()));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X")));

        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::DatabaseAndFilesystem,
            {QStringLiteral("A/X"), QStringLiteral("A/X space"), QStringLiteral("A/X/beta"), QStringLiteral("foo bar space/touch"), QStringLiteral("foo/"),
                QStringLiteral("zzz"), QStringLiteral("zzzz")});

        QVERIFY(engine.shouldDiscoverLocally(QString()));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("B")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("A B")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("B/X")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("foo bar space")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("foo")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("foo bar")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("foo bar/touch")));
        // These are within QStringLiteral("A/X") so they should be discovered
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X/alpha")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X beta")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X/Y")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X space")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("A/X space/alpha")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("A/Xylo/foo")));
        QVERIFY(engine.shouldDiscoverLocally(QStringLiteral("zzzz/hello")));
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("zzza/hello")));

        QEXPECT_FAIL("", "There is a possibility of false positives if the set contains a path "
            "which is a prefix, and that prefix is followed by a character less than '/'", Continue);
        QVERIFY(!engine.shouldDiscoverLocally(QStringLiteral("A/X o")));

        fakeFolder.syncEngine().setLocalDiscoveryOptions(
            LocalDiscoveryStyle::DatabaseAndFilesystem,
            {});

        QVERIFY(!engine.shouldDiscoverLocally(QString()));
    }

    // Check whether item success and item failure adjusts the
    // tracker correctly.
    void testTrackerItemCompletion()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);

        LocalDiscoveryTracker tracker;
        connect(&fakeFolder.syncEngine(), &SyncEngine::itemCompleted, &tracker, &LocalDiscoveryTracker::slotItemCompleted);
        connect(&fakeFolder.syncEngine(), &SyncEngine::finished, &tracker, &LocalDiscoveryTracker::slotSyncFinished);
        auto trackerContains = [&](const char *path) {
            return tracker.localDiscoveryPaths().find(QString::fromLatin1(path)) != tracker.localDiscoveryPaths().end();
        };

        tracker.addTouchedPath(QStringLiteral("A/spurious"));

        fakeFolder.localModifier().insert(QStringLiteral("A/a3"));
        tracker.addTouchedPath(QStringLiteral("A/a3"));

        fakeFolder.localModifier().insert(QStringLiteral("A/a4"));
        fakeFolder.serverErrorPaths().append(QStringLiteral("A/a4"));
        // We're not adding a4 as touched, it's in the same folder as a3 and will be seen.
        // And due to the error it should be added to the explicit list while a3 gets removed.

        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::DatabaseAndFilesystem, tracker.localDiscoveryPaths());
        tracker.startSyncPartialDiscovery();
        QVERIFY(!fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/a3")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("A/a4")));
        QVERIFY(!trackerContains("A/a3"));
        QVERIFY(trackerContains("A/a4"));
        QVERIFY(trackerContains("A/spurious")); // not removed since overall sync not successful

        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::FilesystemOnly);
        tracker.startSyncFullDiscovery();
        QVERIFY(!fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("A/a4")));
        QVERIFY(trackerContains("A/a4")); // had an error, still here
        QVERIFY(!trackerContains("A/spurious")); // removed due to full discovery

        fakeFolder.serverErrorPaths().clear();
        fakeFolder.syncJournal().wipeErrorBlacklist();
        tracker.addTouchedPath(QStringLiteral("A/newspurious")); // will be removed due to successful sync

        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::DatabaseAndFilesystem, tracker.localDiscoveryPaths());
        tracker.startSyncPartialDiscovery();
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/a4")));
        QVERIFY(tracker.localDiscoveryPaths().empty());
    }

    void testDirectoryAndSubDirectory()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);

        fakeFolder.localModifier().mkdir(QStringLiteral("A/newDir"));
        fakeFolder.localModifier().mkdir(QStringLiteral("A/newDir/subDir"));
        fakeFolder.localModifier().insert(QStringLiteral("A/newDir/subDir/file"), 10_B);

        // Only "A" was modified according to the file system tracker
        fakeFolder.syncEngine().setLocalDiscoveryOptions(
            LocalDiscoveryStyle::DatabaseAndFilesystem,
            { QStringLiteral("A") });

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("A/newDir/subDir/file")));
    }

    // Tests the behavior of invalid filename detection
    void testServerBlacklist()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        FakeFolder fakeFolder(FileInfo::A12_B12_C12_S12(), vfsMode, filesAreDehydrated);
        QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());

        auto cap = TestUtils::testCapabilities();
        cap.insert(
            {{QStringLiteral("files"), QVariantMap{{QStringLiteral("blacklisted_files"), QVariantList{QStringLiteral(".foo"), QStringLiteral("bar")}}}}});
        fakeFolder.account()->setCapabilities({fakeFolder.account()->url(), cap});
        fakeFolder.localModifier().insert(QStringLiteral("C/.foo"));
        fakeFolder.localModifier().insert(QStringLiteral("C/bar"));
        fakeFolder.localModifier().insert(QStringLiteral("C/moo"));
        fakeFolder.localModifier().insert(QStringLiteral("C/.moo"));

        QVERIFY(fakeFolder.applyLocalModificationsAndSync());
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("C/moo")));
        QVERIFY(fakeFolder.currentRemoteState().find(QStringLiteral("C/.moo")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("C/.foo")));
        QVERIFY(!fakeFolder.currentRemoteState().find(QStringLiteral("C/bar")));
    }

    void testNameNormalization_data()
    {
        QTest::addColumn<QString>("correct");
        QTest::addColumn<QString>("incorrect");

        const unsigned char a_umlaut_composed_bytes[] = {0xc3, 0xa4, 0x00};
        const QString a_umlaut_composed = QString::fromUtf8(reinterpret_cast<const char *>(a_umlaut_composed_bytes));
        const QString a_umlaut_decomposed = a_umlaut_composed.normalized(QString::NormalizationForm_D);

        QTest::newRow("a_umlaut decomposed") << a_umlaut_decomposed << a_umlaut_composed;
        QTest::newRow("a_umlaut composed") << a_umlaut_composed << a_umlaut_decomposed;
    }

    // Test that when a file/directory name on the remote is encoded in NFC, the local name is encoded
    // in the same way, and that a subsequent sync does not change anything. And the same for NFD.
    void testNameNormalization()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        QFETCH(QString, correct);
        QFETCH(QString, incorrect);

        // Create an empty remote folder
        FakeFolder fakeFolder({FileInfo{}}, vfsMode, filesAreDehydrated);
        OperationCounter counter(fakeFolder);

        // Create a file with an a-umlout in the "correct" normalization:
        fakeFolder.remoteModifier().mkdir(QStringLiteral("P"));
        fakeFolder.remoteModifier().mkdir(QStringLiteral("P/A"));
        fakeFolder.remoteModifier().insert(QStringLiteral("P/A/") + correct);

        // Same for a directory, holding a "normal" file:
        fakeFolder.remoteModifier().mkdir(QStringLiteral("P/B") + correct);
        fakeFolder.remoteModifier().insert(QStringLiteral("P/B") + correct + QStringLiteral("/b"));

        LocalDiscoveryTracker tracker;
        connect(&fakeFolder.syncEngine(), &SyncEngine::itemCompleted, &tracker, &LocalDiscoveryTracker::slotItemCompleted);
        connect(&fakeFolder.syncEngine(), &SyncEngine::finished, &tracker, &LocalDiscoveryTracker::slotSyncFinished);

        // First sync: discover that there are files/directories on the server that are not yet synced to the local end
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        // Check that locally we have the file and the directory with the correct names:
        {
            auto localState = fakeFolder.currentLocalState();
            QVERIFY(localState.find(QStringLiteral("P/A/") + correct) != nullptr); // check if the file exists
            QVERIFY(localState.find(QStringLiteral("P/B") + correct + QStringLiteral("/b")) != nullptr); // check if the file exists
        }

        counter.reset();

        qDebug() << "*** MARK"; // Log marker to check if a PUT/DELETE shows up in the second sync

        // Force a full local discovery on the next sync, which forces a walk of the (local) file system, reading back names (and file sizes/mtimes/etc.)...
        fakeFolder.syncEngine().setLocalDiscoveryOptions(LocalDiscoveryStyle::DatabaseAndFilesystem, {QStringLiteral("P")});
        tracker.startSyncFullDiscovery();

        // ... and start the second sync:
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        // If the normalization of the file/directory name did not change, no rename/move/etc. should have been detected, so check that the client didn't issue
        // any of these operations:
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nPUT, 0);

        // Check that the remote names are unchanged, and that no "incorrect" names have been introduced:
        FileInfo &remoteState = fakeFolder.currentRemoteState();
        QVERIFY(remoteState.find(QStringLiteral("P/A/") + correct) != nullptr); // check if the file still exists in the original normalization
        QVERIFY(remoteState.find(QStringLiteral("P/A/") + incorrect) == nullptr); // there should NOT be a file with another normalization
        QVERIFY(remoteState.find(QStringLiteral("P/B") + correct + QStringLiteral("/b"))
            != nullptr); // check if the directory still exists in the original normalization
        QVERIFY(remoteState.find(QStringLiteral("P/B") + incorrect + QStringLiteral("/b"))
            == nullptr); // there should NOT be a directory with another normalization
    }

    void testNameNormalizationOC10mac5to6migration()
    {
        QFETCH_GLOBAL(Vfs::Mode, vfsMode);
        QFETCH_GLOBAL(bool, filesAreDehydrated);

        // Create an empty remote folder
        FakeFolder fakeFolder({FileInfo{}}, vfsMode, filesAreDehydrated);
        OperationCounter counter(fakeFolder);

        const unsigned char a_umlaut_composed_bytes[] = {0xc3, 0xa4, 0x00};
        const QString a_umlaut_composed = QString::fromUtf8(reinterpret_cast<const char *>(a_umlaut_composed_bytes));
        const QString a_umlaut_decomposed = a_umlaut_composed.normalized(QString::NormalizationForm_D);

        // OC10 stores names in composed form only
        fakeFolder.remoteModifier().insert(a_umlaut_composed);

        // Download the file
        QVERIFY(fakeFolder.applyLocalModificationsAndSync());

        // With client version 5 on mac, the file name would be decomposed by Qt. Simulate that:
        QString err;
        bool result = FileSystem::uncheckedRenameReplace(fakeFolder.localPath() + a_umlaut_composed, fakeFolder.localPath() + a_umlaut_decomposed, &err);
        QVERIFY(result);
        QVERIFY(err.isEmpty());

        // Now nothing should happen...
        counter.reset();
        QVERIFY(fakeFolder.syncOnce());

        QCOMPARE(counter.nGET, 0);
        QCOMPARE(counter.nDELETE, 0);
        QCOMPARE(counter.nMOVE, 0);
        QCOMPARE(counter.nPUT, 0);
        QVERIFY(fakeFolder.currentRemoteState().find(a_umlaut_composed) != nullptr);
        QVERIFY(fakeFolder.currentRemoteState().find(a_umlaut_decomposed) == nullptr);
    }
};

QTEST_GUILESS_MAIN(TestLocalDiscovery)
#include "testlocaldiscovery.moc"
