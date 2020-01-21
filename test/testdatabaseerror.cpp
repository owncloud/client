/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <QtTest>
#include "syncenginetestutils.h"
#include <syncengine.h>

using namespace OCC;


class TestDatabaseError : public QObject
{
    Q_OBJECT

private slots:
    void testDatabaseError() {
        /* This test will make many iteration, at each iteration, the iᵗʰ database access will fail.
         * The test ensure that if there is a failure, the next sync recovers. And if there was
         * no error, then everything was sync'ed properly.
         */

        FileInfo finalState;
        for (int count = 0; true; ++count) {
            qInfo() << "Starting Iteration" << count;

            FakeFolder fakeFolder{FileInfo::A12_B12_C12_S12()};

            // Do a couple of changes
            fakeFolder.remoteModifier().insert(QStringLiteral("A/a0"));
            fakeFolder.remoteModifier().appendByte(QStringLiteral("A/a1"));
            fakeFolder.remoteModifier().remove(QStringLiteral("A/a2"));
            fakeFolder.remoteModifier().rename(QStringLiteral("S/s1"), QStringLiteral("S/s1_renamed"));
            fakeFolder.remoteModifier().mkdir(QStringLiteral("D"));
            fakeFolder.remoteModifier().mkdir(QStringLiteral("D/subdir"));
            fakeFolder.remoteModifier().insert(QStringLiteral("D/subdir/file"));
            fakeFolder.localModifier().insert(QStringLiteral("B/b0"));
            fakeFolder.localModifier().appendByte(QStringLiteral("B/b1"));
            fakeFolder.remoteModifier().remove(QStringLiteral("B/b2"));
            fakeFolder.localModifier().mkdir(QStringLiteral("NewDir"));
            fakeFolder.localModifier().rename(QStringLiteral("C"), QStringLiteral("NewDir/C"));

            // Set the counter
            fakeFolder.syncJournal().autotestFailCounter = count;

            // run the sync
            bool result = fakeFolder.syncOnce();

            qInfo() << "Result of iteration" << count << "was" << result;

            if (fakeFolder.syncJournal().autotestFailCounter >= 0) {
                // No error was thrown, we are finished
                QVERIFY(result);
                QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
                QCOMPARE(fakeFolder.currentRemoteState(), finalState);
                return;
            }

            if (!result) {
                fakeFolder.syncJournal().autotestFailCounter = -1;
                // Try again
                QVERIFY(fakeFolder.syncOnce());
            }

            QCOMPARE(fakeFolder.currentLocalState(), fakeFolder.currentRemoteState());
            if (count == 0) {
                finalState = fakeFolder.currentRemoteState();
            } else {
                // the final state should be the same for every iteration
                QCOMPARE(fakeFolder.currentRemoteState(), finalState);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestDatabaseError)
#include "testdatabaseerror.moc"
