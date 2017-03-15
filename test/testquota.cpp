/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <QtTest>
#include "syncenginetestutils.h"
#include <syncengine.h>
#include <syncjournaldb.h>

using namespace OCC;

class TestQuota : public QObject
{
    Q_OBJECT

private slots:

    // Verify that too low quota leads to skipping uploads
    void testSkipUpload() {
        FakeFolder fakeFolder{FileInfo::A12_B12_C12_S12()};

        fakeFolder.syncEngine().account()->setCapabilities({ { "dav", QVariantMap{
                {"chunking", "1.0"} } } });

        fakeFolder.remoteModifier().quota_available = 5 * 1000 * 1000; // 5 MB

        int size;

        // TEST: Upload of smaller file succeeds
        // (note that this doesn't change quota_available)
        size = 4 * 1000 * 1000; // 4 MB
        fakeFolder.localModifier().insert("A/new0", size);
        QVERIFY(fakeFolder.syncOnce());


        // TEST: Increasing the size of an existing file by an amount
        // smaller than the quota succeeds
        size = 6 * 1000 * 1000; // 6 MB (going from 4 MB to 6 MB)
        fakeFolder.localModifier().remove("A/new0");
        fakeFolder.localModifier().insert("A/new0", size);
        QVERIFY(fakeFolder.syncOnce());


        // TEST: Upload of larger file fails
        size = 6 * 1000 * 1000; // 6 MB
        fakeFolder.localModifier().insert("A/new1", size);
        QVERIFY(!fakeFolder.syncOnce());
        fakeFolder.localModifier().remove("A/new1");


        // TEST: Exceeding the quota gradually works and makes
        // some uploads succeed. This also tests parallelism
        // in uploads interacting with quota checks.
        size = 2 * 1000 * 1000; // 2 MB
        fakeFolder.localModifier().insert("A/new2", size);
        fakeFolder.localModifier().insert("A/new3", size);
        fakeFolder.localModifier().insert("A/new4", size);
        // the last of these three will fail
        QVERIFY(!fakeFolder.syncOnce());

        auto folderA = fakeFolder.currentRemoteState().children["A"];
        int successes =
            folderA.children.count("new2")
            + folderA.children.count("new3")
            + folderA.children.count("new4");
        QVERIFY(successes == 2);
    }
};

QTEST_GUILESS_MAIN(TestQuota)
#include "testquota.moc"
