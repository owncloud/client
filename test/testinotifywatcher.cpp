/*
 *    This software is in the public domain, furnished "as is", without technical
 *       support, and with no warranty, express or implied, as to its usefulness for
 *          any purpose.
 *          */

#include <QtTest>

#include "folderwatcher_linux.h"
#include "utility.h"

using namespace OCC;


struct FriendlyFolderWatcherPrivate : FolderWatcherPrivate
{
    using FolderWatcherPrivate::FolderWatcherPrivate;
    friend class TestInotifyWatcher;
};


class TestInotifyWatcher : public QObject
{
    Q_OBJECT
private slots:
    // Test the recursive path listing function lists everything
    void testAddFolderRecursiveHelper() {
        QTemporaryDir tmpDir;

        QString _root = tmpDir.path();
        qDebug() << "creating test directory tree in " << _root;
        QDir rootDir(_root);

        rootDir.mkpath(_root + "/a1/b1/c1");
        rootDir.mkpath(_root + "/a1/b1/c2");
        rootDir.mkpath(_root + "/a1/b2/c1");
        rootDir.mkpath(_root + "/a1/b3/c3");
        rootDir.mkpath(_root + "/a2/b3/c3");

        FriendlyFolderWatcherPrivate watcher(0, _root);
        QVERIFY(watcher._fd >= 0);
        QCoreApplication::processEvents(); // Let the slotAddFolderRecursive slot run;
        QStringList dirs = watcher._watches.values();

        QVERIFY( dirs.indexOf(_root)>-1);

        QVERIFY( dirs.indexOf(_root + "/a1")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b1")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b1/c1")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b1/c2")>-1);

        QVERIFY(Utility::writeRandomFile(_root+"/a1/rand1.dat"));
        QVERIFY(Utility::writeRandomFile(_root+"/a1/b1/rand2.dat"));
        QVERIFY(Utility::writeRandomFile(_root+"/a1/b1/c1/rand3.dat"));

        QVERIFY( dirs.indexOf(_root + "/a1/b2")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b2/c1")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b3")>-1);
        QVERIFY( dirs.indexOf(_root + "/a1/b3/c3")>-1);

        QVERIFY( dirs.contains(_root + "/a2"));
        QVERIFY( dirs.contains(_root + "/a2/b3"));
        QVERIFY( dirs.contains(_root + "/a2/b3/c3"));

        QCOMPARE(dirs.count(), 12);
    }

};

QTEST_APPLESS_MAIN(TestInotifyWatcher)
#include "testinotifywatcher.moc"
