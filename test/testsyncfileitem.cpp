/*
 *    This software is in the public domain, furnished "as is", without technical
 *       support, and with no warranty, express or implied, as to its usefulness for
 *          any purpose.
 *          */

#include <QtTest>

#include "syncfileitem.h"

using namespace OCC;

class TestSyncFileItem : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() {
    }

    void cleanupTestCase() {
    }

    void testComparator_data() {
        QTest::addColumn<SyncFileItem>("a");
        QTest::addColumn<SyncFileItem>("b");
        QTest::addColumn<SyncFileItem>("c");

        QTest::newRow("a1") << SyncFileItem(QStringLiteral("client")) << SyncFileItem(QStringLiteral("client/build"))
                            << SyncFileItem(QStringLiteral("client-build"));
        QTest::newRow("a2") << SyncFileItem(QStringLiteral("test/t1")) << SyncFileItem(QStringLiteral("test/t2")) << SyncFileItem(QStringLiteral("test/t3"));
        QTest::newRow("a3") << SyncFileItem(QStringLiteral("ABCD")) << SyncFileItem(QStringLiteral("abcd")) << SyncFileItem(QStringLiteral("zzzz"));

        SyncFileItem movedItem1(QStringLiteral("folder/source/file.f"));
        movedItem1._renameTarget = QStringLiteral("folder/destination/file.f");
        movedItem1.setInstruction(CSYNC_INSTRUCTION_RENAME);

        QTest::newRow("move1") << SyncFileItem(QStringLiteral("folder/destination")) << movedItem1 << SyncFileItem(QStringLiteral("folder/destination-2"));
        QTest::newRow("move2") << SyncFileItem(QStringLiteral("folder/destination/1")) << movedItem1 << SyncFileItem(QStringLiteral("folder/source"));
        QTest::newRow("move3") << SyncFileItem(QStringLiteral("abc")) << movedItem1 << SyncFileItem(QStringLiteral("ijk"));
    }

    void testComparator() {
        QFETCH( SyncFileItem , a );
        QFETCH( SyncFileItem , b );
        QFETCH( SyncFileItem , c );

        QVERIFY(a < b);
        QVERIFY(b < c);
        QVERIFY(a < c);

        QVERIFY(!(b < a));
        QVERIFY(!(c < b));
        QVERIFY(!(c < a));

        QVERIFY(!(a < a));
        QVERIFY(!(b < b));
        QVERIFY(!(c < c));
    }
};

QTEST_APPLESS_MAIN(TestSyncFileItem)
#include "testsyncfileitem.moc"
